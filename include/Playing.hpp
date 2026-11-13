#ifndef BH_PLAY
#define BH_PLAY

#include <cstdint>
#include <cstdio>

#include "PeerConfig.hpp"
#include "Strings.hpp"
#include "Utils.hpp"

#include "esp_err.h"
#include "esp_now.h"
#include "esp_wifi.h"
#include "freertos/idf_additions.h"
#include "freertos/task.h"

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
  bh::PeerConfig &peerConfig;
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

  bh::StringMembers m_strings;
};

template <bh::State S>
  requires Playing<S>
Play<S> *Play<S>::instance = nullptr;

template <bh::State S>
  requires Playing<S>
Play<S>::Play(NeededReferences<S> neededReferences,
              const bh::FretValues &fretValues) noexcept
    : m_references(neededReferences), m_strings(fretValues) {

  ESP_ERROR_CHECK(::gpio_install_isr_service(ESP_INTR_FLAG_IRAM));

  if constexpr (S == bh::State::PlayingHub) {
    ::wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(::esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(::esp_wifi_set_mode(::WIFI_MODE_STA));
    ESP_ERROR_CHECK(::esp_wifi_start());

    ESP_ERROR_CHECK(::esp_now_init());
    m_references.peerConfig.registerPeer();
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

    if constexpr (S == bh::State::PlayingHub) {
      m_references.peerConfig.poll(true);
    }

    std::uint32_t val = m_note;

    m_strings.play(val);

    if (val == m_note) {
      continue;
    }

    if constexpr (S == bh::State::PlayingHub) {

      (void)::esp_now_send(m_references.peerConfig.address().data(),
                           reinterpret_cast<const std::uint8_t *>(&val),
                           sizeof(val));

    } else if constexpr (S == bh::State::PlayingSerial) {

      std::fwrite(&val, 1, sizeof(val), stdout);
      std::fflush(stdout);
    }

    m_note = val;
  }
}

} // namespace bh

#endif
