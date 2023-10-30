#include "ps2_keyboard.pio.h"
#include "ps2_keyboard.h"

#include "hardware/clocks.h"
#include "hardware/pio.h"

static PIO ps2_pio;         // pio0 or pio1
static uint ps2_sm;         // pio state machine index
static uint ps2_data;      // data signal gpio #

void ps2_init(uint pio, uint gpio) {
    // Could move some of the below to ps2_keyboard.pio
    // in '% c-sdk {' as we do with the VGA stuff
    // but this is another way. so show both
    ps2_pio = pio ? pio1 : pio0;
    ps2_data = gpio;
    gpio_init(ps2_data);  // ps2_data is data signal
    gpio_init(ps2_data + 1);  // ps2_data+1 is clock signal
    gpio_pull_up(ps2_data);
    gpio_pull_up(ps2_data + 1);
    ps2_sm = pio_claim_unused_sm(ps2_pio, true);
    uint offset = pio_add_program(ps2_pio, &ps2_keyboard_program);
    pio_sm_set_consecutive_pindirs(ps2_pio, ps2_sm, ps2_data, 2, false);
    pio_sm_config c = ps2_keyboard_program_get_default_config(offset);
    sm_config_set_in_pins(&c, ps2_data);
    sm_config_set_in_shift(&c, true, true, 8);
    sm_config_set_fifo_join(&c, PIO_FIFO_JOIN_RX);
    float div = (float)clock_get_hz(clk_sys) / (8 * 16700);
    sm_config_set_clkdiv(&c, div);
    pio_sm_init(ps2_pio, ps2_sm, offset, &c);
    pio_sm_set_enabled(ps2_pio, ps2_sm, true);
}

// clang-format off

static const char ps2_to_ascii_lower[] = {
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x09,  '`', 0x00,
0x00, 0x00, 0x00, 0x00, 0x00,  'q',  '1', 0x00, 0x00, 0x00,  'z',  's',  'a',  'w',  '2', 0x00,
0x00,  'c',  'x',  'd',  'e',  '4',  '3', 0x00, 0x00,  ' ',  'v',  'f',  't',  'r',  '5', 0x00,
0x00,  'n',  'b',  'h',  'g',  'y',  '6', 0x00, 0x00, 0x00,  'm',  'j',  'u',  '7',  '8', 0x00,
0x00,  ',',  'k',  'i',  'o',  '0',  '9', 0x00, 0x00,  '.',  '/',  'l',  ';',  'p',  '-', 0x00,
0x00, 0x00, '\'', 0x00,  '[',  '=', 0x00, 0x00, 0x00, 0x00, 0x0D,  ']', 0x00, '\\', 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1B, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static const char ps2_to_ascii_upper[] = {
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x09,  '~', 0x00,
0x00, 0x00, 0x00, 0x00, 0x00,  'Q',  '!', 0x00, 0x00, 0x00,  'Z',  'S',  'A',  'W',  '@', 0x00,
0x00,  'C',  'X',  'D',  'E',  '$',  '#', 0x00, 0x00,  ' ',  'V',  'F',  'T',  'R',  '%', 0x00,
0x00,  'N',  'B',  'H',  'G',  'Y',  '^', 0x00, 0x00, 0x00,  'M',  'J',  'U',  '&',  '*', 0x00,
0x00,  '<',  'K',  'I',  'O',  ')',  '(', 0x00, 0x00,  '>',  '?',  'L',  ':',  'P',  '_', 0x00,
0x00, 0x00, 0x22, 0x00,  '{',  '+', 0x00, 0x00, 0x00, 0x00, 0x0D,  '}', 0x00,  '|', 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static const char ps2_to_ascii_cntl[] = {
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x09,  '~', 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x11,  '!', 0x00, 0x00, 0x00, 0x1A, 0x13, 0x01, 0x17,  '@', 0x00,
0x00, 0x03, 0x18, 0x04, 0x05,  '$',  '#', 0x00, 0x00,  ' ', 0x16, 0x06, 0x14, 0x12,  '%', 0x00,
0x00, 0x0E, 0x02, 0x08, 0x07, 0x19,  '^', 0x00, 0x00, 0x00, 0x0D, 0x0A, 0x15,  '&',  '*', 0x00,
0x00,  '<', 0x0B, 0x09, 0x0F,  ')',  '(', 0x00, 0x00,  '>',  '?', 0x0C,  ':', 0x10,  '_', 0x00,
0x00, 0x00, 0x22, 0x00,  '{',  '+', 0x00, 0x00, 0x00, 0x00, 0x0D,  '}', 0x00,  '|', 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static int release; // Flag indicates the release of a key
static int shift;   // Shift indication
static int cntl;    // Control indication
static uint8_t ascii;   // Translated to ASCII


// Return keyboard status
// Returns: 0 for not ready, ASCII code otherwise ready
int __attribute__((noinline)) ps2_ready(void) {
    if (ascii)
        return ascii;
    if (pio_sm_is_rx_fifo_empty(ps2_pio, ps2_sm))
        return 0;
    // pull a scan code from the PIO SM fifo
    // uint8_t code = *((io_rw_8*)&ps2_pio->rxf[ps2_sm] + 3);
    uint8_t code = pio_sm_get(ps2_pio, ps2_sm) >> 24;
    switch (code) {
    case 0xF0:               // key-release code 0xF0
        release = 1;         // release flag
        break;
    case 0x12:               // Left-side shift
    case 0x59:               // Right-side shift
        if (release) {
            shift = 0;
            release = 0;
        } else
            shift = 1;
        break;
    case 0x14:               // Left or Right CNTL key (yep, same scancode)
        if (release) {
            cntl = 0;
            release = 0;
        } else
            cntl = 1;
        break;
    default:
        if (!release)
            if (cntl) {
                ascii = ps2_to_ascii_cntl[code];
            } else if (shift) {
                ascii = ps2_to_ascii_upper[code];
            } else {
                // default
                ascii = ps2_to_ascii_lower[code];
            }
        release = 0;
        break;
    }
    return ascii;
}

// Blocking keyboard read
// Returns  - single ASCII character
char ps2_readc(void) {
    char c;
    while (!(c = ps2_ready()))
        tight_loop_contents();
    ascii = 0;
    return c;
}
