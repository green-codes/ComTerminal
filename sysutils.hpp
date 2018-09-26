/*
 * Header for system modules
 * 
 * TODO: REALLY messy code, lots of magic#; clean up when possible
 */

#ifndef SYSTEM_HPP
#define SYSTEM_HPP

#include "ComTerminal.hpp"

/* generic menu */
int menu(const char **items, int num_items, int default_pos, char *prompt)
{
  // check params
  if (items == 0 || num_items == 0)
    return -1;

  // set up vars
  int pos = default_pos;
  char sel_buf[4] = {};
  uint8_t sel_pos = 0;

  // loop
  while (true)
  {
    // prompt
    lcd.clear();
    (prompt) ? lcd.print(prompt) : lcd.print("Menu ");
    lcd.print(pos + 1);
    lcd.print('/');
    lcd.print(num_items);
    // selection buffer display
    lcd.setCursor(d_cols - 4, 0);
    lcd.print(" ___");
    lcd.setCursor(d_cols - 3, 0);
    lcd.print(sel_buf);
    // menu item
    lcd.setCursor(0, 1);
    lcd.print(items[pos]);

    // handle input
    char ch = keypad_in();
    if (ch == m_enter_key || ch == m_enter_key2)
    {
      return pos; // return selection
    }
    else if (ch == m_exit_key)
    {
      return -1; // exit code
    }
    else if (ch == m_up_key)
      pos > 0 ? pos-- : pos = num_items - 1;
    else if (ch == m_down_key)
      pos < num_items - 1 ? pos++ : pos = 0;
    else if (ch == m_clear_key) // clear selection
    {
      pos = 0;
      memset(sel_buf, 0, 4);
      sel_pos = 0;
    }
    else // handle digit inputs
    {
      if (sel_pos < 3) // catch buffer overflow
        sel_buf[sel_pos++] = ch;
      int read_pos = strtol(sel_buf, NULL, 10);
      if (read_pos <= num_items && read_pos > 0)
        pos = read_pos - 1;
    }
  }
}

