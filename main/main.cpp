#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"

#define LGFX_USE_V1
#include <LovyanGFX.hpp>

#define ADC_UNIT       ADC_UNIT_1
#define JOY_X_CHAN     ADC_CHANNEL_3
#define JOY_Y_CHAN     ADC_CHANNEL_4
#define BTN_DRAW_PIN   15

class LGFX : public lgfx::LGFX_Device {
    lgfx::Panel_ILI9486 _panel_instance;
    lgfx::Bus_SPI       _bus_instance;

public:
    LGFX(void) {
        {
            auto cfg = _bus_instance.config();
            cfg.spi_host = SPI2_HOST;
            cfg.spi_mode = 0;
            cfg.freq_write = 20000000;
            cfg.freq_read  = 16000000;
            cfg.use_lock   = true;
            cfg.dma_channel = 1;
            cfg.pin_sclk = 12; 
            cfg.pin_mosi = 11; 
            cfg.pin_miso = -1; 
            cfg.pin_dc   = 10;
            _bus_instance.config(cfg);
            _panel_instance.setBus(&_bus_instance);
        }

        {
            auto cfg = _panel_instance.config();
            cfg.pin_cs   = 9;
            cfg.pin_rst  = 46;
            cfg.panel_width  = 320;
            cfg.panel_height = 480;
            cfg.readable = false;
            cfg.dlen_16bit = false; 
            _panel_instance.config(cfg);
        }
        setPanel(&_panel_instance);
    }
};

LGFX lcd;

/* Global adc handle */
adc_oneshot_unit_handle_t adc1_handle;

void setup_inputs() {
    /* ADC init */
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT,
        .clk_src = ADC_RTC_CLK_SRC_DEFAULT, 
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &adc1_handle));

    /* Config adc channels */
    adc_oneshot_chan_cfg_t config = {
        .atten = ADC_ATTEN_DB_12,         // 0-3.3V range
        .bitwidth = ADC_BITWIDTH_DEFAULT, // Typically 12-bit
    };
    
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, JOY_X_CHAN, &config));
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, JOY_Y_CHAN, &config));

    /* Config button */
    gpio_reset_pin((gpio_num_t)BTN_DRAW_PIN);
    gpio_set_direction((gpio_num_t)BTN_DRAW_PIN, GPIO_MODE_INPUT);
    gpio_set_pull_mode((gpio_num_t)BTN_DRAW_PIN, GPIO_PULLUP_ONLY);
}

extern "C" void app_main(void)
{
    /* Init hardware */
    if (!lcd.init()) {
        printf("LCD Init failed.\n");
        return;
    }
    
    setup_inputs();
    
    lcd.setRotation(1); 
    lcd.fillScreen(TFT_WHITE);
    
    float cursor_x = 240.0;
    float cursor_y = 160.0;

    printf("Start Drawing Loop...\n");

    while (1) {
        /* Read inputs */
        int raw_x, raw_y;
        ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, JOY_X_CHAN, &raw_x));
        ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, JOY_Y_CHAN, &raw_y));
        
        int btn_state = gpio_get_level((gpio_num_t)BTN_DRAW_PIN);

        /* Calc movement */
        float move_x = 0;
        float move_y = 0;

        /* 12-bit adc threshold */
        if (raw_x > 2500) move_x = 3.0;      
        else if (raw_x < 1500) move_x = -3.0; 

        /* Invert y */
        if (raw_y > 2500) move_y = 3.0;       
        else if (raw_y < 1500) move_y = -3.0; 

        cursor_x += move_x;
        cursor_y += move_y;

        /* Clamp to screen edges */
        if (cursor_x < 0) cursor_x = 0;
        if (cursor_x > 479) cursor_x = 479;
        if (cursor_y < 0) cursor_y = 0;
        if (cursor_y > 319) cursor_y = 319;

        /* Drawing logic */
        if (btn_state == 0) {
            /* Button pressed, draw black */
            lcd.fillCircle((int)cursor_x, (int)cursor_y, 4, TFT_BLACK);
        } else {
            /* Button not pressed, draw red */
            lcd.drawPixel((int)cursor_x, (int)cursor_y, TFT_RED);
        }

        vTaskDelay(10 / portTICK_PERIOD_MS); 
    }
}
