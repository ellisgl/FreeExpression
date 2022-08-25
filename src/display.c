#include "display_oled.h"

void display_init(void) {
    oled_display_init();
}

void display_puts(char *s) {
    oled_display_puts(s);
}

void display_print(char *s) {
    oled_display_println(s);
}

void display_update(void) {
    oled_display_update();
}

void display_println(char *s) {
    oled_display_println(s);
}
