/*
   Main file for the DSKY project; using serial input for now in place
   of the matrix keypad.
*/

// include the library code:
#include "system.hpp"
#include "functions.hpp"


void setup() {

  // set up outputs
  lcd.begin(16, 2);
  if (serial) {
    Serial.begin(9600);
    Serial.println("Starting serial");
  }

  // get configs from flash storage
  if (reset_conf) { // reset; read from default and write to flash
    EEPROM.format();
    conf = new ct_config;      // Note: config_len always even
    ee_write(config_address, (byte*)conf, config_len);
  } else {  // read from flash into memory
    conf = (ct_config*)malloc(config_len);  // allocate on heap!
    ee_read(config_address, (byte*)conf, config_len);
  }

  // LCD backlight
  pinMode(backlight_pin, OUTPUT);
  digitalWrite(backlight_pin, (conf->lcd_backlight) ? HIGH : LOW);

  // Print welcome message and request password
  if (conf->splash) {
    fancy_print("M.A.S. Testing");
    delay(2000);
  }

  // handle bootup password
  while (conf->req_pass) {
    int8_t res = password(conf->admin_pass);
    lcd.clear();
    if (res == 1) break;  // correct
    else if (res == -1) fancy_print("PASSWD INCORRECT");
    else if (res == -2) {
      fancy_print("PASSWORD REQ'D");
      lcd.setCursor(0, 1); fancy_print("FOR LOGIN");
    }
    delay(1000);
  }
  if (conf->splash) {
    fancy_print("    Welcome!    ");
    delay(1000);
  }
  lcd.clear();
}

void loop() {

  // main menu
  main_menu();

  // handle menu exit
}


/*===== helper functions =====*/

// menu handling
int main_menu() {

  // debug PROGMEM

  lcd.clear(); fancy_print("Input test...");
  delay(1000);
  char input[64] = "";
  int res = input_window(input, 64, 0, NULL);
  lcd.clear(); (res == 1) ? fancy_print("OK") : fancy_print("Exited!");
  lcd.setCursor(0, 1); fancy_print(input);
  delay(2000);


  // TODO: implement

}


