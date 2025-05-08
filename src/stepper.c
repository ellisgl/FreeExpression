/**
 * stepper.c
 *
 * Driver for stepper motors. Each motor is a 6-wire unipolar model. Each of
 * the 4 coils (per motor) can be driven with a full current (through the big
 * transistor) or a reduced current (through a smaller transistor + 47 Ohm
 * resistor). The reduced current I call 'half current', but it may be less.
 *
 * The Y motor (pen movement) is controlled through PORTC, and the X motor
 * (mat roller) is controlled through PORTA. Connections are identical.
 *
 * In addition, the Z coordinate is controlled by two pins: PE2 is used
 * for up/down toggle, and PB6 selects the pressure with a PWM signal.
 *
 * A small pushbutton is attached to PD1, which is active low when
 * pushed. This button detects when the gray cover on the pen holder is
 * moved all the way to the 'home' position.
 *
 * Step resolution is about 400 steps/inch. For a 12x12 inch mat, that means
 * there's about 4800x4800 steps of usable space. Coordinate origin is the
 * blade starting point on the mat. A small amount of negative X is allowed
 * to roll the mat out of the machine.
 *
 * This source original developed by  https://github.com/Arlet/Freecut
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
#include "avrlib.h"
#include <avr/wdt.h>
#include <avr/io.h>
#include <avr/iom1281.h>
#include <avr/interrupt.h>
#include <inttypes.h>
#include <stdio.h>

#include "stepper.h"
#include "keypad.h"
#include "timer.h"
#include "display.h"

#define MAT_EDGE        250         // distance to roll to load mat
#define HOME_Y_LEAD     100         // distance to move the carriage out before homing.
#define MAX_Y           4800        // This is the width of the carriage 4800 == 12"
#define MAX_X           32000       // That's 80 inches of vinyl cutting --
#define MOTOR_OFF_DEL   30000       // number of iterations through the ISR after last motor movement before the stepper power gets turned off
// 30000 is about 1 minute at speed 5

#define HOME            (1 << 1 )   // PD1, attached to 'home' push button
#define PEN             (1 << 2)    // PE2, attached to pen up/down output

#define at_home()       (!(PIND & HOME))

/**
 * motor phases: There are 16 different stepper motor phases, using
 * various combinations of full/half power to create the smallest
 * possible steps:
 *
 *   0   1   2   3   4   5   6   7   8   9  10   11  12  13  14  15
 *  ___________    .   .   .   .   .   .   .   .   .   .   .________
 *     .   .   \___    .   .   .   .   .   .   .   .    ___/   .   .
 * 0   .   .   .   \___________________________________/   .   .   .
 *     .   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .
 *     .   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .
 *     .   .___________________    .   .   .   .   .   .   .   .   .
 *     .___/   .   .   .       \___    .   .   .   .   .   .   .   .
 * 1 __/   .   .   .   .   .       \________________________________
 *     .   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .
 *     .   .   .   .   .    ___________________    .   .   .   .   .
 *     .   .   .   .    ___/   .   .       .   \___.   .   .   .   .
 * 2 __________________/   .   .   .   .       .   \________________
 *     .   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .
 *     .   .   .   .   .   .   .   .   .    ___________________    .
 *     .   .   .   .   .   .   .   .   .___/       .   .   .   \___
 * 3 __________________________________/   .   .       .   .   .   \
 *
 *
 */
/*
 * Pinout for the PORTA as well as PORTC port
 */
#define H0              0x01        // half current, coil 0 - 1 0000 0001
#define F0              0x02        // full current, coil 0 - 2 0000 0010
#define H1              0x04        // half current, coil 1 - 4 0000 0100
#define F1              0x08        // full current, coil 1 - 8 0000 1000
#define H2              0x10        // half current, coil 2 - 16 0001 0000
#define F2              0x20        // full current, coil 2 - 32 0010 0000
#define H3              0x40        // half current, coil 3 - 64 0100 0000
#define F3              0x80        // full current, coil 3 - 128 1000 0000

static uint8_t StepperPhaseTable[] = {
    F0, F0 | H1, F0 | F1, H0 | F1,
    F1, F1 | H2, F1 | F2, H1 | F2,
    F2, F2 | H3, F2 | F3, H2 | F3,
    F3, F3 | H0, F3 | F0, H3 | F0,
};

