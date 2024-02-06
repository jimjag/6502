/**
 * MIT License
 * Copyright (c) 2021-2024 Jim Jagielski
 */
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

// Orig version V. Hunter Adams / Cornell

// VGA graphics library
#include "vga_core.h"
#include "ps2_keyboard.h"
#include "pico_synth_ex.h"
#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/dma.h"
#include "pico/multicore.h"

// Some globals for storing timer information
volatile int time_accum = 12;
int time_accum_old = 0;
char timetext[40];

// Timer interrupt
bool repeating_timer_callback(struct repeating_timer *t) {
    time_accum -= 1;
    return true;
}

void core1_main() {
    initSOUND();
    startup_chord();
    while (true) {
        soundTask();
        //tight_loop_contents();
    }
}

int main() {
    set_sys_clock_khz(250000, true);
    // Initialize stdio
    stdio_init_all();

    // start core 1 threads
    //multicore_reset_core1();
    multicore_launch_core1(&core1_main);

    // Initialize the VGA screen
    initVGA();
    initPS2();
    setFont(0);
    // circle radii
    short circle_r = 0;
    short circle_x = 320;

    // color chooser
    char color_index = 0;

    // position of the disc primitive
    short disc_x = 0;
    // position of the box primitive
    short box_x = 0;
    // position of vertical line primitive
    short Vline_x = 350;
    // position of horizontal line primitive
    short Hline_y = 250;
    // Draw some filled rectangles

    drawFilledRect(64, 0, 176, 50, BLUE); // blue box
    drawFilledRect(250, 0, 176, 50, RED); // red box
    drawFilledRect(435, 0, 176, 50, GREEN); // green box

    // Write some text
    setTextColor2(WHITE, BLUE);
    setCursor(65, 0);
    setTextSize(1);
    drawString("Raspberry Pi Pico");
    setCursor(65, 16);
    drawString("Graphics demo/HA based");
    setCursor(65, 32);
    drawString("JJ65C02");
    setCursor(250, 4);
    setTextSize(2);
    setTextColor2(WHITE, RED);
    drawString("Countdown:");

    // Setup a 1Hz timer
    struct repeating_timer timer;
    add_repeating_timer_ms(-999, repeating_timer_callback, NULL, &timer);

    while (true) {
        // Modify the color chooser
        if (color_index++ == 15) color_index = 0;

        // A row of filled circles
        drawFilledCircle(disc_x, 100, 20, color_index);
        disc_x += 35;
        if (disc_x > 640) disc_x = 0;

        // Concentric empty circles
        drawCircle(circle_x, 200, circle_r, color_index);
        if (circle_r++ > 95) circle_r = 0;
        circle_x += 2;
        if (circle_x > 350) circle_x = 320;

        // A series of rectangles
        drawRect(10, 300, box_x, box_x, color_index);
        box_x += 5;
        if (box_x > 195) box_x = 10;

        // Random lines
        drawLine(210 + (rand() & 0x7f), 350 + (rand() & 0x7f), 210 + (rand() & 0x7f),
                 350 + (rand() & 0x7f), color_index);

        // Vertical lines
        drawVLine(Vline_x, 300, (Vline_x >> 2), color_index);
        Vline_x += 2;
        if (Vline_x > 620) Vline_x = 350;

        // Horizontal lines
        drawHLine(400, Hline_y, 150, color_index);
        Hline_y += 2;
        if (Hline_y > 400) Hline_y = 240;

        // Timing text
        if (time_accum != time_accum_old) {
            time_accum_old = time_accum;
            sprintf(timetext, "%02d", time_accum);
            setCursor(440, 4);
            setTextSize(2);
            setTextColor2(WHITE, GREEN);
            drawString(timetext);
        }

        // A brief nap
        sleep_ms(10);
        if (time_accum < 0) break;
    }
    setTextColor2(WHITE, BLACK);
    sleep_ms(1000);
    vgaScroll(0);
    sleep_ms(1000);
    termScroll(1);
    sleep_ms(1000);
    vgaScroll(64);
    sleep_ms(1000);
    enableSmoothScroll(true);
    vgaScroll(64);
    sleep_ms(1000);
    vgaFillScreen(BLUE);
    sleep_ms(1000);
    vgaFillScreen(BLACK);
    char video_buffer[32];
    setTextColor2(WHITE, BLACK);
    setTextSize(1);
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            drawFilledRect(i * 110 + 20, 100 + j * 70, 60, 60, i + 4 * j);
            setCursor(i * 110 + 20, 100 + j * 70);
            sprintf(video_buffer, "%2d", i + 4 * j);
            drawString(video_buffer);
        }
    }
    // first row of colors
    setCursor(0 * 110 + 20, 150 + 0 * 70);
    drawString("Black");
    setCursor(1 * 110 + 20, 150 + 0 * 70);
    drawString("Red");
    setCursor(2 * 110 + 20, 150 + 0 * 70);
    drawString("Green");
    setCursor(3 * 110 + 20, 150 + 0 * 70);
    drawString("Yellow");
    // second row of colors
    setCursor(0 * 110 + 20, 150 + 1 * 70);
    drawString("Blue");
    setCursor(1 * 110 + 20, 150 + 1 * 70);
    drawString("Magenta");
    setCursor(2 * 110 + 20, 150 + 1 * 70);
    drawString("Cyan");
    setCursor(3 * 110 + 20, 150 + 1 * 70);
    drawString("Light Grey");
    // third row of colors
    setCursor(0 * 110 + 20, 150 + 2 * 70);
    drawString("Grey");
    setCursor(1 * 110 + 20, 150 + 2 * 70);
    drawString("Light Red");
    setCursor(2 * 110 + 20, 150 + 2 * 70);
    drawString("Light Green");
    setCursor(3 * 110 + 20, 150 + 2 * 70);
    drawString("Light Yellow");
    // fourth row of colors
    setCursor(0 * 110 + 20, 150 + 3 * 70);
    drawString("Light Blue");
    setCursor(1 * 110 + 20, 150 + 3 * 70);
    drawString("Light Magenta");
    setCursor(2 * 110 + 20, 150 + 3 * 70);
    drawString("Light Cyan");
    setCursor(3 * 110 + 20, 150 + 3 * 70);
    drawString("White");
    setFont(1);
    setCursor(0, 460);
    drawString("1234567890ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz!?#%*&@0123456789");
    setFont(2);
    setCursor(0, 440);
    drawString("1234567890ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz!?#%*&@0123456789");
    setFont(3);
    setCursor(0, 420);
    drawString("1234567890ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz!?#%*&@0123456789");
    setFont(0);
    sleep_ms(1000);

    char hex[40];
    setTxtCursor(60, 20);
    printString("PS2 test");
    clearPS2();
    setTextColor2(GREEN, BLACK);
    while (true) {
        char c = ps2GetChar(false);
        if (c == '\b') break;
        if (c) {
            setTxtCursor(60, 24);
            printChar(c);
            sprintf(hex, "0x%02x", c);
            setTxtCursor(70, 24);
            printString(hex);
        }
    }
    vgaFillScreen(BLACK);
    setTextColor(RED);
    printString("\x1b[Z1;10;10;500;5Z");
    setTextColor(BLUE);
    printString("\x1b[Z2;30;30;40;45Z");
    setTextColor(GREEN);
    printString("\x1b[Z3;50;50;75;155Z");
    setTextColor(CYAN);
    printString("\x1b[Z4;100;100;55Z");
    setTextColor(YELLOW);
    printString("\x1b[Z5;200;200;55Z");
    setTxtCursor(0, 20);
    setTextColor2(WHITE, BLACK);
    writeChar('a');
    enableCurs(true);
    writeChar('b');
    while (true) {
        unsigned char c = ps2GetChar(true);
        if (c == 'Q') break;
    }
    enableCurs(false);
    vgaFillScreen(BLUE);
