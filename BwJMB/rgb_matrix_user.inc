// Custom "solid background" reactive effects for the ZSA Voyager.
//
// Unpressed keys show the colour set with the hue / saturation / brightness
// keys (or a "Set colour" key). Pressed keys blend towards a highlight colour,
// using the same shapes as QMK's built-in Wide, Cross, Nexus and Splash effects.
//
// The animation speed keys control how fast the highlight fades.
// These effects appear in the "Switch animation" cycle after the built-in ones.

RGB_MATRIX_EFFECT(SOLID_BG_WIDE)
RGB_MATRIX_EFFECT(SOLID_BG_MULTIWIDE)
RGB_MATRIX_EFFECT(SOLID_BG_CROSS)
RGB_MATRIX_EFFECT(SOLID_BG_MULTICROSS)
RGB_MATRIX_EFFECT(SOLID_BG_NEXUS)
RGB_MATRIX_EFFECT(SOLID_BG_MULTINEXUS)
RGB_MATRIX_EFFECT(SOLID_BG_SPLASH)
RGB_MATRIX_EFFECT(SOLID_BG_MULTISPLASH)

#ifdef RGB_MATRIX_CUSTOM_EFFECT_IMPLS

// ---- Settings you can change ----------------------------------------------
// How far round the colour wheel the highlight is from the idle colour.
// 0-255 covers the whole wheel: 128 = opposite colour, 0 = same colour.
// This is now a variable controlled by the RGB tool keys: see rgb_tools.c
// for its presets and default.
extern uint8_t solid_bg_hue_shift;
#    define SOLID_BG_HUE_SHIFT solid_bg_hue_shift
// Highlight brightness, set per preset and scaled by the brightness keys.
uint8_t solid_bg_highlight_val(uint8_t idle_v);
// Highlight saturation, set per preset and scaled by the saturation keys.
uint8_t solid_bg_highlight_sat(uint8_t idle_s);
// ---------------------------------------------------------------------------

// A "shape" returns how strongly one key press affects one LED:
// 255 = full highlight colour, 0 = idle colour.
typedef uint8_t (*solid_bg_shape_f)(int16_t dx, int16_t dy, uint8_t dist, uint16_t tick);

// Glow spreading out from the pressed key (same maths as SOLID_REACTIVE_WIDE).
static uint8_t solid_bg_wide_shape(int16_t dx, int16_t dy, uint8_t dist, uint16_t tick) {
    uint16_t effect = tick + dist * 5;
    if (effect > 255) effect = 255;
    return 255 - effect;
}

// Glow along the pressed key's row and column (same maths as SOLID_REACTIVE_CROSS).
static uint8_t solid_bg_cross_shape(int16_t dx, int16_t dy, uint8_t dist, uint16_t tick) {
    uint16_t effect = tick + dist;
    dx              = dx < 0 ? -dx : dx;
    dy              = dy < 0 ? -dy : dy;
    dx              = dx * 16 > 255 ? 255 : dx * 16;
    dy              = dy * 16 > 255 ? 255 : dy * 16;
    effect += dx > dy ? dy : dx;
    if (effect > 255) effect = 255;
    return 255 - effect;
}

// Lines shooting out along the row and column (same maths as SOLID_REACTIVE_NEXUS).
static uint8_t solid_bg_nexus_shape(int16_t dx, int16_t dy, uint8_t dist, uint16_t tick) {
    uint16_t effect = tick - dist;
    if (effect > 255) effect = 255;
    if (dist > 72) effect = 255;
    if ((dx > 8 || dx < -8) && (dy > 8 || dy < -8)) effect = 255;
    return 255 - effect;
}

// Ring rippling outwards from the pressed key (same maths as SOLID_SPLASH).
static uint8_t solid_bg_splash_shape(int16_t dx, int16_t dy, uint8_t dist, uint16_t tick) {
    uint16_t effect = tick - dist;
    if (effect > 255) effect = 255;
    return 255 - effect;
}

// Shared code: paint every LED in the idle colour, blended towards the
// highlight colour by however much recent key presses affect it.
// multi = false reacts to the latest key press only; true reacts to all recent ones.
static bool solid_bg_runner(bool multi, effect_params_t *params, solid_bg_shape_f shape) {
    RGB_MATRIX_USE_LIMITS(led_min, led_max);

    HSV base = rgb_matrix_config.hsv;
    HSV high = base;
    high.h += SOLID_BG_HUE_SHIFT;
    high.v = solid_bg_highlight_val(base.v);
    high.s = solid_bg_highlight_sat(base.s);
    RGB base_rgb = hsv_to_rgb(base);
    RGB high_rgb = hsv_to_rgb(high);

    uint8_t count = g_last_hit_tracker.count;
    uint8_t start = (multi || count == 0) ? 0 : count - 1;
    uint8_t speed = qadd8(rgb_matrix_config.speed, 1);

    for (uint8_t i = led_min; i < led_max; i++) {
        RGB_MATRIX_TEST_LED_FLAGS();

        uint8_t amount = 0;
        for (uint8_t j = start; j < count; j++) {
            int16_t  dx   = g_led_config.point[i].x - g_last_hit_tracker.x[j];
            int16_t  dy   = g_led_config.point[i].y - g_last_hit_tracker.y[j];
            uint8_t  dist = sqrt16(dx * dx + dy * dy);
            uint16_t tick = scale16by8(g_last_hit_tracker.tick[j], speed);
            amount        = qadd8(amount, shape(dx, dy, dist, tick));
        }

        rgb_matrix_set_color(i,
                             scale8(base_rgb.r, 255 - amount) + scale8(high_rgb.r, amount),
                             scale8(base_rgb.g, 255 - amount) + scale8(high_rgb.g, amount),
                             scale8(base_rgb.b, 255 - amount) + scale8(high_rgb.b, amount));
    }
    return rgb_matrix_check_finished_leds(led_max);
}

static bool SOLID_BG_WIDE(effect_params_t *params) {
    return solid_bg_runner(false, params, &solid_bg_wide_shape);
}
static bool SOLID_BG_MULTIWIDE(effect_params_t *params) {
    return solid_bg_runner(true, params, &solid_bg_wide_shape);
}
static bool SOLID_BG_CROSS(effect_params_t *params) {
    return solid_bg_runner(false, params, &solid_bg_cross_shape);
}
static bool SOLID_BG_MULTICROSS(effect_params_t *params) {
    return solid_bg_runner(true, params, &solid_bg_cross_shape);
}
static bool SOLID_BG_NEXUS(effect_params_t *params) {
    return solid_bg_runner(false, params, &solid_bg_nexus_shape);
}
static bool SOLID_BG_MULTINEXUS(effect_params_t *params) {
    return solid_bg_runner(true, params, &solid_bg_nexus_shape);
}
static bool SOLID_BG_SPLASH(effect_params_t *params) {
    return solid_bg_runner(false, params, &solid_bg_splash_shape);
}
static bool SOLID_BG_MULTISPLASH(effect_params_t *params) {
    return solid_bg_runner(true, params, &solid_bg_splash_shape);
}

#endif // RGB_MATRIX_CUSTOM_EFFECT_IMPLS
