/**
 * flash.c
 *
 * Driver for serial dataflash AT45DB041B on main board, attached as follows:
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

#ifndef FLASH_H
#define FLASH_H

#define SetBit(x, y) (x |=  (y))
#define ClrBit(x, y) (x &=~ (y))
#define ChkBit(x, y) (x &   (y))

extern unsigned char PageBits;
extern unsigned int  PageSize;

uint8_t  flash_read_next_byte(void);
void     flash_start_read(uint32_t offset);
void     flash_start_write(uint32_t offset);
void     flash_init(void);
uint8_t  flash_write_next_byte(uint8_t data);
void     flash_test(void);
void     flash_flush(void);

#endif
