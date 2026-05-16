#include <cstdint>

#include <iostream>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void setup() {}
void loop() {
  static std::uint64_t i{};
  std::cout << "Hello: " << i++ << '\n';
}

extern "C" void app_main() {
  setup();

  while (1) {
    loop();
  }
}
