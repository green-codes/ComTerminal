/*
 * Header for system modules
 * 
 * TODO: REALLY messy code, lots of magic#; clean up when possible
 */

#ifndef SYSTEM_HPP
#define SYSTEM_HPP

#include "ComTerminal.h"
#include "data.h"

/* generic menu */
// TODO: port for larger LCDs
int menu(const char **items, int num_items, int default_pos, char *prompt)
{
  // check params
  if (items == 0 || num_items == 0)
    return -1;

  // set up vars
  int pos = default_pos;    // selected item
  int d_root = default_pos; // first item on display
  char sel_buf[4] = {};
  byte sel_pos = 0;

  // loop
  lcd.blink();
  while (true)
  {
    // prompt
    lcd.clear();
    lcd.print(prompt ? prompt : "Menu");
    lcd.print(":");
    lcd.print(pos + 1);
    lcd.print('/');
    lcd.print(num_items);
    // selection buffer display
    lcd.setCursor(D_COLS - 4, 0);
    lcd.print(" ___");
    lcd.setCursor(D_COLS - 3, 0);
    lcd.print(sel_buf);
    // menu items
    for (int i = 0; i < num_items && i < D_ROWS - 1; i++)
    {
      lcd.setCursor(0, 1 + i);
      lcd.print(items[d_root + i]);
    }
    // handle cursor
    lcd.setCursor(0, 1 + pos - d_root);

    // handle input
    char ch = keypad_wait();
    if (ch == M_ENTER_KEY)
    {
      lcd.noBlink();
      return pos; // return selection
    }
    else if (ch == M_EXIT_KEY || ch == M_MENU_KEY)
    {
      lcd.noBlink();
      return -1; // exit code
    }
    else if (ch == M_UP_KEY)
    {
      pos > 0 ? pos-- : pos = num_items - 1;
    }
    else if (ch == M_DOWN_KEY)
    {
      pos < num_items - 1 ? pos++ : pos = 0;
    }
    else if (ch == M_CLEAR_KEY) // clear selection
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
    // handle d_root
    pos < d_root ? d_root = pos : NULL;
    pos > d_root + D_ROWS - 2 ? d_root = pos - (D_ROWS - 2) : NULL;
  }
}
int menu(const char **items, void (**functions)(), int num_items,
         int default_pos, char *prompt)
{
  // get function selection
  int s = menu(items, num_items, default_pos, prompt);
  // run function if valid
  if (s > -1)
    (*functions[s])();
}

