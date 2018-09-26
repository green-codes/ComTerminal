/*
   Header for the DSKY project
   Definitions and configs

   TODO: document all implemented functions/helpers
*/

#ifndef CONFIG_H
#define CONFIG_H

#include <string.h>
#include <stdlib.h>

#include <LiquidCrystal.h>
#include <Key.h>
#include <Keypad.h>
#include <EEPROM.h>

#include <Wire.h>
#include <SPI.h>

/*===== debug configs =====*/
uint8_t serial = 0;
uint8_t debug = 1;

/*===== general configs =====*/

// struct for programmable configs
uint8_t reset_conf = 1;
typedef struct
{
    uint8_t lcd_backlight = 1;
    uint8_t splash = 0;
    uint8_t fancy = 1;
    int fancy_delay = 20;        // in milliseconds
    char req_pass = 0;           // require password
    char admin_pass[5] = "0042"; // admin password (note the null!)
} CT_Config;
const uint16_t CONFIG_ADDRESS = 0x0;
const int CONFIG_LEN = sizeof(CT_Config);

const int MAX_PASS_LEN = 20;       // maximum password length
const int D_COLS = 16, D_ROWS = 2; // display dimensions

// pin configs
const int LCD_RS = PA2, LCD_EN = PA3,
          LCD_D4 = PB11, LCD_D5 = PB10, LCD_D6 = PB1, LCD_D7 = PB0;
const int STATUS_LED = PC13;
const int WORK_LED = PC14;

// keypad config
const byte KEYPAD_ROWS = 4, KEYPAD_COLS = 4;
const char KEYPAD_KEYS[KEYPAD_ROWS][KEYPAD_COLS] = { // Define the Keymap
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}};
byte KEYPAD_ROW_PINS[KEYPAD_ROWS] = {PB15, PB14, PB13, PB12};
byte KEYPAD_COL_PINS[KEYPAD_COLS] = {PB5, PB4, PB3, PA15};

// menu configs
const int MAIN_MENU_LEN = 0;
const char *MAIN_MENU[] = {

};
const char M_UP_KEY = 'A';
const char M_DOWN_KEY = 'B';
const char M_ENTER_KEY = 'C';
const char M_EXIT_KEY = 'D';
const char M_CLEAR_KEY = '*';
const char M_ENTER_KEY2 = '#';

// view window configs
/* view modes
    0: 'View' (normal) mode, view null-terminated strings
    1: 'Force' mode, view entire memory region given by bufsize
*/
const char ED_MODES[] = {'V', 'F'};
// view window menus (note the null!)
const int VW_MENU_LEN = 4;
const char *VW_MENU[] = {
    "Edit from here",
    "Edit entire buf",
    "Settings",
    "Exit"};
const int VW_SETTINGS_LEN = 2;
const char *VW_SETTINGS[] = {
    "Force Display?",
    "Edit 0-Fill?"};
const char VW_MODE_KEY = 'A';
const char VW_HEX_KEY = 'B';
const char VW_MENU_KEY = 'C';
const char VW_EXIT_KEY = 'D';
const char VW_LEFT_KEY = '4';
const char VW_RIGHT_KEY = '6';
const char VW_UP_KEY = '2';
const char VW_DOWN_KEY = '8';
const char VW_HOME_KEY = '1';
const char VW_END_KEY = '3';
const char VW_ENTER_KEY = '5';

// input window configs
const int DEFAULT_BUFSIZE = 64, MAX_BUFSIZE = 1023;
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
const char IW_INPUT_METHODS[] = {'D', 'H', 'K', 'A'};
const char IW_MODE_KEY = 'A';
const char IW_SHIFT_KEY = 'B';
const char IW_CLEAR_KEY = 'C';
const char IW_EXIT_KEY = 'D';
const char IW_DEL_KEY = '*';
const char IW_ENTER_KEY = '#';
const char IW_DEC_MAP[] = {'+', '-', '*', '/', '.', '='};
const char IW_HEX_MAP[] = {'A', 'B', 'C', 'D', 'E', 'F', 'x'};
const int IW_KEYPAD_WAIT = 500;     // rough, in milliseconds
const char IW_KEYPAD_MAP[10][6] = { // null-terminate for wrapping
    {'0', ' ', ',', '.'},
    {'1'},
    {'2', 'a', 'b', 'c'},
    {'3', 'd', 'e', 'f'},
    {'4', 'g', 'h', 'i'},
    {'5', 'j', 'k', 'l'},
    {'6', 'm', 'n', 'o'},
    {'7', 'p', 'q', 'r', 's'},
    {'8', 't', 'u', 'v'},
    {'9', 'w', 'x', 'y', 'z'}};

/*===== global vars =====*/
LiquidCrystal lcd(LCD_RS, LCD_EN, LCD_D4, LCD_D5, LCD_D6, LCD_D7); // see docs
Keypad kpd = Keypad(makeKeymap(KEYPAD_KEYS), KEYPAD_ROW_PINS, KEYPAD_COL_PINS,
                    KEYPAD_ROWS, KEYPAD_COLS);
CT_Config *conf;                       // pointer to config struct
unsigned long work_started_millis = 0; // record start time for last task

/*===== sysutil functions =====*/

/* generic menu
    Returns the selected entry, or -1 if user exits menu
*/
int menu(const char **items, int num_items, int default_pos, char *prompt);

/* view window: basic character viewer
    Params
      bufsize: 0 for strlen(buf)
    Returns
      1: success
      -1: user exit
      -2: failure
*/
int buffered_editor(char *buf, int bufsize, uint8_t read_only,
                    uint8_t ed_mode, const char *prompt);

/* input window
   Params
    bufsize: max number of bytes we can modify
      if 0, use strlen(buf) to be safe
    iw_mode: input modes
    prompt: string prompt; ignored if NULL
   Returns TODO
    1: success
    -1: user exit
    -2: failure
*/
int input_window(char *buf, int bufsize,
                 uint8_t iw_mode, const char *prompt);

// password handling
// returns 1 for correct password, -1 for wrong ones
int password(const char *true_pass);

// print lines to LCD from buffer
void print_lines(char *const buf, int bufsize, uint8_t force,
                 int num_lines, int start_row);

// simple message display
void print_message(const char *buf, int message_delay);

// view char buffers fancily (full screen, no force)
int fancy_view(char *buf, int bufsize, int roll_delay, int end_delay);

// basic print functions
void fancy_print(const char *buf);
void fancy_print(const int num);
void hex_print(const char data);
void hex_print(const char *data, int num);

// read input character
char keypad_in();

/*===== Emulated EEPROM handling =====
   the EEPROM library manages a virtual address space starting on 0x0 */
// Note: possible 1-byte overflow; make sure num is even
// Note: ptr is unprotected; know what you're doing
void ee_write(uint16_t address, byte *ptr, int num);
byte *ee_read(uint16_t address, byte *ptr, int num);

#endif //CONFIG_H
