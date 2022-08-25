MCU = atmega1281

TARGET = libm2u8.a
SRC +=  \
u8g_bitmap.c \
u8g_circle.c \
u8g_clip.c \
u8g_com_api.c \
u8g_com_api_16gr.c \
u8g_com_atmega_sw_spi.c \
u8g_com_i2c.c \
u8g_com_io.c \
u8g_com_null.c \
u8g_com_atmega_hw_spi.c \
u8g_com_atmega_sw_spi.c \
u8g_cursor.c \
u8g_delay.c \
u8g_dev_null.c \
u8g_dev_ssd1325_nhd27oled_bw_new.c \
u8g_dev_ssd1325_nhd27oled_gr_new.c \
u8g_ellipse.c \
u8g_font.c \
u8g_font_data.c \
u8g_line.c \
u8g_ll_api.c \
u8g_page.c \
u8g_pb.c \
u8g_pb14v1.c \
u8g_pb16h1.c \
u8g_pb16h2.c \
u8g_pb16v1.c \
u8g_pb16v2.c \
u8g_pb32h1.c \
u8g_pb8h1.c \
u8g_pb8h1f.c \
u8g_pb8h2.c \
u8g_pb8h8.c \
u8g_pb8v1.c \
u8g_pb8v2.c \
u8g_pbxh16.c \
u8g_pbxh24.c \
u8g_polygon.c \
u8g_rect.c \
u8g_rot.c \
u8g_scale.c \
u8g_state.c \
u8g_u16toa.c \
u8g_u8toa.c \
u8g_virtual_screen.c

CFLAGS = -Os -fpack-struct
CFLAGS += -fshort-enums -Wall -Wno-error -Wstrict-prototypes -DF_CPU=16000000UL

# Optional assembler flags.
ASFLAGS = -Wa,-ahlms=$(<:.S=.lst),-gstabs

# Optional linker flags.
LDFLAGS = -Wl,-Map=$(TARGET).map,--cref

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

# Default target.
all: $(TARGET) tags

$(TARGET) : $(OBJ)
	@ar rcs $(TARGET) $(OBJ)
.PHONY: prog

#you may need to change this line to program your device
prog: all
	avrdude -pm128 -cstk500v2 -b115200 -P/dev/ttyUSB0 -u -V -U flash:w:$(TARGET).hex:i

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
# (Code originally taken from the GNU make user manual and modified (See README.txt Credits).
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

