// RGB tool keys for the custom SOLID_BG_* effects.
//
// In Oryx, put these keys on your RGB layer. They are caught here and never
// reach the computer, so do not use them for anything else in the layout.
//
//   F13-F16  Presets 1-4: set the colour, brightness, hue shift and
//            highlight brightness and saturation (keeps the current animation)
//            (with Shift held: presets 5-8)
//   F17      Hue shift - 1  (with Shift held: - 16)
//   F18      Hue shift + 1  (with Shift held: + 16)
//   F19      Type out the current values, ready to paste into the presets
//            table below, e.g. "{131, 255, 200, 86, 200, 255},"
//   F20-F23  Switch to the Wide / Cross / Nexus / Splash effect
//            (with Shift held: the Multi version of that effect)
//   F24      Type out the current effect's name, e.g. "SOLID_BG_WIDE"
//
// "Shift held" can mean either holding a Shift key, or a key set up in Oryx
// as Shift + the F-key (e.g. a dedicated Shift+F20 key). Both work.
//
// All values are on QMK's 0-255 scale (hue 0-255 covers the whole wheel).

#include "rgb_tools.h"

// ---- Settings you can change ----------------------------------------------
// Each preset:
//   { hue, saturation, brightness, hue shift, highlight brightness, highlight saturation }
// All 0-255. The first three are the idle keys. The highlight values are what
// pressed keys show when the idle keys are at exactly that colour.
//
// After a preset is applied, the highlight follows the built-in keys:
//   hue keys         highlight hue keeps the same offset (wraps round)
//   brightness keys  highlight brightness keeps the same ratio
//   saturation keys  highlight saturation keeps the same ratio
// A ratio can't push a value past 255 (or the keyboard's brightness limit),
// so a brighter highlight needs idle keys below full brightness.
//
// In the comments, each hex code is the colour QMK shows at full brightness,
// followed by the brightness the preset actually uses.
typedef struct {
    uint8_t h, s, v, shift, highlight_v, highlight_s;
} rgb_preset_t;

static const rgb_preset_t presets[] = {
    // F13-F16
    {131, 255, 200, 87, 255, 238},  // Vaporwave Overload: cyan #00F3FF at 78%, magenta highlight #FF10EE at 100%
    {131, 255, 200, 161, 255, 255}, // Chromed Out: cyan #00F3FF at 78%, yellow highlight #FFD800 at 100%
    {192, 255, 200, 143, 255, 234}, // Bio-Tech Pulse: electric violet #7800FF at 78%, acid green highlight #39FF14 at 100%
    {241, 221, 200, 177, 255, 221}, // Neon Synth: hot pink #FF2178 at 78%, electric blue highlight #2154FF at 100%
    // Shift + F13-F16: dark idle keys, bright flash
    {193, 255, 90, 98, 255, 255},  // Night City Tactical: deep violet #7E00FF at 35%, cyber yellow highlight #FFD200 at 100%
    {162, 255, 102, 223, 255, 255}, // Netrunner Terminal: abyssal blue #0039FF at 40%, electric cyan highlight #00FEFF at 100%
    {252, 255, 76, 0, 255, 208},    // Arasaka Alert: dark crimson #FF0021 at 30%, hot coral-red highlight #FF2E4A at 100%
    {126, 255, 90, 202, 255, 255},  // Bio-Hazard Matrix: shadow teal #00FFF0 at 35%, acid lime highlight #51FF00 at 100%
};

_Static_assert(sizeof(presets) / sizeof(presets[0]) == 8, "there must be exactly 8 presets");

#define SHIFT_DEFAULT 128 // used until a shift has been saved
#define SHIFT_BIG_STEP 16 // F17 / F18 step with Shift held
// ---------------------------------------------------------------------------

uint8_t solid_bg_hue_shift = SHIFT_DEFAULT;

// Highlight brightness and saturation as ratios of the idle values:
// 256 = the same, 512 = twice as much. Kept as ratios so the built-in
// brightness and saturation keys move the highlight with the idle keys.
#define RATIO_ONE 256
#define V_RATIO_MAX 2047 // about 8x: fits in the save slot
#define S_RATIO_MAX 1023 // about 4x
static uint16_t highlight_v_ratio = RATIO_ONE;
static uint16_t highlight_s_ratio = RATIO_ONE;

static uint8_t apply_ratio(uint8_t idle, uint16_t ratio, uint8_t max) {
    uint32_t x = ((uint32_t)idle * ratio + RATIO_ONE / 2) / RATIO_ONE;
    return x > max ? max : x;
}

