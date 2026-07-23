#ifndef BH_STRING
#define BH_STRING

#include <cstdint>

#include "driver/gpio.h"

namespace bh {

template <std::uint8_t Num>
concept StringIndex = Num < 4;

template <gpio_num_t PIN, std::uint8_t N>
  requires StringIndex<N>
class String {
public:
  explicit String(bool &played) noexcept;
  inline ~String() noexcept { gpio_isr_handler_remove(PIN); }

  inline std::uint8_t check(std::uint32_t &packet) const noexcept {
    std::uint32_t val{};
    packet |= val << (2 + (N * 5));
  }

private:
  inline static void IRAM_ATTR PlayedCallback(void *args) noexcept {
    *(static_cast<volatile bool *>(args)) = true;
  }
};

template <gpio_num_t PIN, std::uint8_t N>
  requires StringIndex<N>
String<PIN, N>::String(bool &played) noexcept {

  gpio_config_t cfg;

  cfg.pin_bit_mask = 1ULL << PIN;
  cfg.mode = GPIO_MODE_INPUT;
  cfg.pull_up_en = GPIO_PULLUP_DISABLE;
  cfg.pull_down_en = GPIO_PULLDOWN_DISABLE;
  cfg.intr_type = GPIO_INTR_POSEDGE;

  gpio_config(&cfg);

  gpio_isr_handler_add(PIN, PlayedCallback, static_cast<void *>(&played));
}

} // namespace bh

#endif