/* view window: basic character viewer */
// TODO: magic numbers
int view_window(char *buf, int bufsize, uint8_t read_only,
                uint8_t vw_mode, const char *prompt)
{
  if (bufsize == 0)
    bufsize = strlen(buf);
  if (!buf || bufsize < 1)
    return -2;

  // set up vars
  uint8_t mode = 0;      // 2 modes: 1-line scrolling, fullscreen
  uint8_t hex = 0;       // hex display enable
  uint8_t iw_mode = 0;   // editor mode (I/R/N)
  int d_root = 0;        // buffer position at leftmost of screen
  int d_pos[2] = {0, 1}; // cursor position on screen
  // loop until command to exit
  while (true)
  {
    // update bufsize in View mode to reflect possible new '\0's
    bufsize = (vw_modes[vw_mode] == 'F') ? bufsize : strlen(buf);

    /*===== handle display =====*/
    lcd.clear();
    lcd.blink();
    uint8_t line_size = hex ? (mode ? d_cols / 3 - 1 : d_cols / 3) : d_cols;
    if (mode == 0) // 1-line horizontal scrolling
    {
      d_pos[1] = 1; // snap cursor to the second line
      // prompt
      if (prompt)
        lcd.print(prompt);
      else
        lcd.print(vw_modes[vw_mode]);
      lcd.print(':');
      // print counter
      lcd.print(d_root + (mode ? d_pos[1] * line_size : 0) + d_pos[0] + 1);
      lcd.print('/');
      lcd.print(bufsize);
      // display ASCII for the current byte
      lcd.setCursor(12, 0);
      lcd.print("0x");
      hex_print((byte)buf[d_root + d_pos[0]]);

      // rolling window
      lcd.setCursor(0, 1);
      for (int i = 0; i < line_size && d_root + i < bufsize; i++)
      {
        if (hex)
        { // hex mode
          lcd.write(' ');
          hex_print(buf[d_root + i]);
        }
        else
        { // normal mode
          if (vw_modes[vw_mode] == 'F' && buf[d_root + i] == 0)
            lcd.write(' '); // if in Forced mode, use space for null
          else
            lcd.write(buf[d_root + i]);
        }
      }
    }
    else if (mode == 1) // fullscreen
    {
      int d_lines = 0; // number of lines displayed
      if (hex)
      {
        char temp[4] = "";
        sprintf(temp, "%.3d", d_root);
        lcd.print(temp); // count
      }

      // print d_rows lines or until end of buffer
      for (int i = 0; i < d_rows * line_size && d_root + i < bufsize; i++)
      {
        if (i > 0 && i % line_size == 0)
        { // line wrap
          lcd.setCursor(0, ++d_lines);
          if (hex)
          {
            char temp[4] = "";
            sprintf(temp, "%.3d", d_root + 4 * d_lines);
            lcd.print(temp); // count
          }
        }
        if (hex)
        { // hex mode
          lcd.write(' ');
          hex_print(buf[d_root + i]);
        }
        else
        { // normal mode
          if (vw_modes[vw_mode] == 'F' && buf[d_root + i] == 0)
            lcd.write(' ');
          else
            lcd.write(buf[d_root + i]);
        }
      }
    }
    // set cursor
    if (hex)
      if (mode == 0)
        lcd.setCursor(3 * d_pos[0] + 1, d_pos[1]); // 1-line
      else
        lcd.setCursor(3 * d_pos[0] + 4, d_pos[1]); // fullscreen
    else
      lcd.setCursor(d_pos[0], d_pos[1]);

    /*===== handle commands =====*/
    char ch = keypad_in();

    if (ch == vw_mode_key) // switch between 1-line and fullscreen
      mode = mode ? 0 : 1;
    else if (ch == vw_hex_key) // switch between normal and hex modes
      hex = hex ? 0 : 1;
    else if (ch == vw_menu_key) // handle menu operations
    {
      int res = menu(vw_menu, vw_menu_len, 0, NULL);
      switch (res)
      {
      case 0: // Edit from cursor position
      {
        if (read_only)
        {
          lcd_message("Read only buffer", 1000);
          break;
        }
        char op_buf[16] = {};
        char *edit_addr =
            buf + d_root + (mode ? d_pos[1] * line_size : 0) + d_pos[0];
        input_window(op_buf, 16, 2, "LENGTH:");
        int num = strtol(op_buf, NULL, 10);
        if (num > 0)
          input_window(edit_addr, num, iw_mode, NULL);
        else
          lcd_message("Bad edit length", 1000);
        break;
      }
      case 1: // edit entire buffer
        input_window(buf, bufsize, iw_mode, NULL);
        break;
      case 2: // settings
      {
        int s = menu(vw_settings, vw_settings_len, 0, "Settings ");
        if (s == 0)
        {
          char op_buf[1] = {};
          input_window(op_buf, 1, 2, "Force?");
          (strtol(op_buf, NULL, 10)) ? vw_mode = 1 : vw_mode = 0;
          lcd.clear();
          lcd.print(vw_mode);
        }
        else if (s == 1)
        {
          char op_buf[1] = {};
          input_window(op_buf, 1, 2, "0-Fill?");
          (strtol(op_buf, NULL, 10)) ? iw_mode = 1 : iw_mode = 0;
          lcd.clear();
          lcd.print(iw_mode);
        }
        delay(1000);
        break;
      }
      case 3:
        lcd.noBlink();
        return -1;
      }
    }
    else if (ch == vw_exit_key)
    {
      lcd.noBlink();
      return -1;
    }
    else if (ch == vw_enter_key)
    {
      // TODO
    }
    else if (ch == vw_home_key)
    {
      d_root = 0;
      d_pos[0] = 0;
      d_pos[1] = 0;
    }
    else if (ch == vw_end_key)
    {
      d_root = ((bufsize - 1) / line_size) * line_size;
      d_pos[0] = (bufsize - 1) % line_size;
      d_pos[1] = 0;
    }
    else if (ch == vw_left_key)
    {
      if (d_pos[0] > 0) // move left
      {
        d_pos[0]--;
      }
      else if (mode == 0)
      { // 1-line mode
        if (d_root > 0)
          d_root--; // try scrolling left
      }
      else if (mode == 1)
      {                   // fullscreen mode
        if (d_pos[1] > 0) // not yet on first line
        {
          d_pos[0] = line_size - 1;
          d_pos[1]--;
        }
        else if (d_root >= line_size) // try scrolling up
        {
          d_pos[0] = line_size - 1;
          d_root -= line_size;
        }
        else if (d_root > 0)
          d_root--;
      }
    }
    else if (ch == vw_right_key)
    {
      if (mode == 0)
      { // 1-line mode
        if (d_pos[0] < line_size - 1)
          d_pos[0]++;
        else if (bufsize > d_root + line_size)
          d_root++; // scroll right
      }
      else if (mode == 1)
      { // fullscreen
        if (d_pos[0] < line_size - 1)
          d_pos[0]++;
        else if (d_pos[1] < d_rows - 1)
        {
          d_pos[0] = 0;
          d_pos[1]++;
        }
        else if (bufsize > d_root + d_rows * line_size)
        { // try scrolling down
          d_pos[0] = 0;
          d_root += line_size;
        }
      }
    }
    else if (ch == vw_up_key)
    {
      if (mode == 1 && d_pos[1] > 0)
        d_pos[1]--; // not on the first line
      else
      {
        if (d_root >= line_size)
          d_root -= line_size; // try scrolling up
        else
          d_root = 0;
      }
    }
    else if (ch == vw_down_key)
    {
      if (mode == 1 && d_pos[1] < 1)
        d_pos[1]++; // not on the bottom line
      else
      {
        if (d_root + line_size < bufsize)
          d_root += line_size;
        else
          d_pos[0] = line_size - 1;
      }
    }
    // ignore other keys

    // possible cleanups
    d_root = (d_root < 0) ? 0 : d_root; // catch d_root < 0
    // catch case: cursor out of window  TODO: buggy?
    if (d_pos[0] >= line_size)
      d_pos[0] = line_size - 1;
    if (d_pos[1] >= d_rows)
      d_pos[1] = d_rows - 1;
    // catch case: cursor past end of buffer
    if (mode == 0) // 1-line
    {
      if (d_pos[0] >= bufsize - d_root)
        d_pos[0] = bufsize - 1 - d_root;
    }
    else if (mode == 1) // fullscreen
    {
      if (bufsize - 1 < d_root + d_pos[1] * line_size + d_pos[0])
      { // just put the cursor at the right place
        d_pos[1] = (bufsize - 1 - d_root) / line_size;
        d_pos[0] = (bufsize - 1 - d_root) % line_size;
      }
    }
  } // loop
}

