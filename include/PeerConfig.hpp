#pragma once

#include "SerialCommand.hpp"
#include "Utils.hpp"

namespace bh {

// Owned and polled by the main task, so updates cannot race ESP-NOW sends.
class PeerConfig {
public:
  explicit PeerConfig(const bh::MAC &defaultAddress) noexcept;

  const bh::MAC &address() const noexcept { return m_address; }
  void registerPeer() const noexcept;
  // Pass true only while ESP-NOW is initialized and the peer is registered.
  void poll(bool hubActive = false) noexcept;

private:
  void update(const bh::MAC &address, bool hubActive) noexcept;

  bh::MAC m_address;
  bh::SerialCommandReader m_reader;
  ::TickType_t m_last_poll{};
};

} // namespace bh