/* *
 * current location of cutter.
 *
  The two variables loc_x and loc_y represent the current x/y location of the cutting tool and the lower 4 bits are directly
  used to look up the stepper motor drive signals via the phase table (16 micro-steps in the stepper_tick ISR.
  This implies that loc_xand loc_y always have to increment/decrement in steps of 1 in order to execute through all the phases.

 * Initialize at left (away from home switch), and with the mat touching
 * the rollers, such that loading the mat is a simple movement to (0,0)
 */

static volatile int loc_x = -MAT_EDGE;
static volatile int loc_y = -HOME_Y_LEAD; // This is  the axis we need t home first -- negative number prohibits any movement of Y axis for cutting until homed
static volatile int ofs_x = 0; // Stores an offset to the absolute 0 of the X axis -- As set by the offset keyboard command
static volatile int ofs_y = 0; // ""

/**
 * current pressure
 */
static int pressure = MAX_PEN_PWM;

static struct bresenham {
    int step; // current step
    int steps; // number of steps in main direction
    int delta; // number of steps in other direction
    int error; // residual error
    int dx; // x step direction
    int dy; // y step direction
    char steep; // y > x
} b;

static int step_delay; // delay between steps (if not 0)
static unsigned short motor_off_delay = MOTOR_OFF_DEL;

static enum state {
    HOME0 = 0,
    HOME1, // homing until switch is pushed
    HOME2, // reversing until switch is released
    READY, // motor off, pen up
    LINE // draw straight line
} ActionState;

/**
 * command queue. The stepper controller takes commands from the queue
 * using step timer interrupt. Main program can put new commands in.
 *
 * The idea behind this queue is to keep the movement engine as busy as
 * possible, by smoothly joining the end of one stroke with the beginning of
 * the next one, so ideally the whole path can be traced in one continuous
 * motion.
 */
#define CMD_QUEUE_SIZE 32 // must be power of two

enum type {
    MOVE, // move with pen up
    DRAW, // move with pen down
    SPEED, // set speed
    PRESSURE, // set pressure
};

struct cmd {
    enum type action_type; // command type,
    int x, y; // target coordinates
} cmd_queue[CMD_QUEUE_SIZE];

static volatile uint8_t cmd_head, cmd_tail;

// Store the current position of the cutter in offset x and y.
// Later used for positioning relative to this recorded origin

void stepper_set_origin00(void) {
    // if there is anything in the queue don't do it , we are in the middle of cutting possibly
    if (cmd_head != cmd_tail) {
        return;
    }

    if (loc_x < 0 || loc_y < 0) {
        // can't take negative offset, media is not loaded or not homed yet
        return;
    }

    ofs_x = loc_x;
    ofs_y = loc_y;
}

// Find Y home via switch and assume the unloaded media position.

void stepper_home(void) {
    // if there is anything in the queue don't do it , we are in the middle of cutting
    if (cmd_head != cmd_tail) {
        return;
    }

    pen_up();
    loc_y = -HOME_Y_LEAD;
    loc_x = -MAT_EDGE;

    ActionState = HOME0; // immediately do a home sequence
}

// Take stepper drivers off power --

void stepper_off(void) {
    PORTA = 0;
    PORTC = 0;
}

/**
 * allocate a new command, fill in the type, but don't put it in the queue yet.
 */
static struct cmd * alloc_cmd(uint8_t type) {
    struct cmd *cmd;

    while ((uint8_t) (cmd_head - cmd_tail) >= CMD_QUEUE_SIZE) {
        wdt_reset();
    }

    cmd = &cmd_queue[cmd_head % CMD_QUEUE_SIZE];
    cmd->action_type = type;

    return cmd;
}

/**
 * get next command from the queue (called in ISR)
 */
static struct cmd * get_cmd(void) {
    if (cmd_head == cmd_tail) {
        return NULL;
    } else {
        return &cmd_queue[cmd_tail++ % CMD_QUEUE_SIZE];
    }
}

/**
 * Cut to coordinate (x, y).
 */
void stepper_draw(int x, int y) {
    struct cmd *cmd = alloc_cmd(DRAW); // This call blocks if the queue is full

    x += ofs_x;
    y += ofs_y;

    if (x < 0 || x > MAX_X || y < 0 || y > MAX_Y) {
        // don't do it if it's outside the media
        return;
    }

    cmd->x = x;
    cmd->y = y;
    ++cmd_head; // this really allocates the entry in the queue
}

