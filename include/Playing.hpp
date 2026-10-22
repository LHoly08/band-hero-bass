#ifndef BH_PLAY
#define BH_PLAY

#include <cstdint>
#include <cstdio>

#include "Strings.hpp"
#include "Utils.hpp"

#include "esp_err.h"
#include "esp_now.h"
#include "esp_wifi.h"
#include "freertos/idf_additions.h"
#include "freertos/task.h"
#include "nvs_flash.h"

#include "driver/gpio.h"

namespace bh {

namespace {

template <bh::State S>
concept Playing = S == bh::State::PlayingHub || S == bh::State::PlayingSerial;

template <bh::State S>
  requires Playing<S>
struct NeededReferences;

template <> struct NeededReferences<bh::State::PlayingSerial> {
  bh::AtomicState &state;
};

template <> struct NeededReferences<bh::State::PlayingHub> {
  bh::AtomicState &state;
  const bh::MAC &macAddress;
};

} // namespace

template <bh::State S>
  requires Playing<S>
class Play {
public:
  explicit Play(NeededReferences<S> neededReferences,
                const bh::FretValues &fretValues) noexcept;

  ~Play() noexcept;

  inline void loop() noexcept;

private:
  static Play<S> *instance;

  NeededReferences<S> m_references;
  std::uint32_t m_note{};

  bh::String<::GPIO_NUM_1, 0> m_string1;
  bh::String<::GPIO_NUM_2, 1> m_string2;
  bh::String<::GPIO_NUM_3, 2> m_string3;
  bh::String<::GPIO_NUM_4, 3> m_string4;
};

template <bh::State S>
  requires Playing<S>
Play<S> *Play<S>::instance = nullptr;

template <bh::State S>
  requires Playing<S>
Play<S>::Play(NeededReferences<S> neededReferences,
              const bh::FretValues &fretValues) noexcept
    : m_references(neededReferences), m_string1(fretValues), m_string2(fretValues),
      m_string3(fretValues), m_string4(fretValues) {

  ESP_ERROR_CHECK(::gpio_install_isr_service(ESP_INTR_FLAG_IRAM));

  if constexpr (S == bh::State::PlayingHub) {
    ::wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(::esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(::esp_wifi_set_mode(::WIFI_MODE_STA));
    ESP_ERROR_CHECK(::esp_wifi_start());

    ESP_ERROR_CHECK(::esp_now_init());
  }
  {
    ::gpio_config_t cfg;

    cfg.pin_bit_mask = 1ULL << bh::STATE_BUTTON_PIN;
    cfg.mode = ::GPIO_MODE_INPUT;
    cfg.pull_up_en = ::GPIO_PULLUP_DISABLE;
    cfg.pull_down_en = ::GPIO_PULLDOWN_ENABLE;
    cfg.intr_type = ::GPIO_INTR_POSEDGE;

    ESP_ERROR_CHECK(::gpio_config(&cfg));
  }

  instance = this;

  ESP_ERROR_CHECK(::gpio_isr_handler_add(
      bh::STATE_BUTTON_PIN, bh::mode_button_pressed, &m_references.state));
}

template <bh::State S>
  requires Playing<S>
Play<S>::~Play() noexcept {
  (void)::esp_now_unregister_recv_cb();
  (void)::esp_now_deinit();

  if constexpr (S == bh::State::PlayingHub) {
    (void)::esp_wifi_stop();
    (void)::esp_wifi_deinit();
  }

  ::gpio_isr_handler_remove(bh::STATE_BUTTON_PIN);
  ::gpio_uninstall_isr_service();

  instance = nullptr;
}

template <bh::State S>
  requires Playing<S>
inline void Play<S>::loop() noexcept {

  while (m_references.state.load(std::memory_order_relaxed) == S) {

    std::uint32_t val = m_note;

    m_string1.check(val);
    m_string2.check(val);
    m_string3.check(val);
    m_string4.check(val);

    if (val == m_note) {
      continue;
    }

    if constexpr (S == bh::State::PlayingHub) {

      ::esp_err_t status = ::esp_now_send(
          m_references.macAddress.data(),
          reinterpret_cast<const std::uint8_t *>(&val), sizeof(val));

      for (unsigned char i{}; i < 4 && status != ESP_OK; ++i) {

        status = ::esp_now_send(m_references.macAddress.data(),
                                reinterpret_cast<const std::uint8_t *>(&val),
                                sizeof(val));
      }
    } else if constexpr (S == bh::State::PlayingSerial) {

      std::fwrite(&val, 1, sizeof(val), stdout);
      std::fflush(stdout);
    }

    m_note = val;
  }
}

} // namespace bh

#endif
