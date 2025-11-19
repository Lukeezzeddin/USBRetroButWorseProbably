// gamecube.c
#include "gamecube.h"
#include "joybus.pio.h"
#include "GamecubeConsole.h"
#include "pico/bootrom.h"
#include "pico/stdlib.h"
#include "tusb.h"

// Declaration of global variables
GamecubeConsole gc;
gc_report_t gc_report;

extern void GamecubeConsole_init(GamecubeConsole* console, uint pin, PIO pio, int sm, int offset);
extern bool GamecubeConsole_WaitForPoll(GamecubeConsole* console);
extern void GamecubeConsole_SendReport(GamecubeConsole* console, gc_report_t* report);
extern void GamecubeConsole_SetMode(GamecubeConsole* console, GamecubeMode mode);

uint8_t hid_to_gc_key[256] = { [0 ... 255] = GC_KEY_NOT_FOUND };
uint8_t gc_rumble = 0;
uint8_t gc_kb_led = 0;
uint8_t gc_last_rumble = 0;
uint8_t gc_kb_counter = 0;
static const float LStick = 0.85f;   // scaling factor for left stick

// *** FIX: helper to scale relative to centre
static inline uint8_t scale_toward_center(uint8_t val, float scale, uint8_t center) {
    int16_t rel = (int16_t)val - (int16_t)center;
    int16_t scaled = (int16_t)(rel * scale);
    int16_t result = scaled + (int16_t)center;
    if (result < 0)   result = 0;
    if (result > 255) result = 255;
    return (uint8_t)result;
}

