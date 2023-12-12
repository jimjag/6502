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

int main() {
    set_sys_clock_khz(250000, true);
    // Initialize stdio
    stdio_init_all();

    // Initialize the VGA screen and PS/2 interface
    initVGA();
    initPS2();
    clearPS2();
    setFont(0);
    setTextColor2(WHITE, BLUE);
    setTxtCursor(0, 0);
    vgaFillScreen(BLUE);
    setCursor(65, 0);
    setTextSize(2);
    drawString("Pi Pico Enabled");
    setTextSize(1);
    setTxtCursor(0, 3);
    enableCurs(true);
    //clearPS2();
    while (true) {
        conInTask();  // Look for incoming data
        ps2Task(false);  // And send out any PS/2 data
        //tight_loop_contents();
    }
}