/*
   Header for the DSKY project
   Definitions and configs

   TODO: document all implemented functions/helpers
*/

#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <string.h>
#include <stdlib.h>

#include <LiquidCrystal.h>
#include <Key.h>
#include <Keypad.h>
#include <EEPROM.h>

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
} ct_config;
const uint16_t config_address = 0x0;
const int config_len = sizeof(ct_config);

const int max_pass_len = 20;       // maximum password length
const int d_cols = 16, d_rows = 2; // display dimensions

// pin configs
//const int rs = 7, en = 8, d4 = 9, d5 = 10, d6 = 11, d7 = 12;
const int rs = PA2, en = PA3, d4 = PB11, d5 = PB10, d6 = PB1, d7 = PB0;
const int backlight_pin = PA1; // Note: pulling 30mA while on

// keypad config
const byte ROWS = 4, COLS = 4;
const char keys[ROWS][COLS] = { // Define the Keymap
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}};
byte rowPins[4] = {PA15, PB3, PB4, PB5};
byte colPins[COLS] = {PB12, PB13, PB14, PB15};

// menu configs
const int main_menu_len = 0;
const char *main_menu[] = {

};
const char m_up_key = 'A';
const char m_down_key = 'B';
const char m_enter_key = 'C';
const char m_exit_key = 'D';
const char m_clear_key = '*';
const char m_enter_key2 = '#';

// view window configs
/* view modes
    0: 'View' (normal) mode, view null-terminated strings
    1: 'Force' mode, view entire memory region given by bufsize
*/
const char vw_modes[] = {'V', 'F'};
// view window menus (note the null!)
const int vw_menu_len = 4;
const char *vw_menu[] = {
    "Edit from here",
    "Edit entire buf",
    "Settings",
    "Exit"};
const int vw_settings_len = 1;
const char *vw_settings[] = {
    "Force Display?",
    "Edit 0-Fill?"};
const char vw_mode_key = 'A';
const char vw_hex_key = 'B';
const char vw_menu_key = 'C';
const char vw_exit_key = 'D';
const char vw_left_key = '4';
const char vw_right_key = '6';
const char vw_up_key = '2';
const char vw_down_key = '8';
const char vw_home_key = '1';
const char vw_end_key = '3';
const char vw_enter_key = '5';

// input window configs
const int default_bufsize = 64, max_bufsize = 128;
/* input modes
    0: 'Input' (normal) mode, will 0-fill end of buf
    1: 'Replace' mode, won't 0-fill end of buf
    2: 'Numeric' mode, only the Decimal input method is enabled
*/
const char iw_modes[] = {'I', 'R', 'N'};
/* 4 input modes -- Dec, Hex, ASCII
    Dec: decimal/calculator input (Shift: + - * / to 1234)
    Hex: use Shift to input A-F (using 0-6 keys), 8='x'
    Keypad: 9-key alphanumeric input
    ASCII: Decimal(0-127) ASCII
*/
const char input_modes[] = {'D', 'H', 'K', 'A'};
const char iw_mode_key = 'A';
const char iw_shift_key = 'B'; // used only in Hex mode
const char iw_clear_key = 'C';
const char iw_exit_key = 'D';
const char iw_del_key = '*';
const char iw_enter_key = '#';
const char iw_dec_map[] = {'+', '-', '*', '/', '.', '='};
const char iw_hex_map[] = {'A', 'B', 'C', 'D', 'E', 'F', 'x'};
const int iw_keypad_wait = 500;     // rough, in milliseconds
const char iw_keypad_map[10][6] = { // null-terminate for wrapping
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
LiquidCrystal lcd(rs, en, d4, d5, d6, d7); // see docs
Keypad kpd = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);
ct_config *conf; // pointer to config struct

/*===== helper functions =====*/

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
int view_window(char *buf, int bufsize, uint8_t read_only,
                uint8_t vw_mode, const char *prompt);

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

// view char buffers fancily
int fancy_view(char *buf, int bufsize, int roll_delay, int end_delay);

// simple message display
void lcd_message(const char *buf, int message_delay);

// fancy_print
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

#endif //CONFIG_HPP