// init hid key to gc key lookup table
void gc_kb_key_lookup_init()
{
    hid_to_gc_key[HID_KEY_A] = GC_KEY_A;
    hid_to_gc_key[HID_KEY_B] = GC_KEY_B;
    hid_to_gc_key[HID_KEY_C] = GC_KEY_C;
    hid_to_gc_key[HID_KEY_D] = GC_KEY_D;
    hid_to_gc_key[HID_KEY_E] = GC_KEY_E;
    hid_to_gc_key[HID_KEY_F] = GC_KEY_F;
    hid_to_gc_key[HID_KEY_G] = GC_KEY_G;
    hid_to_gc_key[HID_KEY_H] = GC_KEY_H;
    hid_to_gc_key[HID_KEY_I] = GC_KEY_I;
    hid_to_gc_key[HID_KEY_J] = GC_KEY_J;
    hid_to_gc_key[HID_KEY_K] = GC_KEY_K;
    hid_to_gc_key[HID_KEY_L] = GC_KEY_L;
    hid_to_gc_key[HID_KEY_M] = GC_KEY_M;
    hid_to_gc_key[HID_KEY_N] = GC_KEY_N;
    hid_to_gc_key[HID_KEY_O] = GC_KEY_O;
    hid_to_gc_key[HID_KEY_P] = GC_KEY_P;
    hid_to_gc_key[HID_KEY_Q] = GC_KEY_Q;
    hid_to_gc_key[HID_KEY_R] = GC_KEY_R;
    hid_to_gc_key[HID_KEY_S] = GC_KEY_S;
    hid_to_gc_key[HID_KEY_T] = GC_KEY_T;
    hid_to_gc_key[HID_KEY_U] = GC_KEY_U;
    hid_to_gc_key[HID_KEY_V] = GC_KEY_V;
    hid_to_gc_key[HID_KEY_W] = GC_KEY_W;
    hid_to_gc_key[HID_KEY_X] = GC_KEY_X;
    hid_to_gc_key[HID_KEY_Y] = GC_KEY_Y;
    hid_to_gc_key[HID_KEY_Z] = GC_KEY_Z;
    hid_to_gc_key[HID_KEY_1] = GC_KEY_1;
    hid_to_gc_key[HID_KEY_2] = GC_KEY_2;
    hid_to_gc_key[HID_KEY_3] = GC_KEY_3;
    hid_to_gc_key[HID_KEY_4] = GC_KEY_4;
    hid_to_gc_key[HID_KEY_5] = GC_KEY_5;
    hid_to_gc_key[HID_KEY_6] = GC_KEY_6;
    hid_to_gc_key[HID_KEY_7] = GC_KEY_7;
    hid_to_gc_key[HID_KEY_8] = GC_KEY_8;
    hid_to_gc_key[HID_KEY_9] = GC_KEY_9;
    hid_to_gc_key[HID_KEY_0] = GC_KEY_0;
    hid_to_gc_key[HID_KEY_MINUS] = GC_KEY_MINUS;
    hid_to_gc_key[HID_KEY_EQUAL] = GC_KEY_CARET;
    hid_to_gc_key[HID_KEY_GRAVE] = GC_KEY_YEN;
    hid_to_gc_key[HID_KEY_PRINT_SCREEN] = GC_KEY_AT;
    hid_to_gc_key[HID_KEY_BRACKET_LEFT] = GC_KEY_LEFTBRACKET;
    hid_to_gc_key[HID_KEY_SEMICOLON] = GC_KEY_SEMICOLON;
    hid_to_gc_key[HID_KEY_APOSTROPHE] = GC_KEY_COLON;
    hid_to_gc_key[HID_KEY_BRACKET_RIGHT] = GC_KEY_RIGHTBRACKET;
    hid_to_gc_key[HID_KEY_COMMA] = GC_KEY_COMMA;
    hid_to_gc_key[HID_KEY_PERIOD] = GC_KEY_PERIOD;
    hid_to_gc_key[HID_KEY_SLASH] = GC_KEY_SLASH;
    hid_to_gc_key[HID_KEY_BACKSLASH] = GC_KEY_BACKSLASH;
    hid_to_gc_key[HID_KEY_F1] = GC_KEY_F1;
    hid_to_gc_key[HID_KEY_F2] = GC_KEY_F2;
    hid_to_gc_key[HID_KEY_F3] = GC_KEY_F3;
    hid_to_gc_key[HID_KEY_F4] = GC_KEY_F4;
    hid_to_gc_key[HID_KEY_F5] = GC_KEY_F5;
    hid_to_gc_key[HID_KEY_F6] = GC_KEY_F6;
    hid_to_gc_key[HID_KEY_F7] = GC_KEY_F7;
    hid_to_gc_key[HID_KEY_F8] = GC_KEY_F8;
    hid_to_gc_key[HID_KEY_F9] = GC_KEY_F9;
    hid_to_gc_key[HID_KEY_F10] = GC_KEY_F10;
    hid_to_gc_key[HID_KEY_F11] = GC_KEY_F11;
    hid_to_gc_key[HID_KEY_F12] = GC_KEY_F12;
    hid_to_gc_key[HID_KEY_ESCAPE] = GC_KEY_ESC;
    hid_to_gc_key[HID_KEY_INSERT] = GC_KEY_INSERT;
    hid_to_gc_key[HID_KEY_DELETE] = GC_KEY_DELETE;
    hid_to_gc_key[HID_KEY_GRAVE] = GC_KEY_GRAVE;
    hid_to_gc_key[HID_KEY_BACKSPACE] = GC_KEY_BACKSPACE;
    hid_to_gc_key[HID_KEY_TAB] = GC_KEY_TAB;
    hid_to_gc_key[HID_KEY_CAPS_LOCK] = GC_KEY_CAPSLOCK;
    hid_to_gc_key[HID_KEY_SHIFT_LEFT] = GC_KEY_LEFTSHIFT;
    hid_to_gc_key[HID_KEY_SHIFT_RIGHT] = GC_KEY_RIGHTSHIFT;
    hid_to_gc_key[HID_KEY_CONTROL_LEFT] = GC_KEY_LEFTCTRL;
    hid_to_gc_key[HID_KEY_ALT_LEFT] = GC_KEY_LEFTALT;
    hid_to_gc_key[HID_KEY_GUI_LEFT] = GC_KEY_LEFTUNK1;
    hid_to_gc_key[HID_KEY_SPACE] = GC_KEY_SPACE;
    hid_to_gc_key[HID_KEY_GUI_RIGHT] = GC_KEY_RIGHTUNK1;
    hid_to_gc_key[HID_KEY_APPLICATION] = GC_KEY_RIGHTUNK2;
    hid_to_gc_key[HID_KEY_ARROW_LEFT] = GC_KEY_LEFT;
    hid_to_gc_key[HID_KEY_ARROW_DOWN] = GC_KEY_DOWN;
    hid_to_gc_key[HID_KEY_ARROW_UP] = GC_KEY_UP;
    hid_to_gc_key[HID_KEY_ARROW_RIGHT] = GC_KEY_RIGHT;
    hid_to_gc_key[HID_KEY_ENTER] = GC_KEY_ENTER;
    hid_to_gc_key[HID_KEY_HOME] = GC_KEY_HOME;
    hid_to_gc_key[HID_KEY_END] = GC_KEY_END;
    hid_to_gc_key[HID_KEY_PAGE_DOWN] = GC_KEY_PAGEDOWN;
    hid_to_gc_key[HID_KEY_PAGE_UP] = GC_KEY_PAGEUP;
}

