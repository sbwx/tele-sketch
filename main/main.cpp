#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

#define LGFX_USE_V1
#include <LovyanGFX.hpp>

// --- LGFX Configuration (Your Stable Config) ---
class LGFX : public lgfx::LGFX_Device {
    lgfx::Panel_ILI9486 _panel_instance;
    lgfx::Bus_SPI       _bus_instance;

public:
    LGFX(void) {
        {
            auto cfg = _bus_instance.config();

            cfg.spi_host = SPI2_HOST;
            cfg.spi_mode = 0;
            cfg.freq_write = 10000000; // 10MHz Safe Speed
            cfg.freq_read  = 16000000;
            cfg.use_lock   = true;
            cfg.dma_channel = 0; // DMA Disabled (Safety first)

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
            
            // NOTE: If colors are wrong (e.g. Blue background instead of White),
            // change this to 'false'.
            cfg.dlen_16bit = false; 

            _panel_instance.config(cfg);
        }
        setPanel(&_panel_instance);
    }
};

LGFX lcd;

// --- Main Application ---
extern "C" void app_main(void)
{
    // 1. Initialize the Hardware
    if (!lcd.init()) {
        printf("LCD Init failed.\n");
        return;
    }
    
    // 2. Setup Screen
    lcd.setRotation(1); // Landscape mode
    
    // 3. Draw Background (White)
    printf("Drawing White Background...\n");
    lcd.fillScreen(TFT_WHITE);
    
    // 4. Draw Circle (Empty, Unfilled, Black)
    // x=240, y=160, radius=50
    printf("Drawing Black Circle...\n");
    lcd.setColor(TFT_BLACK);
    lcd.drawCircle(240, 160, 50); 

    printf("Drawing Complete. Entering idle loop.\n");

    // 5. Idle Loop (Prevents Watchdog Reset)
    while (1) {
        vTaskDelay(1000 / portTICK_PERIOD_MS); 
    }
}
