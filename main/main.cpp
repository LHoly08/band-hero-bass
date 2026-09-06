#include <chrono>
#include <thread>

#include "States.hpp"
#include "esp_log.h"

#include "driver/gpio.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

extern "C" void app_main() {
  volatile bh::State state{bh::State::Config};

  vTaskDelay(pdMS_TO_TICKS(1000));

infinite_loop:

  switch (state) {
  case bh::State::Config: {
    break;
  }
  case bh::State::PlayingHub: {
    break;
  }
  case bh::State::PlayingSerial: {
    break;
  }
  }

  goto infinite_loop;
}
