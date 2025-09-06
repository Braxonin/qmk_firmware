#include QMK_KEYBOARD_H
#define RAW_EPSIZE 32

#include "rgblight.h"

// Declare the Bongo Cat tap animation control from oled.c
extern uint8_t tap_anim;
extern uint16_t tap_timer;

static uint16_t rgb_reset_timer = 0;
static bool rgb_override_active = false;
static bool rgb_saved_once = false;

// Save the current RGB state
static uint8_t saved_mode = 0;
static uint16_t saved_hue = 0;
static uint8_t saved_sat = 0;
static uint8_t saved_val = 0;

// OLED step function declaration
void oled_display_mode_step(void);

// Keymap layers
const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    [0] = LAYOUT(
        KC_A,                        KC_B,
        KC_C,   KC_D, KC_E,   LT(0, KC_NO),
        KC_F,           KC_G, KC_H, KC_I,
        KC_J,           KC_K, KC_L, KC_M
    ),
    [1] = LAYOUT(
        KC_1,                        KC_2,
        KC_3,   KC_4, KC_5,   LT(0, KC_NO),
        KC_6,           KC_7, KC_8, KC_9,
        KC_0,           KC_MINS, KC_EQL, KC_BSPC
    ),
    [2] = LAYOUT(
        KC_EXLM,                    KC_AT,
        KC_HASH,   KC_DLR, KC_PERC, LT(0, KC_NO),
        KC_CIRC,       KC_AMPR, KC_ASTR, KC_LPRN,
        KC_RPRN,       KC_LEFT, KC_DOWN, KC_UP
    ),
    [3] = LAYOUT(
        KC_MPLY,                    KC_MUTE,
        KC_VOLD,   KC_VOLU, KC_MNXT, LT(0, KC_NO),
        KC_MPRV,       KC_HOME, KC_END, KC_DEL,
        KC_PGUP,       KC_PGDN, KC_TAB, KC_ENT
    )
};


// Toggle switch state tracking
typedef struct {
    bool     is_pressed;
    uint16_t press_time;
    bool     long_press_handled;
} toggle_state_t;

static toggle_state_t toggle_state = {
    .is_pressed = false,
    .press_time = 0,
    .long_press_handled = false
};

// Handle the toggle switch behavior
void handle_toggle_switch(bool pressed) {
    if (pressed) {
        toggle_state.is_pressed = true;
        toggle_state.press_time = timer_read();
        toggle_state.long_press_handled = false;
    } else {
        if (toggle_state.is_pressed) {
            if (timer_elapsed(toggle_state.press_time) < 1500 && !toggle_state.long_press_handled) {
                uint8_t current = get_highest_layer(layer_state);
                if (current >= 3) {
                    layer_clear();
                } else {
                    layer_move(current + 1);
                }
            }
            toggle_state.is_pressed = false;
        }
    }
}

// RGB layer color override with one-time save
void trigger_layer_rgb(uint16_t hue, uint8_t sat, uint8_t val) {
    if (!rgb_override_active && !rgb_saved_once) {
        saved_mode = rgblight_get_mode();
        HSV hsv = rgblight_get_hsv();
        saved_hue = hsv.h;
        saved_sat = hsv.s;
        saved_val = hsv.v;
        rgb_saved_once = true;
    }

    rgblight_mode(RGBLIGHT_MODE_STATIC_LIGHT);
    rgblight_sethsv_noeeprom(hue, sat, val);

    rgb_reset_timer = timer_read();
    rgb_override_active = true;
}

// Matrix scan: handle long press and RGB timeout
void matrix_scan_user(void) {
    if (toggle_state.is_pressed && 
        !toggle_state.long_press_handled && 
        timer_elapsed(toggle_state.press_time) >= 1500) {

        oled_display_mode_step();
        toggle_state.long_press_handled = true;
    }

    if (rgb_override_active && timer_elapsed(rgb_reset_timer) > 2500) {
        rgblight_mode(saved_mode);
        rgblight_sethsv_noeeprom(saved_hue, saved_sat, saved_val);
        rgb_override_active = false;
        rgb_saved_once = false;
    }
}

// 🔥 Tap animation trigger + toggle logic
bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    if (record->event.pressed) {
        tap_anim = true;
        tap_timer = timer_read();
    }

    switch (keycode) {
        case LT(0, KC_NO):
            handle_toggle_switch(record->event.pressed);
            return false;
    }
    return true;
}

// OLED housekeeping (unchanged)
void housekeeping_task_user(void) {
#ifdef OLED_ENABLE
    static uint16_t oled_timer = 0;
    if (timer_elapsed(oled_timer) > 1500) {
        oled_timer = timer_read();
    }
#endif
}

// RGB color change on layer switch
layer_state_t layer_state_set_user(layer_state_t state) {
    switch (get_highest_layer(state)) {
        case 1:
            trigger_layer_rgb(0, 0, 255);       // White
            break;
        case 2:
            trigger_layer_rgb(0, 255, 255);     // Red
            break;
        case 3:
            trigger_layer_rgb(85, 255, 255);    // Green
            break;
        case 4:
            trigger_layer_rgb(170, 255, 255);   // Blue
            break;
    }
    return state;
}

// Set default RGB effect at startup
void keyboard_post_init_user(void) {
    // Default rainbow swirl (cycling hues)
    rgblight_mode(RGBLIGHT_MODE_RAINBOW_SWIRL);
}

// Encoder mapping
#if defined(ENCODER_MAP_ENABLE)
const uint16_t PROGMEM encoder_map[][NUM_ENCODERS][NUM_DIRECTIONS] = {
    [0] = { ENCODER_CCW_CW(KC_PGDN, KC_PGUP),  ENCODER_CCW_CW(KC_VOLD, KC_VOLU)  },
    [1] = { ENCODER_CCW_CW(KC_LEFT, KC_RGHT),  ENCODER_CCW_CW(KC_DOWN, KC_UP)    },
    [2] = { ENCODER_CCW_CW(KC_TAB, KC_ENT),    ENCODER_CCW_CW(KC_HOME, KC_END)   },
    [3] = { ENCODER_CCW_CW(KC_MPRV, KC_MNXT),  ENCODER_CCW_CW(KC_VOLD, KC_VOLU)  },
};
#endif
