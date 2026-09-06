#ifndef BH_STRING
#define BH_STRING

#include <array>
#include <cstdint>

#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_err.h"
#include "hal/adc_types.h"

namespace bh {

static constexpr std::uint8_t threshold{100};

static std::array<std::uint16_t, 24> fretValues{};

struct ADC {
  static ADC &instance() noexcept {
    static ADC instance;
    return instance;
  }

  ADC(const ADC &) = delete;
  ADC &operator=(const ADC &) = delete;
  ADC(ADC &&) = delete;
  ADC &operator=(ADC &&) = delete;

  adc_oneshot_unit_handle_t handle() const noexcept { return m_adc; }

private:
  ADC() noexcept {
    adc_oneshot_unit_init_cfg_t cfg{};
    cfg.unit_id = ADC_UNIT_1;
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&cfg, &m_adc));
  }

  ~ADC() noexcept { ESP_ERROR_CHECK(adc_oneshot_del_unit(m_adc)); }

  adc_oneshot_unit_handle_t m_adc{};
};

template <std::uint8_t Num>
concept StringIndex = Num < 4;

template <gpio_num_t PIN, std::uint8_t N>
  requires StringIndex<N>
class String {
public:
  explicit String(bool &played) noexcept;
  inline ~String() noexcept { gpio_isr_handler_remove(PIN); }

  inline void check(std::uint32_t &packet) const noexcept {

    auto adc = bh::ADC::instance().handle();

    std::uint16_t val{};
    (void)adc_oneshot_read(adc, ADC_CHANNEL_0, &val);

    for (std::uint8_t i{}; i < fretValues.size(); ++i) {
      const auto &fretVal = fretValues[i];
      if (fretVal - threshold <= val && val <= fretVal + threshold) {
        packet |= i << (2 + (N * 5));
      }
    }
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
