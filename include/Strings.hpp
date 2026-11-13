#ifndef BH_STRING
#define BH_STRING

#include <array>
#include <cstdint>
#include <meta>
#include <numeric>
#include <ranges>
#include <string>
#include <type_traits>
#include <vector>

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
        break;
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

inline auto StringsClass() {
  class Strings;

  consteval {
    static constexpr auto GPIOS =
        std::array{::GPIO_NUM_1, ::GPIO_NUM_2, ::GPIO_NUM_3, ::GPIO_NUM_4};
    std::vector<std::meta::info> members;

    template for (constexpr auto i :
                  std::define_static_array(std::ranges::views::iota(
                      0, static_cast<int>(GPIOS.size())))) {
      constexpr gpio_num_t gpio = GPIOS[i];
      std::string name = "string";
      name.push_back(static_cast<char>('1' + i));
      members.push_back(
          std::meta::data_member_spec(^^String<gpio, i>, {
                                                             .name = name}));
    }
    std::meta::define_aggregate(^^Strings, members);
  }

  class StringMethod : protected Strings {
  private:
    static Strings makeStrings(const bh::FretValues &fretValues) {
      return [](const auto &initial_value) {
        constexpr auto [... members] =
        [:std::meta::reflect_constant_array(
              std::meta::nonstatic_data_members_of(
                  ^^Strings, std::meta::access_context::current())):];

        return Strings{
            typename[:std::meta::type_of(members):]{initial_value}...};
      }(fretValues);
    }

  public:
    explicit StringMethod(const bh::FretValues &fretValues) noexcept
        : Strings(makeStrings(fretValues)) {}

    inline void play(std::uint32_t &val) noexcept {
      template for (constexpr auto member : std::define_static_array(
                        std::meta::nonstatic_data_members_of(
                            ^^Strings, std::meta::access_context::current()))) {
        this->[:member:].check(val);
      }
    }
  };

  // Expose the generated type without constructing hardware or needing frets.
  return std::type_identity<StringMethod>{};
}

using StringMembers = decltype(StringsClass())::type;

} // namespace bh

#endif