// init for gamecube communication
void ngc_init()
{
    set_sys_clock_khz(130000, true);
    stdio_init_all();

    gpio_init(SHIELD_PIN_L);
    gpio_set_dir(SHIELD_PIN_L, GPIO_OUT);
    gpio_init(SHIELD_PIN_L + 1);
    gpio_set_dir(SHIELD_PIN_L + 1, GPIO_OUT);
    gpio_init(SHIELD_PIN_R);
    gpio_set_dir(SHIELD_PIN_R, GPIO_OUT);
    gpio_init(SHIELD_PIN_R + 1);
    gpio_set_dir(SHIELD_PIN_R + 1, GPIO_OUT);

    gpio_put(SHIELD_PIN_L, 0);
    gpio_put(SHIELD_PIN_L + 1, 0);
    gpio_put(SHIELD_PIN_R, 0);
    gpio_put(SHIELD_PIN_R + 1, 0);

    gpio_init(BOOTSEL_PIN);
    gpio_set_dir(BOOTSEL_PIN, GPIO_IN);
    gpio_pull_up(BOOTSEL_PIN);

    gpio_init(GC_3V3_PIN);
    gpio_set_dir(GC_3V3_PIN, GPIO_IN);
    gpio_pull_down(GC_3V3_PIN);

    sleep_ms(200);
    if (!gpio_get(GC_3V3_PIN)) reset_usb_boot(0, 0);

    int sm = -1;
    int offset = -1;
    gc_kb_key_lookup_init();
    GamecubeConsole_init(&gc, GC_DATA_PIN, pio, sm, offset);
    gc_report = default_gc_report;
}

uint8_t gc_kb_key_lookup(uint8_t hid_key)
{
    return hid_to_gc_key[hid_key];
}

uint8_t furthest_from_center(uint8_t a, uint8_t b, uint8_t center)
{
    int distance_a = abs(a - center);
    int distance_b = abs(b - center);
    return (distance_a > distance_b) ? a : b;
}

void __not_in_flash_func(core1_entry)(void)
{
    while (1)
    {
        gc_rumble = GamecubeConsole_WaitForPoll(&gc) ? 255 : 0;
        GamecubeConsole_SendReport(&gc, &gc_report);
        update_pending = false;

        gc_kb_counter++;
        gc_kb_counter &= 15;

        for (unsigned short int i = 0; i < MAX_PLAYERS; ++i)
        {
            if (players[i].global_x != 0)
            {
                players[i].global_x -= (players[i].output_analog_1x - 128);
                players[i].output_analog_1x = 128;
            }
            if (players[i].global_y != 0)
            {
                players[i].global_y -= (players[i].output_analog_1y - 128);
                players[i].output_analog_1y = 128;
            }
        }
        update_output();
    }
}

