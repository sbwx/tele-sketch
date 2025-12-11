idf_component_register(SRCS "main.cpp"
                       PRIV_REQUIRES spi_flash
                       REQUIRES LovyanGFX esp_adc
                       INCLUDE_DIRS ".")
