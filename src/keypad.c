/**
 * keypad.c
 *
 * Keypad is designed as a matrix with 20 cols, and 5 rows. A 20 bit shift register controls the individual column bits
 * 
 * NOTE: Due to this design, if you press two (or more) keys in the same row
 * simultaneously, the the two corresponding outputs of the shift register
 * are shorted through the keys, and the CPU gets undefined results.
 * 
 * Pin-out of the connector is as follows:
 *
 * Pin 	|  Name	| AVR  | Description
 *------+-------+------+------------
 *  1   |  GND 	|      |
 *  2   |  Vcc  |      |
 *  3   |  Stop | PD0  | Direct connection to stop button
 *  4   |  O4   | PG4  | Row output 2 (bottom)
 *  5   |  O3   | PG3  | Row output 4
 *  6   |  O2 	| PG2  | Row output 3
 *  7   |  O0   | PG0  | Row output 0 (top row)
 *  8   |  LED 	| PD5  | LED Enable (0=LEDs on)
 *  9   |  O1   | PG1  | Row output 1
 * 10   | CLK   | PD7  | Shift register clock
 * 11   | Data  | PD6  | Shift register data in
 * 12   | Test  |      | Shift register data out (not used)
 * 13	| NC	| NC   |
 * 14	| NC	| NC   | 	
 * Key layout is straightforward: column 0 is left, column 13 is right, 
 * row 0 is top, row 4 is bottom. The rest of the keys are on columns 14-19 
 *
 * Cricut Expression: The grey arrow keys and CUT key are mapped in column 14:
 * 
 * Row  | Key
 *------+------
 *  0   | Right
 *  1   | Left 
 *  2   | CUT 
 *  3   | Up 
 *  4   | Down 
 
 * 
 * The various LEDs are also connected to the shift register outputs, 
 * and can be turned on with the LED Enable pin (0=On, 1=Off).  
 *
 * LED Layout is as follows:
 * Function keys, F1 to F6, Material Size, Real Dial Size
 * COL0  COL1
 * COL2  COL3
 * COL4  COL5				COL10 (LED between CUT and Up arrow)
 * COL6  COL7
 * COL8  COL9
 * 
 * On the Expression machine, there are numerous extra buttons that have LEDS
 * But I cannot figure out how to individually address any of the leds
 * Portrait, Mix n Match, Quantity
 * Fit to Page, Fit to Length, Auto Fill
 * Multi Cut, Center Point, Flip, Line Return
 * Settings, Mat Size, xtra1, xtra2
 * - arrow, + arrow, OK 
 *
 * This source original developped by  https://github.com/Arlet/Freecut
 *
 * This file is part of FreeExpression.
 *
 * https://github.com/thetazzbot/FreeExpression
 *
 * FreeExpression is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2.
 *
 * FreeExpression is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public
 * License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with FreeExpression. If not, see http://www.gnu.org/licenses/.
 *
 */
#include <avr/io.h>
#include <avr/interrupt.h>
#include <inttypes.h>
#include <stdio.h>
#include "keypad.h"
#include "timer.h"
#include "stepper.h"
#include "display.h"
#include "flash.h"
#include "usb.h"
// pin assignment
// These appear to be the same on both the Cake and Expression machines
// There are some machine specific routines in the keypad_cake.c and keypad_expression.c files
// 
#define STOP (1 << 0) // PD0
#define LEDS (1 << 5) // PD5
#define DATA (1 << 6)  // PD6
#define CLK  (1 << 7) // PD7
#define ROWS    (0x1f)  // mask for PG4-0

#define clk_h()  do { PORTD |=  CLK;  } while(0)
#define clk_l()  do { PORTD &= ~CLK;  } while(0)
#define data_h() do { PORTD |=  DATA; } while(0)
#define data_l() do { PORTD &= ~DATA; } while(0)
#define get_rows() (~PING & ROWS)

static uint8_t keypad_state[KBD_MAX_COLS]; // current state
static uint8_t keypad_prev[KBD_MAX_COLS]; // previous state
static uint16_t leds;
static uint8_t sound_mode = 1; // sound on

en_language Lang = HPGL;

static int k_state = 0;
void _beep(int key);

/*
 * keypad_write_cols: write all 16 column bits in 'val' to shift register.
 */
static void keypad_write_cols(short val) {
    int i;

    for (i = 0; i < KBD_MAX_COLS; ++i) {
        if (val < 0)
            data_h();
        else
            data_l();
        clk_h();
        val <<= 1;
        clk_l();
    }
}

void keypad_set_leds(uint16_t mask) {
    leds = mask;
    leds_off();
    keypad_write_cols(~leds);
    leds_on();
}

/*
 * return state of 'STOP' key.
  STOP is not part of the keyboard  -- individually wired to port
 */
char keypad_stop_pressed(void) {
    int c = (PIND & STOP);
    return !c;
}

/* 
 * keypad_scan: perform a single scan of keyboard. Returns keycode of 
 * key that was pressed (or -1 if nothing).  
 */
