#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace bh {

struct SerialCommand {
  enum class Kind { None, Receiver, Invalid };
  Kind kind{Kind::None};
  std::array<std::uint8_t, 6> address{};
};

// Match the stick's M <12 hex digits> command, completed by CR or LF.
class SerialCommandReader {
public:
  SerialCommand push(char ch) noexcept {
    if (ch == '\r' || ch == '\n') {
      const auto command = m_discard ? SerialCommand{SerialCommand::Kind::Invalid}
                                    : parse();
      m_size = 0;
      m_discard = false;
      return command;
    }
    if (ch == '\0' || m_size == sizeof(m_line)) {
      m_discard = true;
    }
    if (!m_discard) {
      m_line[m_size++] = ch;
    }
    return {};
  }

private:
  SerialCommand parse() const noexcept {
    using Kind = SerialCommand::Kind;
    if (m_size == 0) {
      return {}; // Includes the LF after a CRLF terminator.
    }
    if (m_size != sizeof(m_line) ||
        (m_line[0] != 'M' && m_line[0] != 'm') || m_line[1] != ' ') {
      return {Kind::Invalid};
    }
    SerialCommand command{Kind::Receiver};
    for (std::size_t i = 2; i < m_size; ++i) {
      const char ch = m_line[i];
      const int nibble = ch >= '0' && ch <= '9' ? ch - '0'
                         : ch >= 'a' && ch <= 'f' ? ch - 'a' + 10
                         : ch >= 'A' && ch <= 'F' ? ch - 'A' + 10
                                                 : -1;
      if (nibble < 0) {
        return {Kind::Invalid};
      }
      auto &byte = command.address[(i - 2) / 2];
      byte = static_cast<std::uint8_t>((byte << 4) | nibble);
    }
    return command;
  }

  char m_line[14]{};
  std::size_t m_size{};
  bool m_discard{};
};

} // namespace bh