void __not_in_flash_func(update_output)(void)
{
    static bool kbModeButtonHeld = false;

    if (players[0].button_mode == BUTTON_MODE_KB) {
        gc_report = default_gc_kb_report;
    }
    else {
        gc_report = default_gc_report;
    }

    for (unsigned short int i = 0; i < playersCount; ++i)
    {
        int16_t byte = (players[i].output_buttons & 0xffff);
        bool kbModeButtonPress =
            players[i].keypress[0] == HID_KEY_SCROLL_LOCK ||
            players[i].keypress[0] == HID_KEY_F14;
        if (kbModeButtonPress)
        {
            if (!kbModeButtonHeld)
            {
                if (players[0].button_mode != BUTTON_MODE_KB)
                {
                    players[0].button_mode = BUTTON_MODE_KB;
                    players[i].button_mode = BUTTON_MODE_KB;
                    GamecubeConsole_SetMode(&gc, GamecubeMode_KB);
                    gc_report = default_gc_kb_report;
                    gc_kb_led = 0x4;
                }
                else
                {
                    players[0].button_mode = BUTTON_MODE_3;
                    players[i].button_mode = BUTTON_MODE_3;
                    GamecubeConsole_SetMode(&gc, GamecubeMode_3);
                    gc_report = default_gc_report;
                    gc_kb_led = 0;
                }
            }
            kbModeButtonHeld = true;
        }
        else
        {
            kbModeButtonHeld = false;
        }

        if (players[0].button_mode != BUTTON_MODE_KB)
        {
            gc_report.dpad_up |= ((byte & USBR_BUTTON_DU) == 0);
            gc_report.dpad_right |= ((byte & USBR_BUTTON_DR) == 0);
            gc_report.dpad_down |= ((byte & USBR_BUTTON_DD) == 0);
            gc_report.dpad_left |= ((byte & USBR_BUTTON_DL) == 0);
            gc_report.a |= ((byte & USBR_BUTTON_B2) == 0);
            gc_report.b |= ((byte & USBR_BUTTON_B1) == 0);
            gc_report.z |= ((byte & USBR_BUTTON_L1) == 0);
            gc_report.start |= ((byte & USBR_BUTTON_S2) == 0);
            gc_report.x |= ((byte & USBR_BUTTON_B4) == 0);
            gc_report.y |= (((byte & USBR_BUTTON_R1) == 0) || ((byte & USBR_BUTTON_B3) == 0));
            gc_report.l |= ((byte & USBR_BUTTON_R2) == 0);
            gc_report.r |= ((byte & USBR_BUTTON_R2) == 0);


            // *** FIX: scale relative to center instead of raw multiply
            gc_report.stick_x = furthest_from_center(
                gc_report.stick_x,
                scale_toward_center(players[i].output_analog_1x, LStick, 128),
                128);
            gc_report.stick_y = furthest_from_center(
                gc_report.stick_y,
                scale_toward_center(players[i].output_analog_1y, LStick, 128),
                128);

            gc_report.cstick_x = furthest_from_center(gc_report.cstick_x,
                players[i].output_analog_2x, 128);
            gc_report.cstick_y = furthest_from_center(gc_report.cstick_y,
                players[i].output_analog_2y, 128);
            gc_report.l_analog = furthest_from_center(gc_report.l_analog,
                players[i].output_analog_l, 0);
            gc_report.r_analog = furthest_from_center(gc_report.r_analog,
                players[i].output_analog_r, 0);

            if ((byte & USBR_BUTTON_L2) == 0) {
                gc_report.l_analog = 43;
            }
        }
        else
        {
            gc_report.keyboard.keypress[0] = gc_kb_key_lookup(players[i].keypress[2]);
            gc_report.keyboard.keypress[1] = gc_kb_key_lookup(players[i].keypress[1]);
            gc_report.keyboard.keypress[2] = gc_kb_key_lookup(players[i].keypress[0]);
            gc_report.keyboard.checksum =
                gc_report.keyboard.keypress[0] ^
                gc_report.keyboard.keypress[1] ^
                gc_report.keyboard.keypress[2] ^ gc_kb_counter;
            gc_report.keyboard.counter = gc_kb_counter;
        }
    }

    codes_task();
    update_pending = true;
}