/* input window */
int input_window(char *buf, int bufsize,
                 uint8_t iw_mode, const char *prompt)
{
  if (bufsize == 0)
    bufsize = strlen(buf); // watch for bad buf
  if (bufsize > max_bufsize)
    bufsize = max_bufsize;
  if (!buf || bufsize < 1)
    return -2;

  // create and init input buffer
  char *in_buf = (char *)calloc(bufsize + 1, 1); // staged input buf w/null
  strncpy(in_buf, buf, bufsize);
  int buf_pos = strlen(in_buf); // cursor positions
  int d_pos = strlen(in_buf) > 15 ? 15 : strlen(in_buf);

  // setup vars
  uint8_t mode = 0;
  uint8_t shift = 0; // 0 for off, 1 for on
  char ascii_buf[4] = {};
  uint8_t ascii_count = 0; // buf for ASCII mode
  char ch = 0;
  lcd.blink();

  // input loop
  while (buf_pos <= bufsize)
  { // full buffer permissible

    /*===== handle display =====*/
    lcd.clear();
    lcd.setCursor(0, 0);
    // prompt
    if (prompt)
      lcd.print((const char *)prompt); // if given prompt
    else
    { // else display iw_mode
      lcd.print(iw_modes[iw_mode]);
      lcd.print(':');
      lcd.print(buf_pos);
      lcd.print('/');
      lcd.print(bufsize);
    }

    // display input mode information
    if (input_modes[mode] == 'A')
    { // handle ASCII mode displays
      lcd.setCursor(10, 0);
      lcd.print("0x__");
      lcd.setCursor(12, 0);
      lcd.print(ascii_buf);
    }
    lcd.setCursor(14, 0);
    lcd.print(input_modes[mode]);            // mode
    shift ? lcd.print('S') : lcd.print(' '); // shift

    // show full buffer
    lcd.setCursor(15, 1);
    if (buf_pos == bufsize)
      lcd.print('!');
    else
      lcd.print(' ');

    // always display the last 16 bytes in in_buf
    // Note: always leave one space on the right side
    lcd.setCursor(0, 1);
    if (strlen(in_buf) < 15)
    { // less than 15 chars
      lcd.print(in_buf);
      d_pos = strlen(in_buf);
    }
    else
    {                // more than 15 characters
      char temp[16]; // faster than displaying a long buffer
      strncpy(temp, in_buf + strlen(in_buf) - 15, 15);
      lcd.print(temp);
      d_pos = 15;
    }
    lcd.setCursor(d_pos, 1);

    /*===== get key =====*/
    ch = keypad_in();

    /*===== handle control chars =====*/
    if (ch == iw_mode_key)
    {
      // only Dec and Hex modes in Numeric mode
      if (iw_mode == 2)
        mode = (mode) ? 0 : 1;
      else
        (mode == sizeof(input_modes) - 1) ? mode = 0 : mode++;
    }
    else if (ch == iw_shift_key)
    {
      shift = shift ? 0 : 1;
      continue; // don't reset ascii buffer
    }
    else if (ch == iw_clear_key)
    {
      memset(in_buf, 0, bufsize); // clear input buffer
      buf_pos = 0;
      d_pos = 0;
      lcd.setCursor(0, 1);
      lcd.print("                ");
      lcd.setCursor(0, 1);
    }
    else if (ch == iw_exit_key)
    {
      lcd.noBlink();
      free(in_buf);
      return -1;
    }
    else if (ch == iw_del_key)
    {
      // Note: case to clear ASCII mode buffer on press
      if (input_modes[mode] == 'A' && ascii_count > 0)
        ; //nop
      else if (buf_pos > 0)
      {                            // prevent reverse overflow
        in_buf[--buf_pos] = 0;     // erase last character in in_buf
        lcd.setCursor(--d_pos, 1); // go back 1 position
        lcd.print(' ');
        lcd.setCursor(d_pos, 1);
      }
    }
    else if (ch == iw_enter_key)
    {
      if (iw_mode == 0)
        strncpy(buf, in_buf, bufsize); // flush to buf
      else
        strncpy(buf, in_buf, strlen(in_buf)); // DON'T null terminate
      lcd.noBlink();
      free(in_buf);
      return 1;
    }

    /*===== handle input to in_buf =====*/
    // Note: catch non-digit characters; catch buffer overflow
    else if (buf_pos < bufsize && isdigit(ch))
    {

      if (input_modes[mode] == 'D')
      { // Decimal-Calculator Mode
        if (shift && (ch - 1 - '0') < sizeof(iw_dec_map))
        { //shift on
          in_buf[buf_pos] = iw_dec_map[ch - 1 - '0'];
        }
        else
        { // shift off
          in_buf[buf_pos] = ch;
        }
      }

      else if (input_modes[mode] == 'H')
      { // Hex Mode
        if (shift && (ch - 1 - '0') < sizeof(iw_hex_map))
        { // shift on
          in_buf[buf_pos] = iw_hex_map[ch - 1 - '0'];
        }
        else
        { // shift off
          in_buf[buf_pos] = ch;
        }
      }

      else if (input_modes[mode] == 'A')
      { // ASCII Mode
        if (shift && (ch - 1 - '0') < sizeof(iw_hex_map))
        { // shift on
          ascii_buf[ascii_count++] = iw_hex_map[ch - 1 - '0'];
        }
        else
        { // shift off
          ascii_buf[ascii_count++] = ch;
        }
        if (ascii_count < 2)
          continue; // NOT updating display
        else
        { // have all 2 hex digits of the ASCII
          lcd.setCursor(12, 0);
          lcd.print(ascii_buf);
          delay(400);
          in_buf[buf_pos] = (char)strtol(ascii_buf, NULL, 16);
          // ascii buffer cleared at the end of input loop
        }
      }

      else if (input_modes[mode] == 'K')
      { // Keypad Mode (self-contained)
        // init
        lcd.setCursor(11, 0);
        lcd.print("[ ]");
        char *key_map = (char *)iw_keypad_map[ch - '0']; // get keymap
        int k = 0, wait_count = 0;
        // loop
        while (wait_count < iw_keypad_wait)
        {
          lcd.setCursor(12, 0);
          if (k != 0 && shift)
            lcd.print((char)(key_map[k] - 32));
          else
            lcd.print(key_map[k]);
          char k_ch = kpd.getKey(); // get raw char
          if (isdigit(k_ch))
          { // catch non-numeric chars
            wait_count = 0;
            if (k_ch == ch)
            {
              k = (key_map[k + 1] == 0) ? 0 : k + 1;
            }
            else
            { // new input
              ch = k_ch;
              k = 0;
              key_map = (char *)iw_keypad_map[ch - '0'];
            }
          }
          lcd.setCursor(d_pos, 1); // just aesthetics
          delay(5);
          wait_count += 5; // NOTE: careful w/the Keypad lib
        }
        in_buf[buf_pos] = (k != 0 && shift) ? key_map[k] - 32 : key_map[k];
      }

      else
      { // erorr: mode not defined
        lcd.clear();
        lcd.print("Error");
        lcd.setCursor(0, 1);
        lcd.print("Inpt Mthd DNE");
        delay(2000);
        free(in_buf);
        return -1;
      }
      buf_pos++;
      d_pos = (d_pos < 15) ? d_pos + 1 : d_pos; // cap d_pos at 15
    }

    // clear ASCII mode buffer
    memset(ascii_buf, 0, 4);
    ascii_count = 0;

  } // end input loop

  // Error: didn't enter loop
}

