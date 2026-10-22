#include <array>
#include <cstdint>

#include "Playing.hpp"
#include "Utils.hpp"

#include "driver/gpio.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

extern "C" void app_main() {
  constexpr std::uint64_t macAddressComptime = BH_MAC;

  bh::AtomicState state{bh::State::PlayingHub};
  bh::MAC macAddress{
      static_cast<std::uint8_t>(macAddressComptime >> (8 * 5)),
      static_cast<std::uint8_t>(macAddressComptime >> (8 * 4)),
      static_cast<std::uint8_t>(macAddressComptime >> (8 * 3)),
      static_cast<std::uint8_t>(macAddressComptime >> (8 * 2)),
      static_cast<std::uint8_t>(macAddressComptime >> (8 * 1)),
      static_cast<std::uint8_t>(macAddressComptime >> (8 * 0))};

  bh::FretValues fretValues{};

  ::vTaskDelay(pdMS_TO_TICKS(1000));

infinite_loop: {

  switch (state.load(std::memory_order_relaxed)) {
  case bh::State::Config: {
    break;
  }
  case bh::State::PlayingHub: {
    bh::Play<bh::State::PlayingHub> play(
        {.state = state, .macAddress = macAddress}, fretValues);
    play.loop();

    break;
  }
  case bh::State::PlayingSerial: {
    bh::Play<bh::State::PlayingSerial> play({.state = state}, fretValues);
    play.loop();
    break;
  }
  }
}

  goto infinite_loop;
}