/* buffered editor: safely view and edit buffers */
int buffered_editor(char *in_buf, int bufsize, byte read_only, byte ed_mode,
                    byte editing, byte in_place, const char *prompt)
{
  // param check
  if (bufsize == 0) // no known buffer length, use first null (dangerous?)
    bufsize = strlen(in_buf);
  if (!in_buf || bufsize < 1) // bad buffer args, return error
    return -2;
  if (bufsize > MAX_BUFSIZE) // trim bufsize to MAX_BUFSIZE to (try to) avoid
    bufsize = MAX_BUFSIZE;   // memory overflow (using staged buffer)

  // create and init staged buffer
  char *buf;
  if (read_only || in_place) // no staged buffer
    buf = in_buf;
  else // copy contents from in_buf
  {
    buf = (char *)calloc(bufsize + 1, 1); // staging buffer w/null
    if (ED_MODES[ed_mode] == 'F')
      memcpy(buf, in_buf, bufsize);
    else
      strncpy(buf, in_buf, bufsize); // won't copy after first null
  }

  // viewing mode vars
  bool fullscreen = 0; // available in viewing only
  bool hex = 0;        // hex display
  // input mode vars
  byte input_method = 0; // input method (D/H/K/A)
  byte iw_mode = 0;      // input mode (Insert/Replace)
  bool shift = 0;        // 0 for off, 1 for on
  char ch = 0;
  // common vars
  int d_root = 0, d_pos[2] = {0, 0}, d_offset; // screen and cursor positions

  // loop until command to exit
  while (true)
  {
    editing = read_only ? 0 : editing; // disallow editing while read_only
    byte num_rows = (fullscreen && !editing) ? D_ROWS : D_ROWS - 1;
    byte row_size = hex ? D_COLS / 3 - 1 : D_COLS;
    d_root = (d_root < 0) ? 0 : d_root; // catch d_root < 0
    int buf_eof = (ED_MODES[ed_mode] == 'F') ? bufsize : strlen(buf);

    /*===== handle cursor =====*/
    // catch case: cursor out of window
    if (d_pos[0] >= row_size)
      d_pos[0] = row_size - 1;
    if (d_pos[1] >= num_rows)
      d_pos[1] = num_rows - 1;
    // catch case: cursor past end of buffer
    if (d_root + d_pos[1] * row_size + d_pos[0] > buf_eof)
    {
      // 1-line mode
      if (num_rows == 1)
        d_pos[0] = buf_eof - d_root;
      else
      {
        d_pos[1] = (buf_eof - d_root) / row_size;
        d_pos[0] = (buf_eof - d_root) % row_size;
      }
    }
    d_offset = d_root + d_pos[1] * row_size + d_pos[0];

    /*===== handle display =====*/
    lcd.clear();
    lcd.blink();
    // print buffer section
    if (hex) // hex mode
    {
      int line_count = 0; // number of lines printed
      for (int i = 0; i < num_rows * row_size && d_root + i < bufsize; i++)
      {
        // line wrap and print byte count
        if (i % row_size == 0)
        {
          lcd.setCursor(0, line_count);
          char temp[4] = "";
          sprintf(temp, "%.3d", d_root + line_count * row_size);
          lcd.print(temp); // count
          line_count++;
        }
        lcd.print(' ');
        hex_print(buf[d_root + i]);
      }
    }
    else // text mode
    {
      int print_num; // catch print_num overflow
      if (bufsize - d_root > num_rows * row_size)
        print_num = num_rows * row_size;
      else
        print_num = bufsize - d_root;
      print_lines(buf + d_root, print_num, ED_MODES[ed_mode] == 'F',
                  num_rows, row_size, 0);
    }
    // print status line if not full screen
    if (!fullscreen)
    {
      lcd.setCursor(0, D_ROWS - 1);
      if (prompt)
        lcd.print(prompt);
      else
      {
        lcd.print(editing ? IW_MODES[iw_mode] : ED_MODES[ed_mode]);
        read_only ? lcd.print("O") : NULL;
        in_place &&editing ? lcd.print("P") : NULL;
        lcd.print(':');
        // print counter
        lcd.print(d_offset);
        lcd.print('/');
        lcd.print(buf_eof);
      }
      if (editing) // editing mode
      {
        // IM and shift on/off displays
        lcd.setCursor(D_COLS - 2, D_ROWS - 1);
        lcd.print(IW_INPUT_METHODS[input_method]); // input method
        shift ? lcd.print('S') : lcd.print(' ');   // shift on/off
        // full buffer alert
        if ((ED_MODES[ed_mode] != 'F' && buf_eof == bufsize) || d_offset == bufsize)
        {
          lcd.setCursor(D_COLS - 2, D_ROWS - 1);
          lcd.print('!');
        }
      }
      else // viewing mode status line
      {
        // display ASCII for the current byte
        lcd.setCursor(D_COLS - 4, D_ROWS - 1);
        lcd.print("0x");
        hex_print(buf[d_offset]);
      }
    }

    // set cursor
    if (hex)
      lcd.setCursor(3 * d_pos[0] + 4, d_pos[1]);
    else
      lcd.setCursor(d_pos[0], d_pos[1]);

    /*===== handle commands =====*/
    char ch = keypad_wait();

    // general commands
    if (ch == ED_MENU_KEY)
    {
      // TODO: add menu
      // TODO: add clearing editor buffer as menu item
      switch (menu(ED_MENU, ED_MENU_LEN, 0, NULL))
      {
      case 0: // save: dump buf to in_buf
      {
        if (read_only)
          print_message("Read only!", DEFAULT_DELAY_TIME);
        else if (in_place)
          print_message("Editing in-place!", DEFAULT_DELAY_TIME);
        else
        {
          memcpy(in_buf, buf, bufsize);
          free(buf);
          return 0;
        }
      }
      case 1: // Revert changes
        if (read_only)
          print_message("Read only!", DEFAULT_DELAY_TIME);
        else if (in_place)
          print_message("Editing in-place!", DEFAULT_DELAY_TIME);
        else // copy contents from in_buf
        {
          free(buf);
          buf = (char *)calloc(bufsize + 1, 1);
          if (ED_MODES[ed_mode] == 'F')
            memcpy(buf, in_buf, bufsize);
          else
            strncpy(buf, in_buf, bufsize);
        }
        break;
      case 2: // settings
      {
        int s = menu(ED_SETTINGS, ED_SETTINGS_LEN, 0, "Settings");
        if (s == 0) // view V/F mode switch
          simple_input("Force 0/1") ? ed_mode = 1 : ed_mode = 0;
        if (s == 1) // input I/R mode switch
          simple_input("I=0 R=1") ? iw_mode = 1 : iw_mode = 0;
        break;
      }
      case 3:
        lcd.noBlink();
        if (!in_place)
          free(buf);
        return -2;
      }
    }
    else if (ch == ED_EXIT_KEY)
    {
      lcd.noBlink();
      if (!in_place)
        free(buf);
      return -2;
    }
    else if (ch == ED_EDIT_KEY) // switch b/w editing and viewing
    {
      editing = editing ? 0 : 1;
      fullscreen = editing ? 0 : fullscreen;
    }

    // view/edit-specific commands
    else if (editing) // editing mode
    {
      /*===== handle control chars =====*/
      if (ch == IW_MODE_KEY)
      {
        if (input_method == sizeof(IW_INPUT_METHODS) - 1)
          input_method = 0;
        else
          input_method++;
      }
      else if (ch == IW_SHIFT_KEY)
      {
        shift = shift ? 0 : 1;
        continue; // don't reset ascii buffer
      }

      else if (ch == IW_DEL_KEY)
      {
        if (IW_MODES[iw_mode] == 'I' && d_offset > 0) // insert mode
        {
          memmove(buf + d_offset - 1, buf + d_offset, bufsize - d_offset);
          buf[bufsize - 1] = 0; // null term
          // handle cursor
          if (d_pos[0] > 0) // move cursor left
            d_pos[0]--;
          else if (num_rows == 1 && d_root > 0) // 2-line display, scroll left
            d_root--;
          else // display has more than 2 lines, scroll up
          {
            if (d_pos[1] > 0) // not yet on first line
            {
              d_pos[0] = row_size - 1;
              d_pos[1]--;
            }
            else if (d_root >= row_size) // try scrolling up
            {
              d_pos[0] = row_size - 1;
              d_root -= row_size;
            }
            else if (d_root > 0)
              d_root--;
          }
        }
        else if (IW_MODES[iw_mode] == 'R') // replace mode
          buf[d_offset] = (d_offset == buf_eof - 1) ? 0 : ' ';
      }

      /*===== handle input to buf =====*/
      // TODO: handle Insert mode
      // Note: catch non-digit characters; catch buffer overflow; etc
      else if ((ED_MODES[ed_mode] == 'F' || buf_eof < bufsize) && d_offset < bufsize && isdigit(ch))
      {
        char c = 0; // key buffer

        if (IW_INPUT_METHODS[input_method] == 'D') // Decimal-Calculator Mode
        {
          if (shift && (ch - 1 - '0') < sizeof(IW_DEC_MAP)) //shift on
            c = IW_DEC_MAP[ch - 1 - '0'];
          else // shift off
            c = ch;
        }

        else if (IW_INPUT_METHODS[input_method] == 'H') // Hex Mode
        {
          if (shift && (ch - 1 - '0') < sizeof(IW_HEX_MAP)) // shift on
            c = IW_HEX_MAP[ch - 1 - '0'];
          else // shift off
            c = ch;
        }

        else if (IW_INPUT_METHODS[input_method] == 'A') // ASCII Mode
        {
          char ascii_buf[3] = {};
          byte ascii_count = 0; // buf for ASCII mode
          lcd.setCursor(D_COLS - 6, D_ROWS - 1);
          lcd.print("0x__");
          while (ascii_count < 2)
          {
            // handle control conditions
            if (ch == '*')
              break;
            else if (ch == IW_SHIFT_KEY)
              shift = shift ? 0 : 1;
            // handle input to ascii buffer
            else if (isdigit(ch))
            {
              if (shift && (ch - 1 - '0') < sizeof(IW_HEX_MAP)) // shift on
                ascii_buf[ascii_count++] = IW_HEX_MAP[ch - 1 - '0'];
              else // shift off
                ascii_buf[ascii_count++] = ch;
            }
            // handle ASCII mode displays
            lcd.setCursor(D_COLS - 4, D_ROWS - 1);
            lcd.print(ascii_buf);
            if (ascii_count < 2)
              ch = keypad_wait();
          }
          if (ascii_count == 2) // complete hex input
          {
            c = (char)strtol(ascii_buf, NULL, 16);
            delay(IW_KEYPAD_WAIT);
          }
        }

        else if (IW_INPUT_METHODS[input_method] == 'K') // Keypad Mode
        {
          lcd.setCursor(D_COLS - 5, D_ROWS - 1);
          lcd.print("[ ]");
          char *key_map = (char *)IW_KEYPAD_MAP[ch - '0']; // get keymap
          int k = 0, wait_count = 0;
          // loop
          while (wait_count < IW_KEYPAD_WAIT)
          {
            lcd.setCursor(D_COLS - 4, D_ROWS - 1);
            if (k > 1 && shift)
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
                key_map = (char *)IW_KEYPAD_MAP[ch - '0'];
              }
            }
            lcd.setCursor(D_COLS - 4, D_ROWS - 1); // just aesthetics
            delay(10);
            wait_count += 10; // NOTE: careful w/the Keypad lib
          }
          c = (k != 0 && shift) ? key_map[k] - 32 : key_map[k];
        }

        // modify editing buffer
        if (c)
        {
          // shift buffer after insertion point
          if (IW_MODES[iw_mode] == 'I')
            memmove(buf + d_offset + 1, buf + d_offset, bufsize - d_offset - 1);
          buf[d_offset] = c;
          // handle cursor
          if (d_pos[0] < row_size - 1 && d_offset <= buf_eof) // move cursor
            d_pos[0]++;
          else if (num_rows == 1 && d_offset <= buf_eof) // 2-line
            d_root++;
          else if (d_pos[1] < num_rows - 1) // not yet on bottom line
          {
            d_pos[0] = 0;
            d_pos[1]++;
          }
          else if (d_root + num_rows * row_size <= buf_eof) // scroll down
          {
            d_pos[0] = 0;
            d_root += row_size;
          }
        }
      }
    }
    else // viewing mode
    {
      if (ch == VW_MODE_KEY) // switch between normal and fullscreen
        fullscreen = fullscreen ? 0 : editing ? 0 : 1;
      else if (ch == VW_HEX_KEY) // switch between text and hex modes
        hex = hex ? 0 : 1;
      else if (ch == VW_HOME_KEY)
      {
        d_root = 0;
        d_pos[0] = 0;
        d_pos[1] = 0;
      }
      else if (ch == VW_END_KEY)
      {
        d_root = (buf_eof / row_size) * row_size;
        d_pos[0] = buf_eof % row_size;
        d_pos[1] = (buf_eof - d_root) / row_size;
      }
      else if (ch == VW_LEFT_KEY)
      {
        if (d_pos[0] > 0) // move cursor left
          d_pos[0]--;
        else if (num_rows == 1 && d_root > 0) // 2-line display, scroll left
          d_root--;
        else // display has more than 2 lines, scroll up
        {
          if (d_pos[1] > 0) // not yet on first line
          {
            d_pos[0] = row_size - 1;
            d_pos[1]--;
          }
          else if (d_root >= row_size) // try scrolling up
          {
            d_pos[0] = row_size - 1;
            d_root -= row_size;
          }
          else if (d_root > 0)
            d_root--;
        }
      }
      else if (ch == VW_RIGHT_KEY)
      {
        if (d_pos[0] < row_size - 1 && d_offset < buf_eof) // move cursor right
          d_pos[0]++;
        else if (num_rows == 1 && d_root + row_size <= buf_eof) // 2-line
          d_root++;
        else if (d_pos[1] < num_rows - 1) // not yet on bottom line
        {
          d_pos[0] = 0;
          d_pos[1]++;
        }
        else if (d_root + num_rows * row_size <= buf_eof) // scroll down
        {
          d_pos[0] = 0;
          d_root += row_size;
        }
      }
      else if (ch == VW_UP_KEY)
      {
        if (d_pos[1] > 0) // not on the first line
          d_pos[1]--;
        else // on first line
        {
          if (d_root >= row_size)
            d_root -= row_size; // scroll up
          else if (d_root >= 0)
          {
            d_root = 0; // reset to home
            d_pos[0] = 0;
          }
        }
      }
      else if (ch == VW_DOWN_KEY)
      {
        if (d_pos[1] < num_rows - 1) // not on the bottom row
          d_pos[1]++;
        else // on the bottom row
        {
          if (d_root + row_size <= buf_eof) // more buffer below
            d_root += row_size;
          else
            //d_pos[0] = buf_eof - d_root; // move to end
            d_pos[0] = row_size - 1;
        }
      }
      // will handle the cursor on next loop
    } // end viewing commands

  } // loop
}