/* dedicated general purpose password prompt */
int password(const char *true_pass)
{
  if (!true_pass)
    return -1;
  // prompt
  lcd.clear();
  lcd.setCursor(0, 0);
  fancy_print("Passwd:");
  lcd.setCursor(0, 1);
  fancy_print(" *:Exit #:Enter ");
  lcd.setCursor(7, 0);
  lcd.blink();

  // get password
  int count = 0;
  char pass_buf[max_pass_len] = {};
  while (count < max_pass_len)
  {
    char ch = keypad_in();
    if (ch == '*')
      return -2;
    if (ch == '#')
      break;
    pass_buf[count] = ch;
    lcd.print('*');
    count++;
  }
  lcd.noBlink();

  // check password
  if (count != strlen(true_pass))
    return -1;
  for (count = 0; count < strlen(true_pass); count++)
    if (pass_buf[count] != true_pass[count])
      return -1;

  return 1;
}

// TODO: fancy view
int fancy_view(char *buf, int bufsize, int roll_delay, int end_delay)
{
  // while (d_root + 32 < bufsize) roll_lines();
}

// simple message viewer
void lcd_message(const char *buf, int message_delay)
{
  lcd.clear();
  lcd.print(buf);
  delay(message_delay);
}

// print helpers
void fancy_print(const char *buf)
{
  if (!buf)
    return;
  if (!conf->fancy)
  {
    lcd.print(buf);
    return;
  }
  int len = strlen(buf);
  for (int i = 0; i < len; i++)
  {
    lcd.print(buf[i]);
    delay(conf->fancy_delay);
  }
}
void fancy_print(const long num)
{
  char temp[12] = {}; // max 12 digits
  sprintf(temp, "%d", num);
  fancy_print(temp);
}
void fancy_print(const double num)
{
  char temp[12] = {}; // max 12 digits
  sprintf(temp, "%f", num);
  fancy_print(temp);
}
void hex_print(const char data)
{
  char temp[2];
  sprintf(temp, "%.2x", (uint8_t)data);
  lcd.print(temp);
}
void hex_print(const char *data, int num)
{
  for (int i = 0; i < num; i++)
  {
    hex_print(data[i]);
  }
}