/**
 * move to coordinate (x, y) (with cutter up). We allow moving
 * beyond the mat so that it will roll out.
 */
void stepper_move(int x, int y) {
    struct cmd *cmd = alloc_cmd(MOVE); // This call blocks if the queue is full

    x += ofs_x;
    y += ofs_y;

    if (x < -MAT_EDGE || x > MAX_X || y < 0 || y > MAX_Y) {
        // don't do it if it's outside the media
        return;
    }

    cmd->x = x;
    cmd->y = y;
    ++cmd_head; // this really allocates the entry in the queue
}

/**
 * This moves the cutter/ media freely within the machines range limits - can be used to set offsets and jog the carriage around
 */
static void stepper_jogRelative(int x, int y) {
    struct cmd *cmd = alloc_cmd(MOVE);

    x += loc_x; // relative to the current position
    y += loc_y;

    if (x < -MAX_X || x > MAX_X || y < 0 || y > MAX_Y) {
        // don't do it if it's outside the range of motion
        return;
    }

    cmd->x = x;
    cmd->y = y;
    ++cmd_head; // this really allocates the entry in the queue
}

/**
 * Function to move the cutting head/media freely throughout the physical range of the machine.
 */
void stepper_jog_manual(int direction, int dist) {
    // If there is anything in the queue don't move !!
    // To protect from moving while a program executes.

    if (cmd_head != cmd_tail) {
        return;
    }

    switch (direction) {
        case KEYPAD_MOVEUP:
            stepper_jogRelative(dist, 0);
            break;

        case KEYPAD_MOVEUPLEFT:
            stepper_jogRelative(dist, dist);
            break;

        case KEYPAD_MOVELEFT:
            stepper_jogRelative(0, dist);
            break;

        case KEYPAD_MOVEDNLEFT:
            stepper_jogRelative(-dist, dist);
            break;

        case KEYPAD_MOVEDN:
            stepper_jogRelative(-dist, 0);
            break;

        case KEYPAD_MOVEDNRIGHT:
            stepper_jogRelative(-dist, -dist);
            break;

        case KEYPAD_MOVERIGHT:
            stepper_jogRelative(0, -dist);
            break;

        case KEYPAD_MOVEUPRIGHT:
            stepper_jogRelative(dist, -dist);
            break;
    }
}

void stepper_speed(int speed) {
    struct cmd *cmd = alloc_cmd(SPEED);
    cmd->x = speed;
    ++cmd_head;
}

/**
 * Loading the media: The mat/media needs to be pulled under the rollers first.
 */
void stepper_load_paper(void) {
    if (loc_y < 0) {
        // not homed yet
        return;
    }

    if (loc_x < 0) {
        // if media is not loaded yet
        loc_x = -MAT_EDGE; // This is the distance we pull in the mat -- if origin offsets are in effect it will drive to the last recorded position
        stepper_move(0, loc_y); // move media to location 0, this is the HOME point for X
        stepper_move(0, 0); // make sure Y is at zero as well -- GS: not sure why it would not be
    }
}

/**
 * Unloading the media: go to absolute 0 minus the MAT feed distance to free the mat/media
 */
void stepper_unload_paper(void) {
    stepper_move(-(MAT_EDGE + ofs_x), -ofs_y); // move to absolute position
}

/**
 * set the pressure of the cutter.
 */
void stepper_pressure(int pressure) {
    struct cmd *cmd = alloc_cmd(PRESSURE);
    cmd->x = pressure;
    ++cmd_head;
}

/**
 * The original firmware also removes the PWM signal, but it seems
 * to work OK when you leave it on.
 */
void pen_up(void) {
    if (PORTE & PEN) {
        step_delay = 50;
    }

    PORTE &= ~PEN;
}

/**
 * move pen down
 */
void pen_down(void) {
    int pe = PORTE;

    if (loc_x < 0 || loc_y < 0) {
        // prevent dropping the cutter when there is no media underneath
        return;
    }

    if (pe & PEN) {
        // already down, ignore
        return;
    }

    PORTE |= PEN;

    step_delay = 50;
}

/**
 * initialize Bresenham line drawing algorithm. Draw from (x, y)
 * to (x1, y1).
 */
