/*
   Main file for the DSKY project; using serial input for now in place
   of the matrix keypad.
*/

// include the library code:
#include "DSKY.hpp"
#include "functions.hpp"


void setup() {

  // enable LCD backlight
  pinMode(backlight_pin, OUTPUT);
  digitalWrite(backlight_pin, HIGH);

  // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);

  // TODO: using Serial for testing, change later
  Serial.begin(9600);
  Serial.println("Begin test!");

  // Print welcome message and request password
  if (splash) {
    fancy_print("M.A.S. Testing");
    delay(2000);
  }

  // handle bootup password
  while (req_pass) {
    int8_t res = password(admin_pass);
    lcd.clear();
    if (res == 1) break;  // correct
    else if (res == -1) fancy_print("PASSWD INCORRECT");
    else if (res == -2) {
      fancy_print("PASSWORD REQ'D");
      lcd.setCursor(0, 1); fancy_print("FOR LOGIN");
    }
    delay(1000);
  }
  if (splash) {
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

  lcd.clear(); fancy_print("Input test...");
  delay(1000);
  char input[64] = "";
  int res = input_window(input, 64, 0, NULL);
  lcd.clear(); (res == 1) ? fancy_print("OK") : fancy_print("Failed!");
  lcd.setCursor(0, 1); fancy_print(input);
  delay(2000);


  // TODO: implement

}


// view window: buffer viewer
int view_window(char* buf, int bufsize,
                const uint8_t vw_mode, const char* prompt) {
  if (bufsize == 0) bufsize = strlen(buf);
  if (!buf || bufsize < 1) return -2;

  // setup vars
  uint8_t mode = 0;  // 2 modes: 1-line scrolling and full screen
  int buf_pos = 0, d_pos = 0;
  int max_buf_pos = (vw_mode == 1) ? bufsize : strlen(buf);

  // loop until command to exit
  while (true) {

    /*===== handle display =====*/
    lcd.clear(); lcd.setCursor(0, 0);
    // prompt
    if (prompt) fancy_print(prompt);
    else lcd.print(vw_modes[vw_mode]);


    /*===== handle commands =====*/
    char ch = keypad_in();



  }

}

// input window: buffer editor
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
    else lcd.print(iw_modes[iw_mode]); // else display iw_mode
    lcd.print(buf_pos); lcd.print('/'); lcd.print(bufsize);

    // display input mode information
    if (input_modes[mode] == 'A') {  // handle ASCII mode displays
      lcd.setCursor(9, 0); lcd.print("0x  ");
      lcd.setCursor(11, 0); lcd.print(ascii_buf);
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
      lcd.print((char*)(in_buf + strlen(in_buf) - 15));
      d_pos = 15;
    }


    /*===== get key =====*/
    ch = keypad_in();

    /*===== handle control chars =====*/
    if (ch == mode_key) {
      // no input method switching in numeric mode
      if (iw_mode == 2)  mode = 0;
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
      if (buf_pos > 0) {  // prevent reverse overflow
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
        lcd.setCursor(11, 0); lcd.print(ascii_buf);
        if (ascii_count < 2) continue; // NOT updating display
        else {    // have all 2 hex digits of the ASCII
          delay(200);
          in_buf[buf_pos] = (char)strtol(ascii_buf, NULL, 16);
        }
      }

      else if (input_modes[mode] == 'K') {  // Keypad Mode (self-contained)
        // init
        lcd.setCursor(10, 0); lcd.print("[ ]");
        char* key_map = (char*) keypad_map[ch - '0'];  // get keymap
        int k = 0, wait_count = 0;
        // loop
        while (wait_count < keypad_wait) {
          lcd.setCursor(11, 0);
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


// general purpose password prompt
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
  } // TODO: max_pass_len+ passwords truncated
  lcd.noBlink();

  // check password
  if (count != strlen(true_pass)) return -1;
  for (count = 0; count < strlen(true_pass); count++)
    if (pass_buf[count] != true_pass[count]) return -1;

  return 1;
}


// fancy print
void fancy_print(const char* buf) {
  if (!buf) return;
  if (!fancy) {
    lcd.print(buf);
    return;
  }
  int len = strlen(buf);
  for (int i = 0; i < len; i++) {
    lcd.print(buf[i]);
    delay(25);
  }
}
void fancy_print(const int num) {
  char temp[12] = {};
  sprintf(temp, "%d", num);
  fancy_print(temp);
}


// read input character
// TODO: maybe use both Serial and Keypad?
char keypad_in() {
  while (true) {  //TODO: hanging the entire system!
    unsigned char ch_s = Serial.read(); //TODO: Serial for testing
    unsigned char ch_k = kpd.getKey();
    if (ch_s != 255) return ch_s;
    if (ch_k != 0) return ch_k;
    delay(5);
  }
}

