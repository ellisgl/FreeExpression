# FreeExpression: open firmware for Cricut Expression 1 devices.
# MCU name
MCU = atmega1281

# Output format. (can be srec, ihex)
FORMAT = ihex

# Target file name (without extension).
TARGET = freeexpression

# Speed
FCLK = 16000000UL

# List C source files here. (C dependencies are automatically generated.)
SRC = src/main.c src/usb.c src/display.c src/display_oled.c src/keypad.c src/timer.c src/stepper.c src/cli.c src/flash.c src/dial.c src/hpgl.c src/shvars.c src/scale.c src/serial.c src/spi.c

# Assembler sources
ASRC =

# Optional compiler flags.
CFLAGS  = -D$(MCU) $(TEST) $(CE) -Os -DFCLK=$(FCLK) -DF_CPU=$(FCLK) -fpack-struct -I./src/m2u8
CFLAGS += -fshort-enums -Wall -Werror -Wstrict-prototypes

# Optional assembler flags.
ASFLAGS = -Wa,-ahlms=$(<:.S=.lst),-gstabs

# Optional linker flags.
LDFLAGS = -Wl,--gc-sections,-Map=$(TARGET).map,--cref

# Additional libraries
LDFLAGS += -lm
LDFLAGS += -L./src/m2u8 -lm2u8
# ---------------------------------------------------------------------------

# Define directories, if needed.

# Define programs and commands.
SHELL = sh
CC = avr-gcc
OBJCOPY = avr-objcopy
OBJDUMP = avr-objdump

# Define all object files.
OBJ = $(ASRC:.S=.o) $(SRC:.c=.o)

# Define all listing files.
LST = $(ASRC:.S=.lst) $(SRC:.c=.lst)

# Combine all necessary flags and optional flags. Add target processor to flags.
ALL_CFLAGS = -mmcu=$(MCU) -I. $(CFLAGS)
ALL_ASFLAGS = -mmcu=$(MCU) -I. -x assembler-with-cpp $(ASFLAGS)

# Ensure the m2u8 library is compiled first, if needed.
ifneq ($(wildcard ./src/m2u8/m2u8.a),)
  ALL_CFLAGS += -L./src/m2u8 -lm2u8
else
  # Compile the m2u8 library first if it does not exist.
  $(shell $(CC) $(ALL_CFLAGS) -c ./src/m2u8/m2u8.c -o ./src/m2u8/m2u8.o)
  $(shell $(CC) -mmcu=$(MCU) -I. -c ./src/m2u8/m2u8.o -o ./src/m2u8/m2u8.a)
  ALL_CFLAGS += -L./src/m2u8 -lm2u8
endif

# Default target.
all: $(TARGET).elf $(TARGET).hex $(TARGET).lss tags

.PHONY: prog

# You may need to change this line to program your device.
prog: all
	#avrdude -pm128 -cstk500v2 -b115200 -P/dev/ttyUSB0 -u -V -U flash:w:$(TARGET).hex:i
	avrdude -c usbasp -p m1281 -U flash:w:$(TARGET).hex

# Create final output files (.hex) from ELF output file.
%.hex: %.elf
	$(OBJCOPY) -O $(FORMAT) -R .eeprom $< $@

# Disassembly listing
%.dis: %.elf
	$(OBJDUMP) -d $< > $@

# Create extended listing file from ELF output file.
%.lss: %.elf
	$(OBJDUMP) -h -S $< > $@

# Link: create ELF output file from object files.
.SECONDARY : $(TARGET).elf
.PRECIOUS : $(OBJ)
%.elf: $(OBJ)
	$(CC) $(ALL_CFLAGS) $(OBJ) --output $@ $(LDFLAGS)

# Compile: create object files from C source files.
%.o : %.c
	$(CC) -c $(ALL_CFLAGS) $< -o $@

# Compile: create assembler files from C source files.
%.s : %.c
	$(CC) -S $(ALL_CFLAGS) $< -o $@

# Assemble: create object files from assembler source files.
%.o : %.S
	$(CC) -c $(ALL_ASFLAGS) $< -o $@

# Target: clean project.
clean :
	rm -f *.hex
	rm -f *.obj
	rm -f *.elf
	rm -f *.map
	rm -f *.obj
	rm -f *.sym
	rm -f *.ssm
	rm -f *.lnk
	rm -f *.lss
	rm -f *.lst
	rm -f $(OBJ)
	rm -f $(LST)
	rm -f $(SRC:.c=.s)
	rm -f $(SRC:.c=.d)
	rm -f logfile

# Automatically generate C source code dependencies.
# (Code originally taken from the GNU make user manual and modified (See README.txt Credits).)
# Note that this will work with sh (bash) and sed that is shipped with WinAVR (see the SHELL variable defined above).
# This may not work with other shells or other seds.
%.d: %.c
	set -e; $(CC) -MM $(ALL_CFLAGS) $< \
	| sed  -e 's,\(.*\)\.o[ :]*,\1.o \1.d : ,g' \
	> $@; [ -s $@ ] || rm -f $@

# Remove the '-' if you want to see the dependency files generated.
-include $(SRC:.c=.d)

# Listing of phony targets.
.PHONY : all clean

tags: *.[hc]
	ctags *.[hc]
