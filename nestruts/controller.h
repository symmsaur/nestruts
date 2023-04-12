#pragma once

#include "log.h"
#include <cstdint>
#include <stdexcept>

enum class button : uint8_t {
    a = 1 << 0,
    b = 1 << 1,
    select = 1 << 2,
    start = 1 << 3,
    up = 1 << 4,
    down = 1 << 5,
    left = 1 << 6,
    right = 1 << 7,
};

class controller {
    uint8_t m_state{};
    bool m_updating{};

  public:
    void write(uint8_t val) {
        // log(log_level::error, "write: {}\n", static_cast<int>(val));
        // 0 bit controls updating state.
        m_updating = val & 0x01;
    }
    void clear() {
        log(log_level::debug, "clear controller\n");
        m_state = 0x00;
    }
    void down(button b) {
        log(log_level::debug, "in down b: {}\n", static_cast<uint8_t>(b));
        m_state |= static_cast<uint8_t>(b);
        // log(log_level::error, "new_state: {}\n", static_cast<int>(m_state));
    }
    // Simulate a shift register.
    uint8_t read() {
        log(log_level::debug, "read state: ${:02x} updating: {}\n", m_state,
            m_updating);
        // Controller 1 is written to lowest bit.
        uint8_t res = m_state & 0x01;
        if (!m_updating) {
            m_state >>= 1;
            // When all bits are read we should get 1s.
            m_state |= 0x80;
        }
        return res;
    }
};
