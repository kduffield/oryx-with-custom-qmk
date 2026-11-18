// RGB tool keys: colour presets, hue-shift control and a "print values" key.
// See rgb_tools.c for the key assignments and preset values.
#pragma once

#include QMK_KEYBOARD_H

// Hue shift used by the SOLID_BG_* effects in rgb_matrix_user.inc.
extern uint8_t solid_bg_hue_shift;

// Highlight saturation for a given idle saturation (used by the same effects).
uint8_t solid_bg_highlight_sat(uint8_t idle_s);

// Highlight brightness for a given idle brightness (used by the same effects).
uint8_t solid_bg_highlight_val(uint8_t idle_v);

// Call once from keyboard_post_init_user().
void rgb_tools_init(void);

// Call first thing in process_record_user(); returns false if it handled the key.
bool process_rgb_tools(uint16_t keycode, keyrecord_t *record);
