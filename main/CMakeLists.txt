idf_component_register(
    SRCS
     "main.cpp"
    INCLUDE_DIRS 
      "../include"
    REQUIRES 
        nvs_flash
        esp_wifi
        esp_netif
        freertos
        bt
        esp_driver_gpio
        esp_driver_gptimer
        driver
)

target_compile_features(${COMPONENT_LIB} PUBLIC cxx_std_23)
