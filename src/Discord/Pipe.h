#pragma once

#include <cstdint>
#include <optional>
#include <span>
#include <vector>

#include "Discord/Protocol.h"

namespace F4DRP::Discord {
class Pipe
{
public:
    Pipe() = default;
    ~Pipe() { close(); }

    Pipe(const Pipe&) = delete;
    Pipe& operator=(const Pipe&) = delete;
    Pipe(Pipe&& other) noexcept;
    Pipe& operator=(Pipe&& other) noexcept;

    bool open();
    void close() noexcept;
    bool isOpen() const noexcept { return m_handleRaw != nullptr; }

    bool write(std::span<const std::uint8_t> bytes);
    std::optional<Frame> readFrame();

private:
    void* m_handleRaw = nullptr;
};
} // namespace F4DRP::Discord