static void bresenham_init(int x1, int y1) {
    int dx, dy;

    if (x1 > loc_x) {
        b.dx = 1;
        dx = x1 - loc_x;
    } else {
        b.dx = -1;
        dx = loc_x - x1;
    }

    if (y1 > loc_y) {
        b.dy = 1;
        dy = y1 - loc_y;
    } else {
        b.dy = -1;
        dy = loc_y - y1;
    }

    if (dx > dy) {
        b.steep = 0;
        b.steps = dx;
        b.delta = dy;
    } else {
        b.steep = 1;
        b.steps = dy;
        b.delta = dx;
    }

    b.step = 0;
    b.error = b.steps / 2;
}

/**
 * perform single step in Bresenham line drawing algorithm.
 */
static enum state bresenham_step(void) {
    if (b.step >= b.steps) {
        return READY;
    }

    ++b.step;
    if ((b.error -= b.delta) < 0) {
        b.error += b.steps;
        loc_x += b.dx;
        loc_y += b.dy;
    } else if (b.steep) {
        loc_y += b.dy;
    } else {
        loc_x += b.dx;
    }

    return LINE;
}

/**
 * get next command from command queue. Called from the ISR at the READY state
 */
enum state do_next_command(void) {
    struct cmd *cmd = get_cmd();

    if (cmd == NULL) {
        return READY; // Stay at the ready
    }

    switch (cmd->action_type) {
        case MOVE:
        case DRAW:
            if (cmd->action_type == MOVE) {
                pen_up();
            } else {
                pen_down();
            }

            // No motion required -- already where we want to go on both axes
            if (loc_x == cmd->x && loc_y == cmd->y) {
                return READY;
            }

            bresenham_init(cmd->x, cmd->y);
            return LINE;

        case PRESSURE:
            pressure = cmd->x;
            timer_set_pen_pressure(pressure);
            break;

        case SPEED:
            timer_set_stepper_speed(cmd->x);
            break;
    }

    return READY;
}

/**
 * This function is called by a timer interrupt. It does one motor step.
 */
void stepper_tick(void) {
    // this introduces a delay in the execution of stepper movements to allow for pen up,down,etc motions to be executed
    if (step_delay) {
        step_delay--;
        return;
    }

    // abort cutting if 'STOP' is pressed.
    // TODO: This is not sufficient -- also need to stop the flow of new commands from the UART and signal to the PC
    // that we want to abort the job, otherwise we only stop what is currently in the queue
    if (keypad_stop_pressed()) {
        ActionState = READY;
        cmd_tail = cmd_head;
        pen_up();
        stepper_off();
    }

    switch (ActionState) {
        case HOME0:
            if (loc_y < 0) {
                // first move out by the offset given in the home init
                ++loc_y; // so that the initial power on jolt doesn't trigger switch
            } else {
                ActionState = HOME1;
            }
            break;


        case HOME1:
            step_delay = 1; // home at a reduced speed

            if (!at_home()) {
                loc_y--; // moving the carriage toward the home location
            } else {
                ActionState = HOME2; // home switch touched -- now move the other way
            }
            break;

        case HOME2:
            step_delay = 4;

            if (at_home()) {
                ++loc_y; // move the other way until the switch opens
            } else {
                loc_y = 0; // now this is home on Y axis
                ofs_x = ofs_y = 0;
                ActionState = READY;
            }
            break;

        case READY:
            if ((ActionState = do_next_command()) == READY) {
                break;
            }
            // else fall through to LINE

        case LINE:
            ActionState = bresenham_step(); // this gets the next loc_x and loc_y, incremented, decremented or left unchanged for the single step motion below
            break;
    }


    if (ActionState == READY) {
        /* *
         * The motors get quite hot when powered on, so we turn them off after a certain time of idling.
         * Note this time is tied to the stepper speed, i.e. at high motor speed this time is shorter
         */
        if (motor_off_delay) {
            motor_off_delay--;
        } else {
            stepper_off();
        }
    } else {
        // this is where the motion happens, command the stepper drives to the next step phase (1 out of 16)
        PORTA = StepperPhaseTable[ loc_x & 0x0f ]; // low 4 bits determine phase
        PORTC = StepperPhaseTable[ loc_y & 0x0f ];
        motor_off_delay = MOTOR_OFF_DEL; // reset the timeout for the stepper motor power down
    }
}

/**
 * Enables the ports for stepper drivers and cutter up/down control
 */
void stepper_init(void) {
    stepper_off();
    DDRA = 0xff;
    DDRC = 0xff;
    DDRE |= PEN;
    PORTD |= HOME; // enable pull-up
    stepper_home(); // Find the X home via switch
}