int keypad_scan(void) {
    int row, col;
    int pressed = -1;

    keypad_write_cols(0); // All bits to 0
    data_h(); // shift in consecutive 1's

    for (col = 0; col < KBD_MAX_COLS; ++col) {
        keypad_state[col] = get_rows();
        clk_h();
        clk_l();
    }
    keypad_write_cols(~leds);

    // keyboard has been scanned, now look for pressed keys
    for (col = 0; col < KBD_MAX_COLS; ++col) {
        uint8_t diff = keypad_state[col] ^ keypad_prev[col];
        if (diff) {
            for (row = 0; row < KBD_MAX_ROWS; ++row) {
                uint8_t mask = 1 << row;
                // the highest column that shows switch bit closure as a bit in the row read is the column associated with this button.
                // we keep overwriting key pressed with readings from higher columns until the last column that shows the bit
                if (diff & mask & keypad_state[col]) {
                    pressed = row * KBD_MAX_COLS + col;
                }
            }
        }
        keypad_prev[col] = keypad_state[col];
    }
    return pressed;
}

void keypadSet_Speed_state(void) {
    k_state = KEYPAD_XTRA2;

}

void keypadSet_Pressure_state(void) {
    k_state = KEYPAD_XTRA1;

}

int keypad_poll(void) {
    int c;
    int key = keypad_scan();
#ifdef DEBUG_KEYBOARD
    char string[40];
    if (key >= 0) {
        sprintf(string, "%d", key);
        display_puts(string);
    }
#endif
    // the button leds are really strange
    // some of them are on a shift register it seems
    // but some are directly connected to pins
    //keypad_set_leds( 0x000 );

    switch (key) {
        case KEYPAD_SOUNDONOFF:
            sound_mode = !sound_mode;
            break;

        case KEYPAD_FLIP:
            usb_puts("Hello world");
            break;

        case KEYPAD_LOADMAT:
            stepper_load_paper();
            display_puts("Media loaded");
            break;

        case KEYPAD_UNLOADMAT:
            stepper_unload_paper();
            display_puts("Media unloaded");
            break;

        case KEYPAD_RESETALL:
            display_puts("Homing carriage...");
            stepper_home();
            break;

        case KEYPAD_BACKSPACE:
            stepper_set_origin00();
            display_puts("Location 0,0 set");
            break;

            // this jogs X and Y freely -- use to load, unload or set position for  0,0 origin
        case KEYPAD_MOVEUP:
        case KEYPAD_MOVEUPLEFT:
        case KEYPAD_MOVELEFT:
        case KEYPAD_MOVEDNLEFT:
        case KEYPAD_MOVEDN:
        case KEYPAD_MOVEDNRIGHT:
        case KEYPAD_MOVERIGHT:
        case KEYPAD_MOVEUPRIGHT:
            stepper_jog_manual(key, 25); // move 1/16" each increment

            // For auto key repeat on these buttons, clear previous kbd status[] so that a new button press registers again 
            for (c = 0; c < KBD_MAX_COLS; ++c) {
                keypad_prev[c] = 0;
            }
            break;

#ifdef DEBUG_FLASH  
        case KEYPAD_F1:
            flash_test();
            break;
#endif
        case KEYPAD_F5:
            display_puts("Cutter down");
            pen_down();
            break;

        case KEYPAD_F6:
            display_puts("Cutter up");
            pen_up();
            break;

        case KEYPAD_XTRA1:
            k_state = key;
            break;

        case KEYPAD_XTRA2:
            k_state = key;
            break;

        case KEYPAD_CUT:
            k_state = key;
            break;

        case KEYPAD_MINUS: // decrements either pressure or speed depending on what was last pressed 
            if (k_state == KEYPAD_XTRA1) {
                // PRESSURE SET
                // pressure is inversely related to 1023, min pressure
                int p = timer_get_pen_pressure() - 1;
                timer_set_pen_pressure(p);
            }

            if (k_state == KEYPAD_XTRA2) { 
                // SPEED SET
                int p = timer_get_stepper_speed() - 1;
                timer_set_stepper_speed(p);
            }
            break;

        case KEYPAD_PLUS: 
            // Increments either pressure or speed depending on what was last pressed 
            if (k_state == KEYPAD_XTRA1) { 
                // PRESSURE SET
                // pressure is inversely related to 1023, min pressure
                int p = timer_get_pen_pressure() + 1;
                timer_set_pen_pressure(p);
            }

            if (k_state == KEYPAD_XTRA2) { 
                // SPEED SET
                int p = timer_get_stepper_speed() + 1;
                timer_set_stepper_speed(p);
            }
            break;
    }

    _beep(key);

    return key;
}

void _beep(int key) {
    switch (key) {
        case KEYPAD_MOVEUP:
        case KEYPAD_MOVEUPLEFT:
        case KEYPAD_MOVELEFT:
        case KEYPAD_MOVEDNLEFT:
        case KEYPAD_MOVEDN:
        case KEYPAD_MOVEDNRIGHT:
        case KEYPAD_MOVERIGHT:
        case KEYPAD_MOVEUPRIGHT:
            break;
        default:
            if (sound_mode == 1 && key > 0) {
                // if a key was pressed that is not assigned beep
                beeper_on(3600);
                msleep(50);
                beeper_off();
            }
    }
}

void keypad_init(void) {
    clk_l();
    leds_off();
    DDRD |= (LEDS | DATA | CLK);
    PORTD |= STOP;
    DDRG &= ~ROWS; // rows are inputs
    PORTG |= ROWS; // enable internal pull-ups
}
