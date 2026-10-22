#ifndef BH_STATE
#define BH_STATE

#include <array>
#include <atomic>
#include <cstdint>

#include "driver/gpio.h"
#include "esp_attr.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace bh {

enum class State : std::uint32_t {
  Config = 0,
  PlayingHub,
  PlayingSerial,
};

using AtomicState = std::atomic<State>;
static_assert(AtomicState::is_always_lock_free);
static_assert(std::atomic<std::uint32_t>::is_always_lock_free);

using MAC = std::array<std::uint8_t, 6>;
using FretValues = std::array<std::uint16_t, 24>;

constexpr ::gpio_num_t STATE_BUTTON_PIN = ::GPIO_NUM_20;

constexpr std::uint32_t BUTTON_DEBOUNCE_MS = 200;
constexpr ::TickType_t BUTTON_DEBOUNCE_TICKS =
    (BUTTON_DEBOUNCE_MS * configTICK_RATE_HZ + 999) / 1000;
static_assert(sizeof(::TickType_t) == sizeof(std::uint32_t));

struct ButtonDebounce {
  ::TickType_t last_press_tick{0};
  bool has_press{false};

  bool accept() noexcept;
};

// Accessed only by the serialized GPIO ISRs; persist across mode changes.
inline DRAM_ATTR ButtonDebounce mode_button_debounce{};

// Defined out of line so Xtensa places ISR literals with the IRAM code.
void mode_button_pressed(void *args) noexcept;

} // namespace bh

#endif
