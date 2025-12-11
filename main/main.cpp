#include <stdio.h>
#include <math.h>
#include <vector> 
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h" 
#include "esp_timer.h" 

#define LGFX_USE_V1
#include <LovyanGFX.hpp>

/* Wiring Config */
#define ADC_UNIT       ADC_UNIT_1
#define JOY_X_CHAN     ADC_CHANNEL_3
#define JOY_Y_CHAN     ADC_CHANNEL_4

#define BTN_DRAW_PIN   15 // Hold to draw/erase
#define BTN_COLOR_PIN  16 // Press=Toggle Palette, Hold=Color Wheel
#define BTN_UNDO_PIN   17 // Press=UNDO, Hold=CLEAR
#define BTN_TOOL_PIN   18 // Press=Cycle, Hold=Toggle Brush/Eraser

/* Display Config */
class LGFX : public lgfx::LGFX_Device {
    lgfx::Panel_ILI9486 _panel_instance;
    lgfx::Bus_SPI       _bus_instance;
public:
    LGFX(void) {
        {
            auto cfg = _bus_instance.config();
            cfg.spi_host = SPI2_HOST;
            cfg.spi_mode = 0;
            cfg.freq_write = 40000000; 
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
LGFX_Sprite canvas(&lcd);      
LGFX_Sprite cursorSprite(&lcd); 
adc_oneshot_unit_handle_t adc1_handle;

/* Undo setup */
struct Snapshot {
    uint16_t* buffer;
};
std::vector<Snapshot> undoStack;
const int MAX_UNDOS = 20;

void saveSnapshot() {
    uint16_t* newBuf = (uint16_t*)heap_caps_malloc(320 * 480 * 2, MALLOC_CAP_SPIRAM);
    if (newBuf == NULL) return;
    memcpy(newBuf, canvas.getBuffer(), 320 * 480 * 2);
    undoStack.push_back({newBuf});
    if (undoStack.size() > MAX_UNDOS) {
        free(undoStack.front().buffer);
        undoStack.erase(undoStack.begin());
    }
}

void performUndo() {
    if (undoStack.empty()) return;
    Snapshot last = undoStack.back();
    memcpy(canvas.getBuffer(), last.buffer, 320 * 480 * 2);
    free(last.buffer);
    undoStack.pop_back();
    canvas.pushSprite(0, 0);
}

/* Joystick Calibration */
int center_x = 2048; 
int center_y = 2048; 

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

    gpio_config_t btn_cfg = {};
    btn_cfg.intr_type = GPIO_INTR_DISABLE;
    btn_cfg.mode = GPIO_MODE_INPUT;
    btn_cfg.pull_up_en = GPIO_PULLUP_ENABLE;
    btn_cfg.pin_bit_mask = (1ULL << BTN_DRAW_PIN) | 
                           (1ULL << BTN_UNDO_PIN) |
                           (1ULL << BTN_COLOR_PIN) |
                           (1ULL << BTN_TOOL_PIN);
    gpio_config(&btn_cfg);

    printf("Calibrating Joystick... DON'T TOUCH ME!!!\n");
    long sum_x = 0, sum_y = 0;
    for(int i=0; i<50; i++) {
        int r_x, r_y;
        ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, JOY_X_CHAN, &r_x));
        ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, JOY_Y_CHAN, &r_y));
        sum_x += r_x; sum_y += r_y;
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
    center_x = sum_x / 50;
    center_y = sum_y / 50;
    printf("Calibration Complete.\n");
}

/* Colour math */
uint16_t hsv_to_rgb565(float h, float s, float v) {
    float r, g, b;
    int i = (int)(h / 60.0);
    float f = (h / 60.0) - i;
    float p = v * (1.0 - s);
    float q = v * (1.0 - s * f);
    float t = v * (1.0 - s * (1.0 - f));
    
    switch(i % 6) {
        case 0: r = v; g = t; b = p; break;
        case 1: r = q; g = v; b = p; break;
        case 2: r = p; g = v; b = t; break;
        case 3: r = p; g = q; b = v; break;
        case 4: r = t; g = p; b = v; break;
        case 5: r = v; g = p; b = q; break;
        default: r=1; g=1; b=1; break;
    }
    return lgfx::color565((uint8_t)(r*255), (uint8_t)(g*255), (uint8_t)(b*255));
}

