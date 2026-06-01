#pragma once
#include <cstdint>

#include "driver/gpio.h"

namespace bh
{

  enum class STRING
  {
    ONE = 1,
    TWO,
    THREE,
    FOUR,
  };

  template <gpio_num_t PIN>
  class String
  {
  public:
    String() noexcept;

    inline int read() const noexcept;
  };

  template <gpio_num_t PIN>
  String<PIN>::String() noexcept
  {

    gpio_config_t cfg;
    cfg.pin_bit_mask = 1ULL << PIN;
    cfg.mode = GPIO_MODE_INPUT;
    cfg.pull_up_en = GPIO_PULLUP_DISABLE;
    cfg.pull_down_en = GPIO_PULLDOWN_DISABLE;
    cfg.intr_type = GPIO_INTR_POSEDGE;
  }

  template <gpio_num_t PIN>
  inline int String<PIN>::read() const noexcept
  {
    return gpio_get_level(PIN);
  }

} // namespace bh
