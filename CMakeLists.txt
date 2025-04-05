cmake_minimum_required(VERSION 3.10)
project(FreeExpression C)

# MCU and other variables
set(MCU atmega1281)
set(FCLK 16000000UL)
set(TARGET freeexpression)
set(FORMAT ihex)

# Add the m2u8 source files
file(GLOB M2U8_SRC "src/m2u8/*.c")

# Create a static library for m2u8
add_library(m2u8 STATIC ${M2U8_SRC})

# Source files
set(SRC
    src/main.c
    src/usb.c
    src/display.c
    src/display_oled.c
    src/keypad.c
    src/timer.c
    src/stepper.c
    src/cli.c
    src/flash.c
    src/dial.c
    src/hpgl.c
    src/shvars.c
    src/scale.c
    src/serial.c
    src/spi.c
)

# Compiler flags
set(CMAKE_C_COMPILER avr-gcc)
set(CMAKE_C_FLAGS "-mmcu=${MCU} -Os -DFCLK=${FCLK} -DF_CPU=${FCLK} -fpack-struct -I./src/m2u8 -fshort-enums -Wall -Werror -Wstrict-prototypes")
set(CMAKE_EXE_LINKER_FLAGS "-Wl,--gc-sections,-Map=${TARGET}.map,--cref -lm")

add_executable(${TARGET}.elf ${SRC})

# Link the m2u8 library to the executable
target_link_libraries(${TARGET}.elf m2u8)

# Create hex file
add_custom_command(TARGET ${TARGET}.elf POST_BUILD
                   COMMAND avr-objcopy -O ${FORMAT} -R .eeprom ${TARGET}.elf ${TARGET}.hex
                   COMMENT "Building ${TARGET}.hex"
)

# Create disassembly listing
add_custom_command(TARGET ${TARGET}.elf POST_BUILD
                   COMMAND avr-objdump -d ${TARGET}.elf > ${TARGET}.dis
                   COMMENT "Creating disassembly listing ${TARGET}.dis"
)

# Create extended listing file
add_custom_command(TARGET ${TARGET}.elf POST_BUILD
                   COMMAND avr-objdump -h -S ${TARGET}.elf > ${TARGET}.lss
                   COMMENT "Creating extended listing file ${TARGET}.lss"
)

# Programming target
add_custom_target(prog
                  COMMAND avrdude -c usbasp -p m1281 -U flash:w:${TARGET}.hex
                  DEPENDS ${TARGET}.hex
                  COMMENT "Programming the device"
)

# Replace .c with .o
string(REPLACE ".c" ".o" OBJ_FILES "${SRC}")

# Replace .c with .d
string(REPLACE ".c" ".d" DEP_FILES "${SRC}")

# Replace .c with .s
string(REPLACE ".c" ".s" ASM_FILES "${SRC}")

# Clean target
add_custom_target(cleanup
                  COMMAND ${CMAKE_COMMAND} -E remove ${TARGET}.hex ${TARGET}.elf ${TARGET}.map ${TARGET}.dis ${TARGET}.lss
                  COMMAND ${CMAKE_COMMAND} -E remove ${OBJ_FILES} ${DEP_FILES} ${ASM_FILES}
                  COMMENT "Cleaning up"
)