/* Graphics State */
int size_index = 1; 
bool is_eraser = false;
uint16_t current_color = TFT_BLACK;
bool is_pastel_mode = false; 

int BRUSH_SIZES[]  = {2, 4, 8};
int ERASER_SIZES[] = {6, 12, 24}; 

int getBrushSize() {
    if (is_eraser) return ERASER_SIZES[size_index];
    else return BRUSH_SIZES[size_index];
}

void drawCursorAt(int x, int y) {
    cursorSprite.pushImage(-x + 18, -y + 18, 480, 320, (uint16_t*)canvas.getBuffer());
    
    uint16_t c;
    if (is_eraser) c = TFT_BLUE; 
    else c = current_color;      
    
    int r = getBrushSize();
    if (r > 2) cursorSprite.drawCircle(18, 18, r/2 + 1, TFT_DARKGREY);
    
    cursorSprite.drawFastHLine(13, 18, 11, TFT_RED);
    cursorSprite.drawFastVLine(18, 13, 11, TFT_RED);
    
    if (!is_eraser) {
        cursorSprite.fillRect(0,0, 6, 6, current_color);
        cursorSprite.fillRect(30,0, 6, 6, current_color);
        cursorSprite.fillRect(0,30, 6, 6, current_color);
        cursorSprite.fillRect(30,30, 6, 6, current_color);
    }
    cursorSprite.pushSprite(x - 18, y - 18);
}

void restoreBackgroundAt(int x, int y) {
    cursorSprite.pushImage(-x + 18, -y + 18, 480, 320, (uint16_t*)canvas.getBuffer());
    cursorSprite.pushSprite(x - 18, y - 18);
}