// view window: character viewer
// Note: updates entire screen on every keystroke.
int view_window(char* buf, int bufsize,
                const uint8_t vw_mode, const char* prompt) {
  if (bufsize == 0) bufsize = strlen(buf);
  if (!buf || bufsize < 1) return -2;

  // setup vars
  uint8_t mode = 0;  // 2 modes: 1-line scrolling, fullscreen
  uint8_t hex = 0;   // hex display enable
  int buf_pos = 0;   // buffer position at cursor
  int d_root = 0;    // buffer position at leftmost of screen
  int d_pos[2] = {0, 1};  // cursor position on screen
  // limit buffer position according to vw_mode
  int max_buf_pos = (vw_modes[vw_mode] == 'F') ? bufsize : strlen(buf);

  // loop until command to exit
  lcd.blink();
  while (true) {

    /*===== handle display =====*/
    lcd.clear();
    if (mode == 0) {  // 1-line horizontal scrolling
      d_pos[1] = 1;  // snap cursor to the second line
      // prompt
      if (prompt) lcd.print(prompt);
      else lcd.print(vw_modes[vw_mode]); lcd.print(':');
      // display ASCII for the current byte
      lcd.setCursor(10, 0); lcd.print("0x"); hex_print(buf[buf_pos]);

      // rolling window
      lcd.setCursor(0, 1);
      if (hex) {  // hex mode
        lcd.setCursor(1, 1);
        for (int i = 0; i < 5 &&
      }
    else if (vw_modes[vw_mode] == 'F') { // force display all characters
        for (int i = 0; i < 15 && d_root + i < bufsize; i++) {
          if (buf[d_root + i] == 0) lcd.write(' '); // use sp for 0
          else lcd.write(buf[d_root + i]);
        }
      }
      else { // use lcd.print for convenience
        char temp[15]; strncpy(temp, buf + d_root, 15); // len <= 15
        lcd.print(temp);   // faster than displaying large bufs
      }
    }
    else if (mode == 1) {  // fullscreen
      if (hex) {
          
      }
    }
    lcd.setCursor(d_pos[0], d_pos[1]);

    /*===== handle commands =====*/
    char ch = keypad_in();

  } // loop


}


// input window
int input_window(char* buf, int bufsize,
                 const uint8_t iw_mode, const char* prompt) {
  if (bufsize == 0) bufsize = strlen(buf);  // watch for bad buf
  if (bufsize > max_bufsize) bufsize = max_bufsize;
  if (!buf || bufsize < 1) return -2;

  // create and init input buffer
  char* in_buf = (char*)calloc(bufsize + 1, 1); // staged input buf w/null
  strncpy(in_buf, buf, bufsize);
  int buf_pos = strlen(in_buf);       // cursor positions
  int d_pos = strlen(in_buf) > 15 ? 15 : strlen(in_buf);

  // setup vars
  uint8_t mode = 0;
  uint8_t shift = 0; // 0 for off, 1 for on
  char ascii_buf[4] = {}; uint8_t ascii_count = 0; // buf for ASCII mode
  char ch = 0;
  lcd.blink();

  // input loop
  while (buf_pos <= bufsize) {  // full buffer permissible

    /*===== handle display =====*/
    lcd.clear(); lcd.setCursor(0, 0);
    // prompt
    if (prompt) lcd.print((const char*)prompt); // if given prompt
    else {                                  // else display iw_mode
      lcd.print(iw_modes[iw_mode]); lcd.print(':');
      lcd.print(buf_pos); lcd.print('/'); lcd.print(bufsize);
    }

    // display input mode information
    if (input_modes[mode] == 'A') {  // handle ASCII mode displays
      lcd.setCursor(10, 0); lcd.print("0x__");
      lcd.setCursor(12, 0); lcd.print(ascii_buf);
    }
    lcd.setCursor(14, 0);
    lcd.print(input_modes[mode]);  // mode
    shift ? lcd.print('S') : lcd.print(' ');  // shift

    // show full buffer
    lcd.setCursor(15, 1);
    if (buf_pos == bufsize) lcd.print('!');
    else lcd.print(' ');

    // always display the last 16 bytes in in_buf
    // Note: always leave one space on the right side
    lcd.setCursor(0, 1);
    if (strlen(in_buf) < 15) {  // less than 15 chars
      lcd.print(in_buf);
      d_pos = strlen(in_buf);
    }
    else {  // more than 15 characters
      char temp[16];  // faster than displaying a long buffer
      strncpy(temp, in_buf + strlen(in_buf) - 15, 15);
      lcd.print(temp);
      d_pos = 15;
    }
    lcd.setCursor(d_pos, 1);


    /*===== get key =====*/
    ch = keypad_in();

    /*===== handle control chars =====*/
    if (ch == mode_key) {
      // only Dec and Hex modes in Numeric mode
      if (iw_mode == 2)  mode = (mode) ? 0 : 1;
      else (mode == sizeof(input_modes) - 1) ? mode = 0 : mode ++;
    }
    else if (ch == shift_key) {
      shift = shift ? 0 : 1;
      continue;  // don't reset ascii buffer
    }
    else if (ch == clear_key) {
      memset(in_buf, 0, bufsize); // clear input buffer
      buf_pos = 0; d_pos = 0;
      lcd.setCursor(0, 1); lcd.print("                ");
      lcd.setCursor(0, 1);
    }
    else if (ch == exit_key) {
      lcd.noBlink();
      free(in_buf); return -1;
    }
    else if (ch == del_key) {
      // Note: case to clear ASCII mode buffer on press
      if (input_modes[mode] == 'A' && ascii_count > 0);  //nop
      else if (buf_pos > 0) {  // prevent reverse overflow
        in_buf[--buf_pos] = 0;  // erase last character in in_buf
        lcd.setCursor(--d_pos, 1);  // go back 1 position
        lcd.print(' '); lcd.setCursor(d_pos, 1);
      }
    }
    else if (ch == enter_key) {
      if (iw_mode == 0) strncpy(buf, in_buf, bufsize);  // flush to buf
      else strncpy(buf, in_buf, strlen(in_buf));  // DON'T null terminate
      lcd.noBlink();
      free(in_buf); return 1;
    }

    /*===== handle input to in_buf =====*/
    // Note: catch non-digit characters; catch buffer overflow
    else if (buf_pos < bufsize && isdigit(ch)) {

      if (input_modes[mode] == 'D') {  // Decimal-Calculator Mode
        if (shift && (ch - 1 - '0') < sizeof(dec_map)) {  //shift on
          in_buf[buf_pos] = dec_map[ch - 1 - '0'];
        } else {      // shift off
          in_buf[buf_pos] = ch;
        }
      }

      else if (input_modes[mode] ==  'H') {  // Hex Mode
        if (shift && (ch - 1 - '0') < sizeof(hex_map)) {  // shift on
          in_buf[buf_pos] = hex_map[ch - 1 - '0'];
        } else {      // shift off
          in_buf[buf_pos] = ch;
        }
      }

      else if (input_modes[mode] == 'A') {  // ASCII Mode
        if (shift && (ch - 1 - '0') < sizeof(hex_map)) {  // shift on
          ascii_buf[ascii_count++] = hex_map[ch - 1 - '0'];
        } else {      // shift off
          ascii_buf[ascii_count++] = ch;
        }
        if (ascii_count < 2) continue; // NOT updating display
        else {    // have all 2 hex digits of the ASCII
          lcd.setCursor(12, 0); lcd.print(ascii_buf);
          delay(400);
          in_buf[buf_pos] = (char)strtol(ascii_buf, NULL, 16);
        }
      }

      else if (input_modes[mode] == 'K') {  // Keypad Mode (self-contained)
        // init
        lcd.setCursor(11, 0); lcd.print("[ ]");
        char* key_map = (char*) keypad_map[ch - '0'];  // get keymap
        int k = 0, wait_count = 0;
        // loop
        while (wait_count < keypad_wait) {
          lcd.setCursor(12, 0);
          if (k != 0 && shift) lcd.print((char)(key_map[k] - 32));
          else lcd.print(key_map[k]);
          char k_ch = kpd.getKey(); // get raw char
          if (isdigit(k_ch)) {  // catch non-numeric chars
            wait_count = 0;
            if (k_ch == ch) {
              k = (key_map[k + 1] == 0) ? 0 : k + 1;
            }
            else  {  // new input
              ch = k_ch; k = 0;
              key_map = (char*) keypad_map[ch - '0'];
            }
          }
          lcd.setCursor(d_pos, 1); // just aesthetics
          delay(5); wait_count += 5;  // NOTE: careful w/the Keypad lib
        }
        in_buf[buf_pos] = (k != 0 && shift) ? key_map[k] - 32 : key_map[k];
      }

      else {  // erorr: mode not defined
        lcd.clear(); lcd.print("Error");
        lcd.setCursor(0, 1); lcd.print("Inpt Mthd DNE");
        delay(2000);
        free(in_buf); return -1;
      }
      buf_pos ++;
      d_pos = (d_pos < 15) ? d_pos + 1 : d_pos; // cap d_pos at 15

    }

    // clear ASCII mode buffer
    memset(ascii_buf, 0, 4); ascii_count = 0;

  }  // end input loop

  // Error: didn't enter loop

}


// dedicated general purpose password prompt
int password(const char* true_pass) {
  if (!true_pass) return -1;
  // prompt
  lcd.clear();
  lcd.setCursor(0, 0); fancy_print("Passwd:");
  lcd.setCursor(0, 1); fancy_print(" *:Exit #:Enter ");
  lcd.setCursor(7, 0); lcd.blink();

  // get password
  int count = 0;
  char pass_buf[max_pass_len] = {};
  while (count < max_pass_len) {
    char ch = keypad_in();
    if (ch == '*') return -2;
    if (ch == '#') break;
    pass_buf[count] = ch;
    lcd.print('*');
    count ++;
  }
  lcd.noBlink();

  // check password
  if (count != strlen(true_pass)) return -1;
  for (count = 0; count < strlen(true_pass); count++)
    if (pass_buf[count] != true_pass[count]) return -1;

  return 1;
}


// print helpers
void fancy_print(const char* buf) {
  if (!buf) return;
  if (!conf->fancy) {
    lcd.print(buf);
    return;
  }
  int len = strlen(buf);
  for (int i = 0; i < len; i++) {
    lcd.print(buf[i]);
    delay(conf->fancy_delay);
  }
}
void fancy_print(const long num) {
  char temp[12] = {};  // max 12 digits
  sprintf(temp, "%d", num);
  fancy_print(temp);
}
void fancy_print(const double num) {
  char temp[12] = {};  // max 12 digits
  sprintf(temp, "%f", num);
  fancy_print(temp);
}
void hex_print(const byte data) {
  char temp[2];
  sprintf(temp, "%h", (uint8_t)data);
  lcd.print(temp);
}
void hex_print(const char* data, int num) {
  for (int i = 0; i < num; i++) {
    hex_print(data[i]);
  }
}


// read input character
char keypad_in() {
  while (true) {  //TODO: hanging the entire system!
    if (serial) {  // Note: serial input takes precedence if enabled
      unsigned char ch_s = Serial.read();
      if (ch_s != 255) return ch_s;
    }
    unsigned char ch_k = kpd.getKey();
    if (ch_k != 0) return ch_k;
    delay(5);
  }
}


/*===== Emulated EEPROM handling ===== */
// Note: possible 1-byte overflow; make sure num is even
// Note: ptr is unprotected; know what you're doing
void ee_write(uint16_t address, byte* ptr, int num) {
  uint16_t buf;  // 2-byte buffer for writing
  while (num > 0) {
    buf = *(uint16_t*)ptr;
    EEPROM.write(address, buf);
    address ++; ptr += 2; num -= 2;
  }
}
// Note: possible 1-byte overflow; make sure num is even
// Note: ptr is unprotected; know what you're doing
byte* ee_read(uint16_t address, byte* ptr, int num) {
  uint16_t buf;  // 2-byte buffer for reading
  byte* orig_ptr = ptr;
  while (num > 0) {
    EEPROM.read(address, &buf);
    *(uint16_t*)ptr = buf;
    address ++; ptr += 2; num -= 2;
  }
  return orig_ptr;  // for convenience
}


