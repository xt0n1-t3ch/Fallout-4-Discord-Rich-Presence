#include "Discord/Pipe.h"

#include <array>

#include <Windows.h>
#include <fmt/xchar.h>

#include "Constants.h"
#include "Util/Logger.h"

namespace F4DRP::Discord {
Pipe::Pipe(Pipe&& other) noexcept : m_handleRaw(other.m_handleRaw)
{
    other.m_handleRaw = nullptr;
}

Pipe& Pipe::operator=(Pipe&& other) noexcept
{
    if (this != &other) {
        close();
        m_handleRaw = other.m_handleRaw;
        other.m_handleRaw = nullptr;
    }
    return *this;
}

bool Pipe::open()
{
    close();
    for (int i = Constants::kPipeIndexMin; i <= Constants::kPipeIndexMax; ++i) {
        auto path = fmt::format(std::wstring{Constants::kPipePathFormat}, i);
        HANDLE h = ::CreateFileW(path.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
        if (h != INVALID_HANDLE_VALUE) {
            m_handleRaw = h;
            F4DRP_LOG_INFO("discord pipe opened: index={}", i);
            return true;
        }
    }
    F4DRP_LOG_WARN(
        "discord pipe unavailable (tried indices {}..{})", Constants::kPipeIndexMin, Constants::kPipeIndexMax);
    return false;
}

void Pipe::close() noexcept
{
    if (m_handleRaw != nullptr) {
        ::CloseHandle(static_cast<HANDLE>(m_handleRaw));
        m_handleRaw = nullptr;
    }
}

bool Pipe::write(std::span<const std::uint8_t> bytes)
{
    if (!isOpen() || bytes.empty()) {
        return false;
    }
    DWORD written = 0;
    const auto ok = ::WriteFile(
        static_cast<HANDLE>(m_handleRaw), bytes.data(), static_cast<DWORD>(bytes.size()), &written, nullptr);
    if (ok == 0 || written != bytes.size()) {
        F4DRP_LOG_WARN("discord pipe write failed (ok={}, written={}, expected={})", ok != 0, written, bytes.size());
        close();
        return false;
    }
    return true;
}

std::optional<Frame> Pipe::readFrame()
{
    if (!isOpen()) {
        return std::nullopt;
    }
    std::array<std::uint8_t, Constants::kPayloadHeaderBytes> header{};
    DWORD read = 0;
    if (::ReadFile(
            static_cast<HANDLE>(m_handleRaw), header.data(), static_cast<DWORD>(header.size()), &read, nullptr) == 0 ||
        read != header.size())
    {
        close();
        return std::nullopt;
    }

    std::uint32_t opcode = 0;
    std::uint32_t length = 0;
    std::memcpy(&opcode, header.data(), sizeof(opcode));
    std::memcpy(&length, header.data() + sizeof(opcode), sizeof(length));

    if (length > Constants::kPayloadMaxBytes) {
        F4DRP_LOG_WARN("discord pipe frame length {} exceeds {}", length, Constants::kPayloadMaxBytes);
        close();
        return std::nullopt;
    }

    std::string payload;
    if (length > 0) {
        payload.resize(length);
        DWORD payloadRead = 0;
        if (::ReadFile(static_cast<HANDLE>(m_handleRaw), payload.data(), length, &payloadRead, nullptr) == 0 ||
            payloadRead != length)
        {
            close();
            return std::nullopt;
        }
    }
    return Frame{static_cast<Opcode>(opcode), std::move(payload)};
}
} // namespace F4DRP::Discord
