# Connectors

This file documents the pinout of the connectors and the modifications required for JTAG and SPI LCD hookup LCD connector -- see also LCD.c

|J1 Pin|  LCD Pin   |AVR pin    | Original connection                                                 |
|------|------------|-----------|---------------------------------------------------------------------|
|1     |GND, 1      |           |                                                                     |
|2     |Vcc, 2      |5v         |                                                                     |
|3     |MOSI,(R/W),5|PB2,MOSI,12|PF5, (TMS,56), cut trace                                             |
|4     |C,(RS),4    |PB5,15     |PF6, (TDO,55), cut trace                                             |
|5     |MISO,(D0),7 |PB3,MISO,13|PF7, (TDI,54), cut trace                                             |
|6     |NC          |PE4        |                                                                     |
|7     |NC          |PE5        |                                                                     |
|8     |NC          |PE6        |                                                                     |
|9     |RES,16      |PE7        |                                                                     |
|10    |SCK,(E),6   |PB1,SCK,11 |PF4 via R28 and Q2 (TCK,57), cut trace.                              |
|      |            |           |Was LCD backlight and Dial power, switched via R28, Q2, PF4 now used |
|      |            |           |as JTAG TCK on J5 pin 10                                             |

## J5, JTAG and serial programming and debugging

|J5 pin|Func.  |via    |AVR pin|Components removed or trace cut|
|------|-------|-------|-------|-------------------------------|
|1     |PDO    |       |3      |                               |
|2     |VCC    |R11    |52     |                               |
|3     |SCK    |R9     |11     |Also connects to U1 pin 2 (SCK)|
|4     |PDI    |R12    |2      |                               |
|5     |RESETn |R15    |20     |                               |
|6     |GND    |direct |22     |                               |
|7     |TMS    |direct |56     |R24 removed                    |
|8     |TDI    |direct |54     |R13 removed                    |
|9     |TDO    |direct |55     |R14 removed                    |
|10    |TCK    |direct |57     |Cut trace on R28, 820ohms      |
