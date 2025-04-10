/**
 * serial.c - Low level functions for sending and recieving bytes via the serial port
 * Part of Grbl v0.9
 *
 * Copyright (c) 2012-2014 Sungeun K. Jeon
 *
 * Grbl is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Grbl is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Grbl.  If not, see <http://www.gnu.org/licenses/>.
 */
/** 
 * This file is based on work from Grbl v0.8, distributed under the 
 * terms of the MIT-license. See COPYING for more details.  
 *   Copyright (c) 2009-2011 Simen Svale Skogsrud
 *   Copyright (c) 2011-2012 Sungeun K. Jeon
 */
#ifndef serial_h
#define serial_h

#define BAUD_RATE                   9600

#define ENABLE_XONXOFF

#define UART1_RECEIVE_INTERRUPT     USART1_RX_vect
#define UART1_TRANSMIT_INTERRUPT    USART1_UDRE_vect
#define UART1_STATUS                UCSR1A
#define UART1_CONTROL               UCSR1B
#define UART1_DATA                  UDR1
#define UART1_UDRIE                 UDRIE1

#ifndef RX_BUFFER_SIZE
#define RX_BUFFER_SIZE              128
#endif
#ifndef TX_BUFFER_SIZE
#define TX_BUFFER_SIZE              64
#endif

#define SERIAL_NO_DATA              0xFF

#ifdef ENABLE_XONXOFF
#define RX_BUFFER_FULL              96 // XOFF high watermark
#define RX_BUFFER_LOW               32 // XON low watermark
#define SEND_XOFF                   1
#define SEND_XON                    2
#define XOFF_SENT                   3
#define XON_SENT                    4
#define XOFF_CHAR                   0x13
#define XON_CHAR                    0x11
#endif

void serial_init(void);

// Writes one byte to the TX serial buffer. Called by main program.
void serial_write(uint8_t data);

// Fetches the first byte in the serial read buffer. Called by main program.
uint8_t serial_read(void);

// Reset and empty data in read buffer. Used by e-stop and reset.
void serial_reset_read_buffer(void);

// Returns the number of bytes used in the RX serial buffer.
uint8_t serial_get_rx_buffer_count(void);

// Returns the number of bytes used in the TX serial buffer.
// NOTE: Not used except for debugging and ensuring no TX bottlenecks.
uint8_t serial_get_tx_buffer_count(void);

#endif
