#include "Discord/Client.h"

#include <algorithm>
#include <random>

#include <Windows.h>

#include "Constants.h"
#include "Util/Logger.h"

namespace F4DRP::Discord {
Client::Client() = default;

Client::~Client()
{
    stop();
}

void Client::start(std::string appId)
{
    if (m_running.exchange(true)) {
        return;
    }
    m_appId = std::move(appId);
    m_worker = std::thread{&Client::run, this};
}

void Client::stop()
{
    if (!m_running.exchange(false)) {
        return;
    }
    {
        std::scoped_lock lock{m_mutex};
        m_cv.notify_all();
    }
    if (m_worker.joinable()) {
        m_worker.join();
    }
    m_pipe.close();
    m_state = State::Disconnected;
    m_ready.store(false, std::memory_order_release);
}

void Client::update(PresenceState desired, int updateIntervalSec)
{
    const auto clamped = clampToDiscordLimits(std::move(desired));
    {
        std::scoped_lock lock{m_mutex};
        m_desired = clamped;
        m_desiredValid = true;
        m_updateIntervalSec = updateIntervalSec;
    }
    m_cv.notify_one();
}

std::string Client::generateNonce()
{
    static thread_local std::mt19937_64 rng{std::random_device{}()};
    std::uniform_int_distribution<std::uint64_t> dist;
    const auto a = dist(rng);
    const auto b = dist(rng);
    char buf[33]{};
    std::snprintf(
        buf, sizeof(buf), "%016llx%016llx", static_cast<unsigned long long>(a), static_cast<unsigned long long>(b));
    return std::string{buf};
}

bool Client::connect()
{
    if (m_pipe.open()) {
        m_state = State::Handshaking;
        m_lastTraffic = std::chrono::steady_clock::now();
        return true;
    }
    return false;
}

bool Client::handshake()
{
    const auto payload = buildHandshakePayload(m_appId).dump();
    const auto bytes = encodeFrame(Opcode::Handshake, payload);
    if (!m_pipe.write(bytes)) {
        return false;
    }
    const auto frame = m_pipe.readFrame();
    if (!frame) {
        return false;
    }
    if (frame->opcode != Opcode::Frame) {
        F4DRP_LOG_WARN("discord handshake unexpected opcode={}", static_cast<int>(frame->opcode));
        return false;
    }
    m_state = State::Ready;
    m_ready.store(true, std::memory_order_release);
    m_lastTraffic = std::chrono::steady_clock::now();
    F4DRP_LOG_INFO("discord ready: {}", frame->payload.substr(0, 256));
    return true;
}

void Client::pumpReads()
{
    while (m_pipe.isOpen()) {
        DWORD avail = 0;
        if (::PeekNamedPipe(static_cast<HANDLE>(::GetCurrentProcess()), nullptr, 0, nullptr, &avail, nullptr) == 0 &&
            avail == 0)
        {
            break;
        }
        auto frame = m_pipe.readFrame();
        if (!frame) {
            break;
        }
        m_lastTraffic = std::chrono::steady_clock::now();
        if (frame->opcode == Opcode::Ping) {
            m_pipe.write(encodeFrame(Opcode::Pong, frame->payload));
        }
        else if (frame->opcode == Opcode::Close) {
            F4DRP_LOG_WARN("discord sent CLOSE: {}", frame->payload);
            m_pipe.close();
        }
    }
}

void Client::tryFlushDesired()
{
    PresenceState snapshot;
    int interval = 0;
    bool have = false;
    {
        std::scoped_lock lock{m_mutex};
        if (m_desiredValid) {
            snapshot = m_desired;
            interval = m_updateIntervalSec;
            have = true;
        }
    }
    if (!have) {
        return;
    }
    const auto now = std::chrono::steady_clock::now();
    const auto decision = m_limiter.admit(snapshot.hash, now, interval);
    if (!decision.send) {
        return;
    }
    if (snapshot.hash == m_lastSentHash) {
        return;
    }
    const auto payload =
        buildSetActivityPayload(static_cast<int>(::GetCurrentProcessId()), generateNonce(), snapshot).dump();
    const auto bytes = encodeFrame(Opcode::Frame, payload);
    if (m_pipe.write(bytes)) {
        m_lastSentHash = snapshot.hash;
        m_lastTraffic = now;
        if (!snapshot.details.empty() || !snapshot.state.empty()) {
            F4DRP_LOG_DBG("discord set_activity sent hash={} details='{}' state='{}'",
                          snapshot.hash,
                          snapshot.details,
                          snapshot.state);
        }
        else {
            F4DRP_LOG_DBG("discord set_activity sent hash={} (empty details+state — App default will show)",
                          snapshot.hash);
        }
    }
    else {
        F4DRP_LOG_WARN("discord set_activity write failed");
    }
}

void Client::run()
{
    while (m_running.load(std::memory_order_acquire)) {
        if (m_state == State::Disconnected) {
            if (!connect()) {
                const std::size_t idx = std::min(m_attempt, Constants::kReconnectBackoffSec.size() - 1);
                const auto wait = std::chrono::seconds{Constants::kReconnectBackoffSec[idx]};
                ++m_attempt;
                std::unique_lock lock{m_mutex};
                m_cv.wait_for(lock, wait, [&] { return !m_running.load(); });
                continue;
            }
            m_attempt = 0;
        }

        if (m_state == State::Handshaking) {
            if (!handshake()) {
                m_pipe.close();
                m_state = State::Disconnected;
                m_ready.store(false, std::memory_order_release);
                continue;
            }
        }

        pumpReads();
        if (!m_pipe.isOpen()) {
            m_state = State::Disconnected;
            m_ready.store(false, std::memory_order_release);
            continue;
        }

        tryFlushDesired();

        const auto now = std::chrono::steady_clock::now();
        const auto idle = now - m_lastTraffic;
        const auto idleMax = std::chrono::seconds{Constants::kHeartbeatIdleSec};
        if (idle > idleMax) {
            m_pipe.write(encodeFrame(Opcode::Ping, "{}"));
            m_lastPing = now;
            m_lastTraffic = now;
        }

        std::unique_lock lock{m_mutex};
        m_cv.wait_for(lock, std::chrono::milliseconds{250}, [&] { return !m_running.load(); });
    }
}
} // namespace F4DRP::Discord
