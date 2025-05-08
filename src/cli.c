/**
 * cli.c
 *
 * command line interface
 *
 * Passes data to the Language parser & interprets the results
 *
 * TODO: Re-implement scaling
 * TODO: Implement store - or - cut feature
 * TODO: Implement retrieve and cut feature
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
#include <avr/wdt.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <setjmp.h>
#include <math.h>

#include "timer.h"
#include "cli.h"
#include "usb.h"
#include "stepper.h"
#include "version.h"
#include "shvars.h"
#include "hpgl.h"
#include "display.h"
#include "keypad.h"

void cli_poll(void) {
    STEPPER_COORD dstx, dsty;
    char          c;
    int8_t        cmd;
    uint8_t       labelchar;

    while ((c = (char) usb_getc()) != SERIAL_NO_DATA) {
        switch (Lang) {
        case HPGL:
            cmd = hpgl_char(c, &dstx, &dsty, &labelchar);
            break;

        default:
            continue; // just consume everything and do nothing
        }

        switch (cmd) {
        case CMD_PU:
            if (dstx >= 0 && dsty >= 0) {
                // filter out illegal moves
                stepper_move(dstx, dsty);
            }
            break;

        case CMD_PD:
            if (dstx >= 0 && dsty >= 0) {
                // filter out illegal moves
                stepper_draw(dstx, dsty);
            }
            break;

        case CMD_INIT:
            // 1. Home
            // 2. Initialize scale and stuff.
            // Typically happens at start and end of each document;
            dstx = dsty = 0;
            stepper_move(dstx, dsty);
            break;

        case CMD_SEEK0:
            stepper_move(dstx, dsty);
            break;

        default:
            break;
        }
    }
}
