/* console.h */

#ifndef CONSOLE_H
#define CONSOLE_H

typedef enum Key
{
    KEY_NONE = 0,

    /* Printable ASCII (32-126) */

    KEY_SPACE          = 32,
    KEY_EXCLAMATION    = 33,  /* ! */
    KEY_DOUBLE_QUOTE   = 34,  /* " */
    KEY_HASH           = 35,  /* # */
    KEY_DOLLAR         = 36,  /* $ */
    KEY_PERCENT        = 37,  /* % */
    KEY_AMPERSAND      = 38,  /* & */
    KEY_APOSTROPHE     = 39,  /* ' */
    KEY_LEFT_PAREN     = 40,  /* ( */
    KEY_RIGHT_PAREN    = 41,  /* ) */
    KEY_ASTERISK       = 42,  /* * */
    KEY_PLUS           = 43,  /* + */
    KEY_COMMA          = 44,  /* , */
    KEY_MINUS          = 45,  /* - */
    KEY_PERIOD         = 46,  /* . */
    KEY_SLASH          = 47,  /* / */

    KEY_0              = 48,
    KEY_1              = 49,
    KEY_2              = 50,
    KEY_3              = 51,
    KEY_4              = 52,
    KEY_5              = 53,
    KEY_6              = 54,
    KEY_7              = 55,
    KEY_8              = 56,
    KEY_9              = 57,

    KEY_COLON          = 58,
    KEY_SEMICOLON      = 59,
    KEY_LESS           = 60,
    KEY_EQUAL          = 61,
    KEY_GREATER        = 62,
    KEY_QUESTION       = 63,
    KEY_AT             = 64,

    KEY_A              = 65,
    KEY_B              = 66,
    KEY_C              = 67,
    KEY_D              = 68,
    KEY_E              = 69,
    KEY_F              = 70,
    KEY_G              = 71,
    KEY_H              = 72,
    KEY_I              = 73,
    KEY_J              = 74,
    KEY_K              = 75,
    KEY_L              = 76,
    KEY_M              = 77,
    KEY_N              = 78,
    KEY_O              = 79,
    KEY_P              = 80,
    KEY_Q              = 81,
    KEY_R              = 82,
    KEY_S              = 83,
    KEY_T              = 84,
    KEY_U              = 85,
    KEY_V              = 86,
    KEY_W              = 87,
    KEY_X              = 88,
    KEY_Y              = 89,
    KEY_Z              = 90,

    KEY_LEFT_BRACKET   = 91,
    KEY_BACKSLASH      = 92,
    KEY_RIGHT_BRACKET  = 93,
    KEY_CARET          = 94,
    KEY_UNDERSCORE     = 95,
    KEY_GRAVE          = 96,

    KEY_a              = 97,
    KEY_b              = 98,
    KEY_c              = 99,
    KEY_d              = 100,
    KEY_e              = 101,
    KEY_f              = 102,
    KEY_g              = 103,
    KEY_h              = 104,
    KEY_i              = 105,
    KEY_j              = 106,
    KEY_k              = 107,
    KEY_l              = 108,
    KEY_m              = 109,
    KEY_n              = 110,
    KEY_o              = 111,
    KEY_p              = 112,
    KEY_q              = 113,
    KEY_r              = 114,
    KEY_s              = 115,
    KEY_t              = 116,
    KEY_u              = 117,
    KEY_v              = 118,
    KEY_w              = 119,
    KEY_x              = 120,
    KEY_y              = 121,
    KEY_z              = 122,

    KEY_LEFT_BRACE     = 123,
    KEY_PIPE           = 124,
    KEY_RIGHT_BRACE    = 125,
    KEY_TILDE          = 126,

    /* Non-ASCII */

    KEY_ESCAPE = 256,

    KEY_ENTER,
    KEY_TAB,
    KEY_BACKSPACE,

    KEY_INSERT,
    KEY_DELETE,

    KEY_RIGHT,
    KEY_LEFT,
    KEY_DOWN,
    KEY_UP,

    KEY_HOME,
    KEY_END,

    KEY_PAGE_UP,
    KEY_PAGE_DOWN,

    KEY_CAPS_LOCK,
    KEY_SCROLL_LOCK,
    KEY_NUM_LOCK,
    KEY_PRINT_SCREEN,
    KEY_PAUSE,

    KEY_F1,
    KEY_F2,
    KEY_F3,
    KEY_F4,
    KEY_F5,
    KEY_F6,
    KEY_F7,
    KEY_F8,
    KEY_F9,
    KEY_F10,
    KEY_F11,
    KEY_F12,
    KEY_F13,
    KEY_F14,
    KEY_F15,
    KEY_F16,
    KEY_F17,
    KEY_F18,
    KEY_F19,
    KEY_F20,
    KEY_F21,
    KEY_F22,
    KEY_F23,
    KEY_F24,

    /* Numeric keypad */

    KEY_KP_0,
    KEY_KP_1,
    KEY_KP_2,
    KEY_KP_3,
    KEY_KP_4,
    KEY_KP_5,
    KEY_KP_6,
    KEY_KP_7,
    KEY_KP_8,
    KEY_KP_9,

    KEY_KP_DECIMAL,
    KEY_KP_DIVIDE,
    KEY_KP_MULTIPLY,
    KEY_KP_SUBTRACT,
    KEY_KP_ADD,
    KEY_KP_ENTER,
    KEY_KP_EQUAL,

    /* Modifiers */

    KEY_LEFT_SHIFT,
    KEY_RIGHT_SHIFT,

    KEY_LEFT_CONTROL,
    KEY_RIGHT_CONTROL,

    KEY_LEFT_ALT,
    KEY_RIGHT_ALT,

    KEY_LEFT_SUPER,
    KEY_RIGHT_SUPER,

    KEY_MENU,

    KEY_MAX
} key_t;

/* Returns 0 on success */
int console_init(void);

/* Restores terminal state */
void console_shutdown(void);

/* Returns 1 if a key is waiting, 0 otherwise */
int console_key_available(void);

/* Returns:
 *   >= 0 : character read
 *   -1   : no character available
 */
key_t console_read_key(void);
key_t console_poll_key(void);

#endif