extern "C" void app_main(void)
{
    if (!lcd.init()) return;
    setup_inputs();
    lcd.setRotation(1); 

    canvas.setColorDepth(16);
    canvas.setPsram(true); 
    canvas.createSprite(480, 320);
    canvas.fillScreen(TFT_WHITE);
    
    cursorSprite.setColorDepth(16);
    cursorSprite.createSprite(36, 36);

    float cursor_x = 240.0, cursor_y = 160.0;
    int prev_x = 240, prev_y = 160;

    saveSnapshot();
    canvas.pushSprite(0, 0);

    /* Input States */
    int last_tool_state = 1; 
    int64_t tool_press_start = 0;
    bool tool_handled = false;

    int last_undo_state = 1;
    int64_t undo_press_start = 0;
    bool undo_handled = false;
    
    bool was_drawing = false;

    const int DEADZONE = 120;
    const float MAX_SPEED = 3.5; 
    const float DIVISOR = 2000.0 / MAX_SPEED;

    while (1) {
        int raw_x, raw_y;
        ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, JOY_X_CHAN, &raw_x));
        ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, JOY_Y_CHAN, &raw_y));
        
        int btn_draw  = gpio_get_level((gpio_num_t)BTN_DRAW_PIN);
        int btn_undo  = gpio_get_level((gpio_num_t)BTN_UNDO_PIN);
        int btn_tool  = gpio_get_level((gpio_num_t)BTN_TOOL_PIN);
        int btn_color = gpio_get_level((gpio_num_t)BTN_COLOR_PIN);

        int64_t now = esp_timer_get_time() / 1000;

        /* Colour selector logic */
        if (btn_color == 0) {
            int64_t press_start = now;
            bool entered_wheel = false;

            // Wait loop while button is held
            while (gpio_get_level((gpio_num_t)BTN_COLOR_PIN) == 0) {
                int64_t held_time = (esp_timer_get_time() / 1000) - press_start;

                if (held_time > 300) { 
                    // LONG HOLD -> Enter Color Wheel
                    entered_wheel = true;

                    // Read Joystick
                    adc_oneshot_read(adc1_handle, JOY_X_CHAN, &raw_x);
                    adc_oneshot_read(adc1_handle, JOY_Y_CHAN, &raw_y);
                    
                    float val_x = (float)raw_x - center_x;
                    float val_y = (float)raw_y - center_y;
                    
                    // Angle = Hue
                    float angle = atan2(val_y, val_x) * 180.0 / M_PI;
                    if (angle < 0) angle += 360.0;
                    
                    // Distance = Intensity
                    float dist = sqrt(val_x*val_x + val_y*val_y);
                    
                    if (dist < 250) {
                        // Center Zone: Black (Dark Mode) or White (Light Mode)
                        current_color = is_pastel_mode ? TFT_WHITE : TFT_BLACK;
                    } else {
                        // Map distance to 0.0 -> 1.0 factor
                        float factor = (dist - 250) / 1750.0;
                        if (factor < 0.2) factor = 0.2;
                        if (factor > 1.0) factor = 1.0;

                        if (is_pastel_mode) {
                            // LIGHT: Saturation changes (Fade to White)
                            current_color = hsv_to_rgb565(angle, factor, 1.0); 
                        } else {
                            // DARK: Value changes (Fade to Black)
                            current_color = hsv_to_rgb565(angle, 1.0, factor);
                        }
                    }
                    
                    if (is_eraser) is_eraser = false;
                    drawCursorAt((int)cursor_x, (int)cursor_y);
                }
                vTaskDelay(10 / portTICK_PERIOD_MS);
            }

            // Button released
            // If never entered wheel, it was a press
            if (!entered_wheel) {
                is_pastel_mode = !is_pastel_mode;
                printf("Palette: %s\n", is_pastel_mode ? "LIGHT" : "DARK");
                // Force redraw to show the user
                drawCursorAt((int)cursor_x, (int)cursor_y);
            }
            continue; // Skip movement processing for this frame
        }


        /* Tool Button */
        if (btn_tool == 0) { 
            if (last_tool_state == 1) {
                tool_press_start = now;
                tool_handled = false;
            } else {
                if (!tool_handled && (now - tool_press_start > 500)) { 
                    is_eraser = !is_eraser; 
                    tool_handled = true;
                    drawCursorAt((int)cursor_x, (int)cursor_y);
                }
            }
        } else { 
            if (last_tool_state == 0 && !tool_handled) {
                size_index++;
                if (size_index > 2) size_index = 0;
            }
        }
        last_tool_state = btn_tool;

        /* Undo/Clear Button */
        if (btn_undo == 0) { 
            if (last_undo_state == 1) {
                undo_press_start = now;
                undo_handled = false;
            } else {
                if (!undo_handled && (now - undo_press_start > 800)) {
                    saveSnapshot(); 
                    canvas.fillScreen(TFT_WHITE);
                    canvas.pushSprite(0, 0);
                    undo_handled = true;
                }
            }
        } else { 
            if (last_undo_state == 0 && !undo_handled) performUndo();
        }
        last_undo_state = btn_undo;

        /* Cursor Movement */
        float val_x = (float)raw_x - center_x;
        float val_y = (float)raw_y - center_y;

        if (fabs(val_x) < DEADZONE) val_x = 0;
        else val_x = val_x / DIVISOR;

        if (fabs(val_y) < DEADZONE) val_y = 0;
        else val_y = val_y / DIVISOR;

        cursor_x += val_x; 
        cursor_y += val_y; 

        if (cursor_x < 18) cursor_x = 18;
        if (cursor_x > 461) cursor_x = 461;
        if (cursor_y < 18) cursor_y = 18;
        if (cursor_y > 301) cursor_y = 301;

        int curr_ix = (int)cursor_x;
        int curr_iy = (int)cursor_y;

        /* Drawing Logic */
        bool moved = (curr_ix != prev_x || curr_iy != prev_y);
        bool drawing = (btn_draw == 0);

        if (drawing && !was_drawing) saveSnapshot();
        was_drawing = drawing;

        if (moved || drawing) {
            if (moved) restoreBackgroundAt(prev_x, prev_y);
            
            if (drawing) {
                uint16_t c = is_eraser ? TFT_WHITE : current_color;
                canvas.fillCircle(curr_ix, curr_iy, getBrushSize(), c);
            }
            
            drawCursorAt(curr_ix, curr_iy);
            prev_x = curr_ix;
            prev_y = curr_iy;
        }

        vTaskDelay(1); 
    }
}
