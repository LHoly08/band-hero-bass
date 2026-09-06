#ifndef BH_PLAY
#define BH_PLAY

#include <cstdint>
#include <cstdio>

#include "States.hpp"
#include "Strings.hpp"
#include "esp_now.h"
#include "esp_wifi.h"
#include "freertos/idf_additions.h"
#include "freertos/task.h"
#include "nvs_flash.h"

#include "driver/gpio.h"
#include "driver/gptimer.h"

namespace bh {

static constexpr std::uint8_t peerAddress[6] = {0x00, 0x11, 0x22,
                                                0x33, 0x44, 0x55};

enum class Type : std::uint8_t {
  Hub = 0,
  Serial,
};

template <Type T> struct NeededState;

template <> struct NeededState<Type::Hub> {
  static constexpr State state = State::PlayingHub;
};

template <> struct NeededState<Type::Serial> {
  static constexpr State state = State::PlayingSerial;
};

template <Type T> class Play {
public:
  explicit Play(State &state) noexcept;

  ~Play() noexcept;

  inline void loop() noexcept;

private:
  State &m_state;
  gptimer_handle_t timer = NULL;
};

template <Type T> Play<T>::Play(State &state) noexcept : m_state(state) {
  gpio_install_isr_service(0);

  gptimer_config_t cfg;

  cfg.clk_src = GPTIMER_CLK_SRC_DEFAULT;
  cfg.direction = GPTIMER_COUNT_UP;
  cfg.resolution_hz = 1 * 1000 * 1000;

  gptimer_new_timer(&cfg, &timer);
  gptimer_enable(timer);

  if constexpr (T == Type::Hub) {
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_start();

    esp_now_init();
  }
}

template <Type T> Play<T>::~Play() noexcept {

  if constexpr (T == Type::Hub) {

    esp_now_deinit();

    esp_wifi_stop();
    esp_wifi_deinit();
  }

  gptimer_disable(timer);

  gptimer_del_timer(timer);

  gpio_uninstall_isr_service();
}

template <Type T> inline void Play<T>::loop() noexcept {

  volatile bool played{};

  String<GPIO_NUM_1, 0> string1(played);
  String<GPIO_NUM_2, 1> string2(played);
  String<GPIO_NUM_3, 2> string3(played);
  String<GPIO_NUM_4, 3> string4(played);

  std::uint32_t val{};

  gptimer_start(timer);
  gptimer_set_raw_count(timer, 0);

  while (m_state == NeededState<T>::state) {

    if (played) {
      string1.check(val);
      string2.check(val);
      string3.check(val);
      string4.check(val);
    }

    {
      std::uint64_t time{};
      gptimer_get_captured_count(timer, &time);

      if (time >= 1000) {

        if constexpr (T == Type::Hub) {

          esp_err_t status = esp_now_send(
              peerAddress, reinterpret_cast<std::uint8_t *>(val), sizeof(val));

          for (unsigned char i{}; i < 4 && status != ESP_OK; ++i) {

            status =
                esp_now_send(peerAddress, reinterpret_cast<std::uint8_t *>(val),
                             sizeof(val));
          }
        } else if constexpr (T == Type::Serial) {

          std::fwrite(reinterpret_cast<std::uint8_t *>(val), 1, sizeof(val),
                      stdout);
          std::fflush(stdout);
        }

        gptimer_set_raw_count(timer, 0);
      }
    }
  }

  gptimer_stop(timer);
}

} // namespace bh

#endif