/* dedicated general purpose password prompt */
int password(const char *true_pass, const char *prompt)
{
  if (!true_pass)
    return -1;
  if (conf->wrong_admin_pass_count >= MAX_PASS_FAILS)
  { // locking system
    print_message("System Locked", 0);
    while (true)
      delay(100);
  }
  if (conf->wrong_admin_pass_count > 0)
  {
    char temp[16] = "";
    sprintf(temp, "PW Retries: %d",
            MAX_PASS_FAILS - conf->wrong_admin_pass_count);
    print_message(temp, DEFAULT_DELAY_TIME);
  }
  char pass_buf[MAX_PASS_LEN + 1] = {};
  int res = simple_input(pass_buf, MAX_PASS_LEN, prompt, true);
  if (strcmp(pass_buf, true_pass) == 0)
  {
    conf->wrong_admin_pass_count = 0;
    write_config();
    return 1;
  }
  conf->wrong_admin_pass_count++;
  write_config();
  return -1;
}

/* ===== output helpers ===== */

// print lines
void print_lines(char *const buf, int bufsize, byte force,
                 int num_rows, int row_size, int start_row)
{
  // if not in force mode, cut bufsize to first null
  if (!force || bufsize == 0)
    bufsize = strlen(buf);
  int row_count = 0, num_printed = 0;
  // print num_lines or until end of buf
  for (int i = 0; i < bufsize && num_printed < num_rows * row_size; i++)
  {
    // handle new line character
    // TODO: breaks buffered_editor
    // if (buf[i] == '\n')
    // {
    //   if (row_count < num_rows)
    //     lcd.setCursor(0, start_row + row_count++);
    //   num_printed += row_size - (i % row_size);
    //   i++;
    // }
    // else
    num_printed++;
    // line wrap
    if (i % row_size == 0 && row_count < num_rows)
      lcd.setCursor(0, start_row + row_count++);
    // print character
    if (buf[i] == 0) // always substitute nulls with spaces
      lcd.write(' ');
    else
      lcd.write(buf[i]);
  }
}
void print_line(char *const buf, byte force, int start_row)
{
  print_lines(buf, D_COLS, force, 1, D_COLS, start_row);
}

