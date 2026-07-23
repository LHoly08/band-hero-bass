#ifndef BH_STATE
#define BH_STATE

#include <cstdint>

namespace bh {

enum class State : std::uint8_t {
  Config = 0,
  PlayingHub,
  PlayingSerial,
  PlayingBluetooth,
};

} // namespace bh

#endif
