/* 
 * Header for the DSKY project
 * Definitions and configs
 * 
 * TODO: document all implemented functions/helpers
*/

#ifndef BASE_H
#define BASE_H

// C standard libs
#include <stdlib.h>
#include <stdarg.h> // variable arguments
#include <math.h>
#include <string.h>

// STM32Duino libs
#include <libmaple/nvic.h> // stm32f1 reset
#include <EEPROM.h>
#include <HardwareTimer.h>
#include <RTClock.h>
#include <Wire.h>
#include <SPI.h>

const char TEST_STR[] PROGMEM = "M.A.S. Testing  %s";

/*===== global configs =====*/
bool led_enable = 1;
bool serial = 1;
bool debug = 1;
bool reset_EEPROM = 1;
bool reset_conf = 1;
bool MPU_enabled = 1;

/* ===== programmable configs ===== */
#define MAX_PASS_LEN 16
#define MAX_PASS_FAILS 10
#define MAX_NAME_LEN 10
typedef struct CT_Config
{
    // system configs
    bool splash = 0;
    bool fancy = 0;
    int fancy_delay = 20;                       // in milliseconds
    char req_pass = 0;                          // require password
    char admin_pass[MAX_PASS_LEN + 1] = "0042"; // admin password
    char device_name[MAX_NAME_LEN + 1] = "mas-tp00x";
    int wrong_admin_pass_count = 0; // number of failed password attempts
    // app configs
    int placeholder;
} CT_Config;
const uint16_t CONFIG_ADDRESS = 0x0;
const int CONFIG_LEN = sizeof(CT_Config);

/* ===== SD Card config ===== */
#define SD_CS_PIN PA4

/* ===== LED configs ===== */
#define LED_STATUS PC13
#define LED_WAIT 7
#define LED_IO 6
#define LED_INT 5

/* ===== display configs ===== */
#define D_COLS 16
#define D_ROWS 4
#define LCD_RS PA2
#define LCD_EN PA3
#define LCD_D4 PB11
#define LCD_D5 PB10
#define LCD_D6 PB1
#define LCD_D7 PB0

/* ===== keypad configs ===== */
const byte KEYPAD_ROWS = 4, KEYPAD_COLS = 4;
const char KEYPAD_KEYS[KEYPAD_ROWS][KEYPAD_COLS] = { // Define the Keymap
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}};
byte KEYPAD_ROW_PINS[KEYPAD_ROWS] = {PA15, PB3, PB4, PB5};
byte KEYPAD_COL_PINS[KEYPAD_COLS] = {PB12, PB13, PB14, PB15};

const int DEFAULT_DELAY_TIME = 1000;

/* ===== sysutils configs ===== */

// menu function configs
#define M_UP_KEY 'A'
#define M_DOWN_KEY 'B'
#define M_MENU_KEY 'C'
#define M_EXIT_KEY 'D'
#define M_CLEAR_KEY '*'
#define M_ENTER_KEY '#'

// Editor configs
#define DEFAULT_BUFSIZE 64
#define MAX_BUFSIZE 1024
/* modes
    0: 'View' (normal) mode, view null-terminated strings
    1: 'Force' mode, view entire memory region given by bufsize
*/
const char ED_MODES[] = {'V', 'F'};
// view window menus (note the null!)
const int ED_MENU_LEN = 4;
const char *ED_MENU[] = {
    "Save buffer",
    "Revert changes",
    "Settings",
    "Exit"};
const int ED_SETTINGS_LEN = 2;
const char *ED_SETTINGS[] = {
    "Force Mode?",
    "Insert/Replace?",
};
#define ED_MENU_KEY 'C'
#define ED_EXIT_KEY 'D'
#define ED_EDIT_KEY '#'

// viewing mode configs
#define VW_MODE_KEY 'A'
#define VW_HEX_KEY 'B'
#define VW_LEFT_KEY '4'
#define VW_RIGHT_KEY '6'
#define VW_UP_KEY '2'
#define VW_DOWN_KEY '8'
#define VW_HOME_KEY '1'
#define VW_END_KEY '3'

/* input modes
    0: 'Insert' mode
    1: 'Replace' mode
*/
const char IW_MODES[] = {'I', 'R'};
/* 4 input modes -- Dec, Hex, Keypad, ASCII
    0: Dec: decimal/calculator input (see IW_DEC_MAP)
    1: Hex: use Shift to input A-F (see IW_HEX_MAP)
    2: Keypad: basic 9-key alphanumeric input
    3: ASCII: Hexidecimal ASCII
*/
#define IW_MODE_KEY 'A'
#define IW_SHIFT_KEY 'B'
#define IW_DEL_KEY '*'
#define IW_KEYPAD_WAIT 500 // rough, in milliseconds
const char IW_INPUT_METHODS[] = {'D', 'H', 'K', 'A'};
const char IW_DEC_MAP[] = {'+', '-', '*', '/', '.', '='};
const char IW_HEX_MAP[] = {'A', 'B', 'C', 'D', 'E', 'F', 'x'};
const char IW_KEYPAD_MAP[10][6] = { // null-terminate for wrapping
    {'0', ' ', ',', '.'},
    {'1', '!', '?', '"'},
    {'2', 'a', 'b', 'c'},
    {'3', 'd', 'e', 'f'},
    {'4', 'g', 'h', 'i'},
    {'5', 'j', 'k', 'l'},
    {'6', 'm', 'n', 'o'},
    {'7', 'p', 'q', 'r', 's'},
    {'8', 't', 'u', 'v'},
    {'9', 'w', 'x', 'y', 'z'}};

#endif //BASE_H
