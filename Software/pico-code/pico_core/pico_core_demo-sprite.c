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
 *
 * RESOURCES USED
 *  - VGA:
 *  -   PIO state machines 0, 1, and 2 on PIO instance 0
 *  -   DMA channels 0, 1, 2, and 3
 *  -   153.6 kBytes of RAM (for pixel color data)
 *  - PS2:
 *  -   PIO state machine 0 on PIO instance 1
 *
 */

// Orig version V. Hunter Adams / Cornell

// VGA graphics library
#include "vga_core.h"
#include "ps2_keyboard.h"
#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/dma.h"

// Some globals for storing timer information
volatile int time_accum = 15;
int time_accum_old = 0;
char timetext[40];

// Timer interrupt
bool repeating_timer_callback(struct repeating_timer *t) {
    time_accum -= 1;
    return true;
}

int main() {
    set_sys_clock_khz(250000, true);
    // Initialize stdio
    stdio_init_all();

    // Initialize the VGA screen
    initVGA();
    initPS2();
    setFont(0);
    clearScreen();
    vgaFillScreen(BLUE);
    sleep_ms(2100);
    char foo[] = {
            0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
            0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x02,0x02,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
            0xff,0xff,0x02,0xff,0xff,0xff,0xff,0x01,0x01,0xff,0xff,0xff,0xff,0x02,0xff,0xff,
            0xff,0xff,0xff,0xff,0xff,0x01,0x01,0x01,0x01,0x01,0x01,0xff,0xff,0xff,0xff,0xff,
            0xff,0xff,0xff,0x0b,0x0b,0x0b,0x0b,0x0b,0x0b,0x0b,0x0b,0x0b,0x0b,0xff,0xff,0xff,
            0xff,0xff,0x04,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x04,0xff,0xff,
            0xff,0xff,0x0a,0x0a,0x0a,0x0a,0x0a,0xff,0xff,0x0a,0x0a,0x0a,0x0a,0x0a,0xff,0xff,
            0xff,0x09,0x09,0x09,0x09,0x09,0x09,0xff,0xff,0x09,0x09,0x09,0x09,0x09,0x09,0xff,
            0xff,0x08,0x08,0x08,0x08,0x08,0xff,0xff,0xff,0xff,0x08,0x08,0x08,0x08,0x08,0xff,
            0xff,0x07,0x07,0x07,0x07,0x07,0x02,0x02,0x02,0x02,0x07,0x07,0x07,0x07,0x07,0xff,
            0xff,0x06,0x06,0x06,0x06,0x06,0x06,0x06,0x06,0x06,0x06,0x06,0x06,0x06,0x06,0xff,
            0xff,0x05,0x05,0x05,0x05,0x05,0x05,0x02,0x02,0x05,0x05,0x05,0x05,0x05,0x05,0xff,
            0xff,0xff,0x04,0x04,0x04,0x04,0x04,0x04,0x04,0x04,0x04,0x04,0x04,0x04,0xff,0xff,
            0xff,0xff,0xff,0x03,0x03,0x03,0x03,0x03,0x03,0x03,0x03,0x03,0x03,0xff,0xff,0xff,
            0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
            0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff
    };
    unsigned char cx;
    unsigned char sdata[SPRITESIZE * SPRITESIZE];
    extern sprite_t *sprites[];
    sprite_t *n = malloc(sizeof(sprite_t));
    for (int i = 0; i < sizeof(sdata); i++) {
        sdata[i] = foo[i];
    }
    // NOW CREATE bitmap, mask, etc...
    for (int i = 0; i < SPRITESIZE; i++) {
        int64_t mask = 0;
        int64_t bitmap = 0;
        for (int j = 0; j < SPRITESIZE; j++) {
            mask<<=4;
            bitmap<<=4;
            cx = sdata[j + (i * SPRITESIZE)];
            if (cx == 0xff) { // transparent
                mask |= 0b00001111;
            }
            bitmap |= (0b00001111 & cx);
        }
        n->bitmap[i] = bitmap;
        n->mask[i] = mask;
    }
    n->bgValid = false;
    sprites[1] = n;

    int y = 10;
    for (int i = 10; i < 500; i+=2) {
        drawSprite(i,y,1);
        y +=1;
        sleep_ms(50);
    }
}