// simple message viewer
void print_message(char *format, int message_delay, ...)
{
  char *p_buf = (char *)calloc(1, strlen(format) + DEFAULT_BUFSIZE);
  va_list args;
  va_start(args, format);
  vsprintf(p_buf, format, args);
  va_end(args);
  lcd.clear();
  print_lines(p_buf, 0, 0, D_ROWS, D_COLS, 0);
  free(p_buf);
  delay(message_delay);
}

// TODO: fancy view
int fancy_view(char *buf, int bufsize, int roll_delay, int end_delay)
{
  // while (d_root + 32 < bufsize) roll_lines();
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
  sprintf(temp, "%.2x", (byte)data);
  lcd.print(temp);
}
void hex_print(const char *data, int num)
{
  for (int i = 0; i < num; i++)
  {
    hex_print(data[i]);
  }
}

/* ===== input helpers ===== */

// simple input window, takes an input buffer
int simple_input(char *buf, int bufsize, const char *prompt, bool is_pw)
{
  bufsize = bufsize > D_COLS ? D_COLS : bufsize; // limit bufsize to one row
  int count = 0;
  lcd.blink();
  while (count < bufsize)
  {
    // prompt
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(prompt ? prompt : "Input:");
    lcd.setCursor(0, 1);
    lcd.print(buf);
    char ch = keypad_wait();
    if (ch == '*')
      buf[--count] = 0;
    else if (ch == '#')
      break;
    else if (isdigit(ch))
    {
      buf[count++] = ch;
      lcd.print(is_pw ? '*' : ch);
    }
  }
  lcd.noBlink();
  return 0;
}
int simple_input(const char *prompt)
{
  char buf[D_COLS + 1] = {};
  simple_input(buf, D_COLS, prompt, 0);
  return strtol(buf, NULL, 10);
}

