#include "Utils.hpp"

bool IRAM_ATTR bh::ButtonDebounce::accept() noexcept {
  const ::TickType_t now = ::xTaskGetTickCountFromISR();
  // Unsigned subtraction also handles tick-counter wraparound.
  if (has_press && static_cast<::TickType_t>(now - last_press_tick) <
                       bh::BUTTON_DEBOUNCE_TICKS) {
    return false;
  }
  last_press_tick = now;
  has_press = true;
  return true;
}

void IRAM_ATTR bh::mode_button_pressed(void *args) noexcept {
  if (args != nullptr && bh::mode_button_debounce.accept()) {
    static_cast<bh::AtomicState *>(args)->store(bh::State::Config,
                                               std::memory_order_relaxed);
  }
}
