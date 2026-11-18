// RGB tool keys for the custom SOLID_BG_* effects.
//
// In Oryx, put these keys on your RGB layer. They are caught here and never
// reach the computer, so do not use them for anything else in the layout.
//
//   F13-F16  Presets 1-4: set the global colour AND the hue shift
//            (keeps current brightness and animation)
//   F17      Hue shift - 1  (with Shift held: - 16)
//   F18      Hue shift + 1  (with Shift held: + 16)
//   F19      Type out the current preset values, ready to paste into the
//            presets table below, e.g. "{169, 255, 128},"
//   F20-F23  Switch to the Wide / Cross / Nexus / Splash effect
//            (with Shift held: the Multi version of that effect)
//   F24      Type out the current effect's name, e.g. "SOLID_BG_WIDE"
//
// All values are on QMK's 0-255 scale (hue 0-255 covers the whole wheel).

#include "rgb_tools.h"

// ---- Settings you can change ----------------------------------------------
// Each preset: { hue, saturation, hue shift }, all 0-255.
typedef struct {
    uint8_t h, s, shift;
} rgb_preset_t;

static const rgb_preset_t presets[] = {
    {169, 255, 128}, // F13: blue (from your Oryx key), opposite-colour highlight
    {34, 255, 128},  // F14: yellow (from your Oryx key), opposite-colour highlight
    {131, 255, 64},  // F15: cyan (from your Oryx key), quarter-turn highlight
    {193, 255, 64},  // F16: purple (from your Oryx key), quarter-turn highlight
};

#define SHIFT_DEFAULT 128 // used until a shift has been saved
#define SHIFT_BIG_STEP 16 // F17 / F18 step with Shift held
// ---------------------------------------------------------------------------

uint8_t solid_bg_hue_shift = SHIFT_DEFAULT;

// The shift is saved in QMK's "user" EEPROM slot so it survives unplugging.
// The marker byte tells a saved value apart from a blank slot.
#define RGB_TOOLS_MARKER 0xA5

typedef union {
    uint32_t raw;
    struct {
        uint8_t shift;
        uint8_t marker;
    };
} rgb_tools_config_t;

static void save_shift(void) {
    rgb_tools_config_t cfg = {.raw = 0};
    cfg.shift              = solid_bg_hue_shift;
    cfg.marker             = RGB_TOOLS_MARKER;
    eeconfig_update_user(cfg.raw);
}

void rgb_tools_init(void) {
    rgb_tools_config_t cfg = {.raw = eeconfig_read_user()};
    solid_bg_hue_shift     = (cfg.marker == RGB_TOOLS_MARKER) ? cfg.shift : SHIFT_DEFAULT;
}

static void set_colour_keep_brightness(uint8_t h, uint8_t s) {
    rgb_matrix_sethsv(h, s, rgb_matrix_get_val());
}

// Writes a number as text (no padding) and returns a pointer just past it.
static char *write_u8(char *p, uint8_t n) {
    if (n >= 100) *p++ = '0' + n / 100;
    if (n >= 10) *p++ = '0' + (n / 10) % 10;
    *p++ = '0' + n % 10;
    return p;
}

static char *write_text(char *p, const char *s) {
    while (*s) *p++ = *s++;
    return p;
}

// Types e.g. "{169, 255, 128}," (hue, saturation, hue shift), the same
// format as the presets table, so it can be pasted straight in.
static void type_values(void) {
    char  buf[24];
    char *p = buf;
    p       = write_text(p, "{");
    p       = write_u8(p, rgb_matrix_get_hue());
    p       = write_text(p, ", ");
    p       = write_u8(p, rgb_matrix_get_sat());
    p       = write_text(p, ", ");
    p       = write_u8(p, solid_bg_hue_shift);
    p       = write_text(p, "},");
    *p      = '\0';
    send_string(buf);
}

// The custom effects from rgb_matrix_user.inc: [single, multi] for F20-F23.
static const uint8_t effects[][2] = {
    {RGB_MATRIX_CUSTOM_SOLID_BG_WIDE, RGB_MATRIX_CUSTOM_SOLID_BG_MULTIWIDE},
    {RGB_MATRIX_CUSTOM_SOLID_BG_CROSS, RGB_MATRIX_CUSTOM_SOLID_BG_MULTICROSS},
    {RGB_MATRIX_CUSTOM_SOLID_BG_NEXUS, RGB_MATRIX_CUSTOM_SOLID_BG_MULTINEXUS},
    {RGB_MATRIX_CUSTOM_SOLID_BG_SPLASH, RGB_MATRIX_CUSTOM_SOLID_BG_MULTISPLASH},
};

// Names of every effect compiled into the firmware, in the same order as
// QMK's own effect list. QMK builds that list by including the same effect
// files with RGB_MATRIX_EFFECT() defined to make an enum entry; here it is
// defined to make a string instead, so the two lists always match, whichever
// effects are switched on or off in config.h.
static const char *const effect_names[] = {
    "NONE",
#define RGB_MATRIX_EFFECT(name, ...) #name,
#include "rgb_matrix_effects.inc"
#ifdef RGB_MATRIX_CUSTOM_KB
#    include "rgb_matrix_kb.inc"
#endif
#ifdef RGB_MATRIX_CUSTOM_USER
#    include "rgb_matrix_user.inc"
#endif
#undef RGB_MATRIX_EFFECT
};

// Stops the build if the two lists ever get out of step.
_Static_assert(sizeof(effect_names) / sizeof(effect_names[0]) == RGB_MATRIX_EFFECT_MAX, "effect_names does not match QMK's effect list");

static void type_effect_name(void) {
    uint8_t mode = rgb_matrix_get_mode();
    send_string(mode < RGB_MATRIX_EFFECT_MAX ? effect_names[mode] : "UNKNOWN");
}

// Types text with Shift (and any other modifier) released, so the
// characters come out as written.
static void type_without_mods(void (*typer)(void)) {
    uint8_t mods = get_mods();
    clear_mods();
    clear_oneshot_mods();
    typer();
    set_mods(mods);
    send_keyboard_report();
}

static bool shift_held(void) {
    return (get_mods() | get_oneshot_mods()) & MOD_MASK_SHIFT;
}

bool process_rgb_tools(uint16_t keycode, keyrecord_t *record) {
    switch (keycode) {
        case KC_F13 ... KC_F24:
            break;
        default:
            return true; // not one of ours
    }
    if (!record->event.pressed) return false;

    switch (keycode) {
        case KC_F13 ... KC_F16: {
            const rgb_preset_t *p = &presets[keycode - KC_F13];
            set_colour_keep_brightness(p->h, p->s);
            solid_bg_hue_shift = p->shift;
            save_shift();
            break;
        }
        // uint8_t wraps round, so 0 - 1 = 255 and 255 + 1 = 0.
        case KC_F17: solid_bg_hue_shift -= shift_held() ? SHIFT_BIG_STEP : 1; save_shift(); break;
        case KC_F18: solid_bg_hue_shift += shift_held() ? SHIFT_BIG_STEP : 1; save_shift(); break;
        case KC_F19: type_without_mods(type_values); break;
        case KC_F20 ... KC_F23:
            rgb_matrix_mode(effects[keycode - KC_F20][shift_held() ? 1 : 0]);
            break;
        case KC_F24: type_without_mods(type_effect_name); break;
    }
    return false;
}