// wait for keypad input
char keypad_wait()
{
  if (led_enabled)
    digitalWrite(LED_WAIT, HIGH); // indicate waiting for input
  while (true)
  {
    if (serial)
    { // Note: serial input takes precedence if enabled
      unsigned char ch_s = Serial.read();
      if (ch_s != 255)
      {
        digitalWrite(LED_WAIT, LOW);
        return ch_s;
      }
    }
    unsigned char ch_k = kpd.getKey();
    if (ch_k != 0)
    {
      digitalWrite(LED_WAIT, LOW);
      return ch_k;
    }
    delay(10);
  }
}

/* ===== System functions ===== */

void handle_exi()
{
  print_message("Interrput!", 1000);
}

void reset_system()
{
  print_message("Reseting system", 0);
  nvic_sys_reset();
}

// config reading/writing
void read_config()
{
  free(conf); // lest memory leak
  conf = (CT_Config *)malloc(CONFIG_LEN);
  ee_read(CONFIG_ADDRESS, (byte *)conf, CONFIG_LEN);
}
void write_config()
{
  ee_write(CONFIG_ADDRESS, (byte *)conf, CONFIG_LEN);
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

/* ===== LCD_Driver class ===== */
// Note: should handle cursor movement, screen scrolling and printing
class LCD_Driver
{
public:
  int d_root, d_pos[2], d_offset;
  LCD_Driver();
};

#endif