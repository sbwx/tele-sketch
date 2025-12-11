idf_component_register(SRCS "main.cpp"
                       INCLUDE_DIRS "." 
                       REQUIRES LovyanGFX driver esp_adc mqtt nvs_flash)
