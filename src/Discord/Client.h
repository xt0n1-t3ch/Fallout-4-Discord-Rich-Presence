#pragma once

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <string>
#include <thread>

#include "Discord/Pipe.h"
#include "Discord/Protocol.h"
#include "Discord/RateLimiter.h"

namespace F4DRP::Discord {
class Client
{
public:
    Client();
    ~Client();

    Client(const Client&) = delete;
    Client& operator=(const Client&) = delete;
    Client(Client&&) = delete;
    Client& operator=(Client&&) = delete;

    void start(std::string appId);
    void stop();

    void update(PresenceState desired, int updateIntervalSec);

    bool isReady() const noexcept { return m_ready.load(std::memory_order_acquire); }

private:
    enum class State : std::uint8_t
    {
        Disconnected,
        Handshaking,
        Ready
    };

    void run();
    bool connect();
    bool handshake();
    void pumpReads();
    void tryFlushDesired();
    std::string generateNonce();

    std::string m_appId;
    std::thread m_worker;
    std::atomic_bool m_running{false};
    std::atomic_bool m_ready{false};
    State m_state = State::Disconnected;
    std::size_t m_attempt = 0;
    std::chrono::steady_clock::time_point m_lastTraffic{};
    std::chrono::steady_clock::time_point m_lastPing{};

    Pipe m_pipe;
    RateLimiter m_limiter;

    std::mutex m_mutex;
    std::condition_variable m_cv;
    bool m_desiredValid = false;
    PresenceState m_desired;
    int m_updateIntervalSec = 3;
    std::uint64_t m_lastSentHash = 0;
};
} // namespace F4DRP::Discord