void __not_in_flash_func(post_globals)(
    uint8_t dev_addr,
    int8_t instance,
    uint32_t buttons,
    uint8_t analog_1x,
    uint8_t analog_1y,
    uint8_t analog_2x,
    uint8_t analog_2y,
    uint8_t analog_l,
    uint8_t analog_r,
    uint32_t keys,
    uint8_t quad_x)
{
    bool is_extra = (instance == -1);
    if (is_extra) instance = 0;

    int player_index = find_player_index(dev_addr, instance);
    uint16_t buttons_pressed = (~(buttons | 0x800)) | keys;
    if (player_index < 0 && buttons_pressed)
    {
        printf("[add player] [%d, %d]\n", dev_addr, instance);
        player_index = add_player(dev_addr, instance);
    }

    if (player_index >= 0)
    {
        if (is_extra)
            players[0].altern_buttons = buttons;
        else
            players[player_index].global_buttons = buttons;

        // *** FIX: always assign analog values (don’t skip zeros)
        players[player_index].output_analog_1x = analog_1x;
        players[player_index].output_analog_1y = analog_1y;
        players[player_index].output_analog_2x = analog_2x;
        players[player_index].output_analog_2y = analog_2y;

        players[player_index].output_analog_l = analog_l;
        players[player_index].output_analog_r = analog_r;
        players[player_index].output_buttons =
            players[player_index].global_buttons & players[player_index].altern_buttons;

        players[player_index].keypress[0] = (keys) & 0xff;
        players[player_index].keypress[1] = (keys >> 8) & 0xff;
        players[player_index].keypress[2] = (keys >> 16) & 0xff;

        players[player_index].output_buttons |= USBR_BUTTON_L2;
        players[player_index].output_buttons |= USBR_BUTTON_R2;

        if (analog_r > GC_DIGITAL_TRIGGER_THRESHOLD)
            players[player_index].output_buttons &= ~USBR_BUTTON_R2;

        if (analog_l > GC_DIGITAL_TRIGGER_THRESHOLD)
            players[player_index].output_buttons &= ~USBR_BUTTON_L2;

        update_output();
    }
}

void __not_in_flash_func(post_mouse_globals)(
    uint8_t dev_addr,
    int8_t instance,
    uint16_t buttons,
    uint8_t delta_x,
    uint8_t delta_y,
    uint8_t quad_x)
{
    bool is_extra = (instance == -1);
    if (is_extra) instance = 0;

    int player_index = find_player_index(dev_addr, instance);
    uint16_t buttons_pressed = (~(buttons | 0x0f00));
    if (player_index < 0 && buttons_pressed)
    {
        printf("[add player] [%d, %d]\n", dev_addr, instance);
        player_index = add_player(dev_addr, instance);
    }

    if (player_index >= 0)
    {
        if (is_extra)
            players[0].altern_buttons = buttons;
        else
            players[player_index].global_buttons = buttons;

        players[player_index].output_buttons =
            players[player_index].global_buttons & players[player_index].altern_buttons;

        players[player_index].global_x += (int8_t)delta_x;
        players[player_index].global_y -= (int8_t)delta_y;

        if (players[player_index].global_x < 0) players[player_index].global_x = 0;
        if (players[player_index].global_y < 0) players[player_index].global_y = 0;
        if (players[player_index].global_x > 255) players[player_index].global_x = 255;
        if (players[player_index].global_y > 255) players[player_index].global_y = 255;

        players[player_index].output_analog_1x = players[player_index].global_x;
        players[player_index].output_analog_1y = players[player_index].global_y;

        update_output();
    }
}