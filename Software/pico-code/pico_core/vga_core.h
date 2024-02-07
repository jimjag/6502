/**
 *
 * HARDWARE CONNECTIONS
 *  - GPIO 17 ---> VGA Hsync
 *  - GPIO 18 ---> VGA Vsync
 *  - GPIO 19 ---> 470 ohm resistor ---> VGA Red
 *  - GPIO 20 ---> 470 ohm resistor ---> VGA Green
 *  - GPIO 21 ---> 470 ohm resistor ---> VGA Blue
 *  - GPIO 22 ---> 1k ohm resistor ---> VGA Intensity (bright)
 *  - GPIO 15 ---> PS2 Data pin
 *  - GPIO 16 ---> PS2 Clock pin
 *  - RP2040 GND ---> VGA GND
 *  - GPIO 0-6 ---> 7 bit PS/2 Data to VIA
 *  - GPIO 7-14 ---> 8 Bit Data In from VIA
 *  - GPIO 26 ---> Data Ready
 *  - GPIO 27 ---> IRQ/Handshake to VIA for PS/2
 *  - GPIO 28 ---> audio/sound
 *
 * RESOURCES USED
 *  CORE 0
 *  - VGA:
 *  -   PIO state machines 0, 1, and 2 on PIO instance 0
 *  -   DMA channels 0, 1, 2, and 3
 *  -   IRQ 0, 1
 *  -   153.6 kBytes of RAM (for pixel color data)
 *  - PS2:
 *  -   PIO state machine 0 on PIO instance 1
 *  -   IRQ 1
 *  - MEMIN:
 *  -   PIO state machine 1 on PIO instance 1
 *  -   IRQ 0
 *
 * CORE 1
 * - SND:
 * -   PWM
 */

#include <stdint.h>
#include <stddef.h>
#include "hardware/pio.h"
#include "hardware/gpio.h"
#include "pico/multicore.h"
#include "pico_synth_ex.h"
#include "pico/platform.h"

// TODO: Eventually support resolutions > 640x480

// VGA timing constants
#define H_ACTIVE 655        // (active + frontporch - 1) - one cycle delay for mov
#define V_ACTIVE 479        // (active - 1)
#define SCANLINE_ACTIVE 319 // (horizontal length)/2 - 1
// #define SCANLINE_ACTIVE 639 // change to this if 1 pixel/byte

// Screen width/height/freq
#define SCREENWIDTH 640
#define SCREENHEIGHT 480
#define PIXFREQ   25000000.0f
#define SCANFREQ 125000000.0f

enum vga_pins {HSYNC=17, VSYNC, RED_PIN, GREEN_PIN, BLUE_PIN, I_PIN};

// We can only produce 16 (4-bit) colors, so let's give them readable names - usable in main()
enum colors {BLACK, RED, GREEN, YELLOW, BLUE, MAGENTA, CYAN, LIGHT_GREY,
            GREY, LIGHT_RED, LIGHT_GREEN, LIGHT_YELLOW, LIGHT_BLUE, LIGHT_MAGENTA, LIGHT_CYAN, WHITE,
            TRANSPARENT=0xFF};

// Bit masks for drawPixel routine - RGBIRGBI
#define TOPMASK 0b00001111
#define BOTTOMMASK 0b11110000
#define ESC 0x1b

#define LSN64 0x0f
#define MSN64 0xf000000000000000

// For drawLine
#define swap(a, b) do { int t = a; a = b; b = t; } while (false)

// For writing text
#define tabspace 4 // number of spaces for a tab

// For accessing the font library
#define pgm_read_byte(addr) (*(const unsigned char *)(addr))

// Sprite
#define SPRITE16_WIDTH 16  // in pixels
#define SPRITE32_WIDTH 32  // "" ""
typedef struct {
    uint64_t *bitmap[2][2];  // [# of 64bit values][odd/even]
    uint64_t *mask[2][2];
    uint64_t *bgrnd[2];
    short x;
    short y;
    unsigned char height;
    unsigned char width;
    bool bgValid;
} sprite_t;

// GPIO pins to VIA chip
enum data_pins {DATA0=7, DATA1, DATA2, DATA3, DATA4, DATA5, DATA6, DATA7, DREADY=26};

// VGA Core Functions
void initVGA(void);
void conInTask(void);

// Graphics functions
void drawPixel(int x, int y, char color);
void drawVLine(int x, int y, int h, char color);
void drawHLine(int x, int y, int w, char color);
void drawLine(int x0, int y0, int x1, int y1, char color);
void drawRect(int x, int y, int w, int h, char color);
void drawCircle(int x0, int y0, int r, char color);
// void drawCircleHelper( int x0, int y0, int r, unsigned char cornername, char color);
void drawFilledCircle(int x0, int y0, int r, char color);
// void fillCircleHelper(int x0, int y0, int r, unsigned char cornername, int delta, char color);
void drawRoundRect(int x, int y, int w, int h, int r, char color);
void drawFilledRoundRect(int x, int y, int w, int h, int r, char color);
void drawFilledRect(int x, int y, int w, int h, char color);
void drawChar(int x, int y, unsigned char chrx, char color, char bg, unsigned char size);
void setCursor(int x, int y);
void setTextColor(char c);
void setTextColor2(char c, char bg);
void setTextSize(unsigned char s);
void setTextWrap(bool w);
void setCr2Crlf(bool w);
void setLf2Crlf(bool w);
void setFont(char n);
// void tft_write(unsigned char c);
void drawString(unsigned char* str);

void vgaFillScreen(uint16_t color);

void dma_memset(void *dest, uint8_t val, size_t num);
void dma_memcpy(void *dest, void *src, size_t num);

void writeChar(unsigned char chrx); // write the interpreted character
void printChar(unsigned char c);     // auto-decide based on graphics/text mode
void vgaScroll (int scanlines);
void termScroll (int rows);
char safeColor(char c);
void setTxtCursor(int x, int y);
void printString(char* str);
bool conInHaveChar(void);
unsigned char conInGetChar(void);
void clearScreen(void);
bool enableCurs(bool flag);
void enableSmoothScroll(bool flag);
// void enableRaw(bool flag);

void drawSprite(int x, int y, uint sn, bool erase);
void loadSprite(uint sn, short width, short height, unsigned char *sdata);
void eraseSprite(uint sn);