#define FOO_LEN 7
    unsigned char foo[] = {
            0xff,0xff,0xff,0xff,0xff,0xff,0x0f,0x0f,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
            0xff,0xff,0xff,0xff,0x08,0x08,0x08,0x08,0x08,0x08,0xff,0xff,0xff,0xff,0xff,0xff,
            0xff,0xff,0x08,0x08,0x08,0x08,0x07,0x07,0x08,0x08,0x08,0x08,0x08,0xff,0xff,0xff,
            0xff,0x08,0x08,0x01,0x08,0x07,0x07,0x07,0x07,0x08,0x08,0x08,0x08,0x08,0xff,0xff,
            0x03,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x03,0xff,
            0xff,0xff,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0xff,0xff,0xff,
            0xff,0xff,0xff,0xff,0xff,0x0f,0x00,0x0f,0x00,0x0f,0xff,0xff,0xff,0xff,0xff,0xff
    };
    extern sprite_t *sprites[];
    int height = FOO_LEN;
    int width = SPRITE16_WIDTH;
    for (int sn = 1; sn <=2; sn++) {
        loadSprite(sn, width, height, foo);
    }

    int y = 2;
    int x0 = 600;
    int y0 = 15;
    for (int i = 10; i < 400; i++) {
        bool changed = false;
        drawSprite(i, y, 1, true);
        drawSprite(x0, y0, 2, true);
        y++;
        x0--;
        y0++;
        multicore_fifo_push_blocking( i%2 ? 'q' : '7');
        if (i > 250 && !changed) {
            multicore_fifo_push_blocking('$');
            changed = true;
        }
        sleep_ms(25);
    }
    y = 5;
    for (int i = 0; i > -16; i--) {
        drawSprite(i, y, 1, false);
        y += 17;
    }
    y = 5;
    for (int i = 624; i < 641; i++) {
        drawSprite(i, y, 1, false);
        y += 17;
    }
    while (true) {
        unsigned char c = ps2GetChar(false);
        if (c == 'Q') break;
        multicore_fifo_push_blocking((uint32_t)c);
    }
}
