#ifndef BH_STRING
#define BH_STRING

#include <array>
#include <cstdint>
#include <numeric>

#include "Utils.hpp"
#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_err.h"
#include "hal/adc_types.h"

namespace bh {

static constexpr std::uint8_t threshold{100};

struct ADC {
  static ADC &instance() noexcept {
    static ADC instance;
    return instance;
  }

  ADC(const ADC &) = delete;
  ADC &operator=(const ADC &) = delete;
  ADC(ADC &&) = delete;
  ADC &operator=(ADC &&) = delete;

  ::adc_oneshot_unit_handle_t handle() const noexcept { return m_adc; }

private:
  ADC() noexcept {
    ::adc_oneshot_unit_init_cfg_t cfg{};
    cfg.unit_id = ::ADC_UNIT_1;
    ESP_ERROR_CHECK(::adc_oneshot_new_unit(&cfg, &m_adc));
  }

  ~ADC() noexcept { ESP_ERROR_CHECK(::adc_oneshot_del_unit(m_adc)); }

  ::adc_oneshot_unit_handle_t m_adc{};
};

template <std::uint8_t Num>
concept StringIndex = Num < 4;

template <::gpio_num_t PIN, std::uint8_t N>
  requires StringIndex<N>
class String {
public:
  explicit String(const bh::FretValues &fretValues) noexcept;
  inline ~String() noexcept = default;

  inline void check(std::uint32_t &note) const noexcept {

    int adcRead{};
    if (::adc_oneshot_read(m_adc, ::ADC_CHANNEL_0, &adcRead) != ESP_OK) {
      return;
    }

    std::uint16_t val = std::saturating_cast<std::uint16_t>(adcRead);
    for (std::uint8_t i{}; i < m_fretValues.size(); ++i) {
      const auto &fretVal = m_fretValues[i];
      if (fretVal - threshold <= val && val <= fretVal + threshold) {
        note |= i << (2 + (N * 5));
      }
    }
  }

private:
  const std::array<std::uint16_t, 24> &m_fretValues;
  ::adc_oneshot_unit_handle_t m_adc{ADC::instance().handle()};
};

template <::gpio_num_t PIN, std::uint8_t N>
  requires StringIndex<N>
String<PIN, N>::String(const std::array<std::uint16_t, 24> &fretValues) noexcept
    : m_fretValues(fretValues) {

  ::gpio_config_t cfg;

  cfg.pin_bit_mask = 1ULL << PIN;
  cfg.mode = ::GPIO_MODE_INPUT;
  cfg.pull_up_en = ::GPIO_PULLUP_DISABLE;
  cfg.pull_down_en = ::GPIO_PULLDOWN_ENABLE;
  cfg.intr_type = ::GPIO_INTR_POSEDGE;

  ::gpio_config(&cfg);
}

} // namespace bh

#endif