// read input character
char keypad_in()
{
  while (true)
  { //TODO: hanging the entire system!
    if (serial)
    { // Note: serial input takes precedence if enabled
      unsigned char ch_s = Serial.read();
      if (ch_s != 255)
        return ch_s;
    }
    unsigned char ch_k = kpd.getKey();
    if (ch_k != 0)
      return ch_k;
    delay(5);
  }
}

/*===== Emulated EEPROM handling ===== */
// Note: possible 1-byte overflow; make sure num is even
// Note: ptr is unprotected; know what you're doing
void ee_write(uint16_t address, byte *ptr, int num)
{
  uint16_t buf; // 2-byte buffer for writing
  while (num > 0)
  {
    buf = *(uint16_t *)ptr;
    EEPROM.write(address, buf);
    address++;
    ptr += 2;
    num -= 2;
  }
}
// Note: possible 1-byte overflow; make sure num is even
// Note: ptr is unprotected; know what you're doing
byte *ee_read(uint16_t address, byte *ptr, int num)
{
  uint16_t buf; // 2-byte buffer for reading
  byte *orig_ptr = ptr;
  while (num > 0)
  {
    EEPROM.read(address, &buf);
    *(uint16_t *)ptr = buf;
    address++;
    ptr += 2;
    num -= 2;
  }
  return orig_ptr; // for convenience
}
#endif