// Works out the ratio that turns idle into target, within limit.
static uint16_t make_ratio(uint8_t target, uint8_t idle, uint16_t limit) {
    if (idle == 0) return RATIO_ONE;
    uint32_t r = ((uint32_t)target * RATIO_ONE + idle / 2) / idle;
    return r > limit ? limit : r;
}

uint8_t solid_bg_highlight_val(uint8_t idle_v) {
    return apply_ratio(idle_v, highlight_v_ratio, RGB_MATRIX_MAXIMUM_BRIGHTNESS);
}

uint8_t solid_bg_highlight_sat(uint8_t idle_s) {
    return apply_ratio(idle_s, highlight_s_ratio, 255);
}

// The shift and both ratios are saved in QMK's 32-bit "user" EEPROM slot so
// they survive unplugging. The marker tells saved values apart from a blank
// slot or an older format.
#define RGB_TOOLS_MARKER 0x5

typedef union {
    uint32_t raw;
    struct {
        uint32_t shift : 8;
        uint32_t v_ratio : 11;
        uint32_t s_ratio : 10;
        uint32_t marker : 3;
    };
} rgb_tools_config_t;

static void save_settings(void) {
    rgb_tools_config_t cfg = {.raw = 0};
    cfg.shift              = solid_bg_hue_shift;
    cfg.v_ratio            = highlight_v_ratio;
    cfg.s_ratio            = highlight_s_ratio;
    cfg.marker             = RGB_TOOLS_MARKER;
    eeconfig_update_user(cfg.raw);
}

void rgb_tools_init(void) {
    rgb_tools_config_t cfg = {.raw = eeconfig_read_user()};
    if (cfg.marker == RGB_TOOLS_MARKER) {
        solid_bg_hue_shift = cfg.shift;
        highlight_v_ratio  = cfg.v_ratio;
        highlight_s_ratio  = cfg.s_ratio;
    }
}

static void apply_preset(const rgb_preset_t *p) {
    rgb_matrix_sethsv(p->h, p->s, p->v);
    solid_bg_hue_shift = p->shift;

    // Use the values QMK actually set (it caps brightness at the keyboard's
    // limit), so the highlight comes out as the preset intends.
    highlight_v_ratio = make_ratio(p->highlight_v, rgb_matrix_get_val(), V_RATIO_MAX);
    highlight_s_ratio = make_ratio(p->highlight_s, rgb_matrix_get_sat(), S_RATIO_MAX);
    save_settings();
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

// Types e.g. "{131, 255, 200, 86, 200, 255},", the same format as the presets
// table, so it can be pasted straight in.
static void type_values(void) {
    uint8_t v = rgb_matrix_get_val();
    uint8_t s = rgb_matrix_get_sat();
    char  buf[40];
    char *p = buf;
    p       = write_text(p, "{");
    p       = write_u8(p, rgb_matrix_get_hue());
    p       = write_text(p, ", ");
    p       = write_u8(p, s);
    p       = write_text(p, ", ");
    p       = write_u8(p, v);
    p       = write_text(p, ", ");
    p       = write_u8(p, solid_bg_hue_shift);
    p       = write_text(p, ", ");
    p       = write_u8(p, solid_bg_highlight_val(v));
    p       = write_text(p, ", ");
    p       = write_u8(p, solid_bg_highlight_sat(s));
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
    bool shifted = shift_held();

    // A key set in Oryx as e.g. Shift+F20 arrives as one combined keycode:
    // split it into the F-key and its modifiers.
    if (keycode >= QK_MODS && keycode <= QK_MODS_MAX) {
        if (QK_MODS_GET_MODS(keycode) & MOD_LSFT) shifted = true; // left or right Shift
        keycode = QK_MODS_GET_BASIC_KEYCODE(keycode);
    }

    switch (keycode) {
        case KC_F13 ... KC_F24:
            break;
        default:
            return true; // not one of ours
    }
    if (!record->event.pressed) return false;

    switch (keycode) {
        case KC_F13 ... KC_F16:
            apply_preset(&presets[(keycode - KC_F13) + (shifted ? 4 : 0)]);
            break;
        // uint8_t wraps round, so 0 - 1 = 255 and 255 + 1 = 0.
        case KC_F17: solid_bg_hue_shift -= shifted ? SHIFT_BIG_STEP : 1; save_settings(); break;
        case KC_F18: solid_bg_hue_shift += shifted ? SHIFT_BIG_STEP : 1; save_settings(); break;
        case KC_F19: type_without_mods(type_values); break;
        case KC_F20 ... KC_F23:
            rgb_matrix_mode(effects[keycode - KC_F20][shifted ? 1 : 0]);
            break;
        case KC_F24: type_without_mods(type_effect_name); break;
    }
    return false;
}
