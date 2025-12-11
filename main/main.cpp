#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h" 

#define LGFX_USE_V1
#include <LovyanGFX.hpp>

/* Wiring */
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
            cfg.freq_write = 40000000; // 40MHz
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
LGFX_Sprite canvas(&lcd);           // actual blank canvas
LGFX_Sprite cursorSprite(&lcd);     // cursor sprite

adc_oneshot_unit_handle_t adc1_handle;

void setup_inputs() {
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT,
        .clk_src = ADC_RTC_CLK_SRC_DEFAULT, 
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &adc1_handle));

    adc_oneshot_chan_cfg_t config = { .atten = ADC_ATTEN_DB_12, .bitwidth = ADC_BITWIDTH_DEFAULT };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, JOY_X_CHAN, &config));
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, JOY_Y_CHAN, &config));

    gpio_reset_pin((gpio_num_t)BTN_DRAW_PIN);
    gpio_set_direction((gpio_num_t)BTN_DRAW_PIN, GPIO_MODE_INPUT);
    gpio_set_pull_mode((gpio_num_t)BTN_DRAW_PIN, GPIO_PULLUP_ONLY);
}

void drawCursorAt(int x, int y) {
    /* crop background from canvas into cursor sprite */
    cursorSprite.pushImage(-x + 18, -y + 18, 480, 320, (uint16_t*)canvas.getBuffer());
    
    /* draw crosshair on top */
    cursorSprite.drawFastHLine(13, 18, 11, TFT_RED); // Horizontal
    cursorSprite.drawFastVLine(18, 13, 11, TFT_RED); // Vertical

    /* push sprite to the screen */
    cursorSprite.pushSprite(x - 18, y - 18);
}

void restoreBackgroundAt(int x, int y) {
    /* crop clean background from canvas */
    cursorSprite.pushImage(-x + 18, -y + 18, 480, 320, (uint16_t*)canvas.getBuffer());
    
    /* push immediately to screen */
    cursorSprite.pushSprite(x - 18, y - 18);
}

extern "C" void app_main(void)
{
    if (!lcd.init()) return;
    setup_inputs();
    lcd.setRotation(1); 

    /* setup canvas */
    canvas.setColorDepth(16);
    canvas.setPsram(true); 
    canvas.createSprite(480, 320);
    canvas.fillScreen(TFT_WHITE);
    
    /* setup cursor */
    cursorSprite.setColorDepth(16);
    cursorSprite.createSprite(36, 36);

    float cursor_x = 240.0;
    float cursor_y = 160.0;
    int prev_x = 240;
    int prev_y = 160;

    canvas.pushSprite(0, 0);

    printf("Loop Start (Max FPS Mode)\n");

    while (1) {
        /* read inputs */
        int raw_x, raw_y;
        ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, JOY_X_CHAN, &raw_x));
        ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, JOY_Y_CHAN, &raw_y));
        int btn_state = gpio_get_level((gpio_num_t)BTN_DRAW_PIN);

        /* calc movement */
        float speed = 2.5; 
        
        float move_x = 0, move_y = 0;

        if (raw_x > 2500) move_x = speed;       
        else if (raw_x < 1500) move_x = -speed; 

        if (raw_y > 2500) move_y = speed;       
        else if (raw_y < 1500) move_y = -speed; 

        cursor_x += move_x;
        cursor_y += move_y;

        /* clamp cursor between screen borders */
        if (cursor_x < 18) cursor_x = 18;
        if (cursor_x > 461) cursor_x = 461;
        if (cursor_y < 18) cursor_y = 18;
        if (cursor_y > 301) cursor_y = 301;

        int curr_ix = (int)cursor_x;
        int curr_iy = (int)cursor_y;

        /* screen update logic */
        bool moved = (curr_ix != prev_x || curr_iy != prev_y);
        bool drawing = (btn_state == 0);

        if (moved || drawing) {
            if (moved) restoreBackgroundAt(prev_x, prev_y);
            
            if (drawing) canvas.fillCircle(curr_ix, curr_iy, 4, TFT_BLACK);
            
            drawCursorAt(curr_ix, curr_iy);
            
            prev_x = curr_ix;
            prev_y = curr_iy;
        }

        vTaskDelay(1); 
    }
}
