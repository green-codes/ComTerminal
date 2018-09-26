/*
 * Header for system modules
 * 
 * TODO: REALLY messy code, lots of magic#; clean up when possible
 */

#ifndef SYSTEM_HPP
#define SYSTEM_HPP

#include "ComTerminal.h"

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
    lcd.setCursor(D_COLS - 4, 0);
    lcd.print(" ___");
    lcd.setCursor(D_COLS - 3, 0);
    lcd.print(sel_buf);
    // menu item
    lcd.setCursor(0, 1);
    lcd.print(items[pos]);

    // handle input
    char ch = keypad_in();
    if (ch == M_ENTER_KEY || ch == M_ENTER_KEY2)
    {
      return pos; // return selection
    }
    else if (ch == M_EXIT_KEY)
    {
      return -1; // exit code
    }
    else if (ch == M_UP_KEY)
      pos > 0 ? pos-- : pos = num_items - 1;
    else if (ch == M_DOWN_KEY)
      pos < num_items - 1 ? pos++ : pos = 0;
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
  }
}

/* buffered editor: safely view and edit buffers */
int buffered_editor(char *buf, int bufsize, uint8_t read_only,
                    uint8_t ed_mode, const char *prompt)
{
  // param check
  if (bufsize == 0) // no known buffer length, use first null (dangerous?)
    bufsize = strlen(buf);
  if (!buf || bufsize < 1) // bad buffer args, return error
    return -2;
  if (bufsize > MAX_BUFSIZE) // trim bufsize to MAX_BUFSIZE to (try to) avoid
    bufsize = MAX_BUFSIZE;   // memory overflow (using staged buffer)

  // create and init staged buffer
  char *in_buf = (char *)calloc(bufsize + 1, 1); // staging buffer w/null
  strncpy(in_buf, buf, bufsize);

  // common vars
  uint8_t editing = 0;               // editing?
  int d_root = 0, d_pos[2] = {0, 0}; // screen and cursor positions
  // viewing mode vars
  uint8_t fullscreen = 0; // available in viewing only
  uint8_t hex = 0;        // hex display
  // input mode vars
  uint8_t input_method = 0; // input method (D/H/K/A)
  uint8_t iw_mode = 0;      // input mode (Insert/Replace)
  uint8_t shift = 0;        // 0 for off, 1 for on
  char ascii_buf[4] = {};
  uint8_t ascii_count = 0; // buf for ASCII mode
  char ch = 0;

  // loop until command to exit
  while (true)
  {
    // if not in force mode, cut bufsize to first null
    bufsize = (ED_MODES[ed_mode] == 'F') ? bufsize : strlen(buf);

    // catch d_root < 0
    d_root = (d_root < 0) ? 0 : d_root;

    /*===== handle display =====*/
    lcd.clear();
    lcd.blink();
    uint8_t row_size = hex ? D_COLS / 3 : D_COLS;
    uint8_t num_rows = fullscreen && !editing ? D_ROWS : D_ROWS - 1;
    // print buffer section
    if (hex) // hex mode
    {
      int line_count = 0; // number of lines printed
      for (int i = 0; i < num_rows * row_size && d_root + i < bufsize; i++)
      {
        // line wrap and print byte count
        if (i % row_size == 0)
        {
          lcd.setCursor(0, line_count++);
          char temp[4] = "";
          sprintf(temp, "%.3d", d_root);
          lcd.print(temp); // count
        }
        lcd.print(' ');
        hex_print(buf[i]);
      }
    }
    else // text mode
    {
      int print_num;
      if (bufsize - d_root > num_rows * row_size)
        print_num = num_rows * row_size;
      else
        print_num = bufsize - d_root;
      print_lines(buf + d_root, print_num,
                  ED_MODES[ed_mode] == 'F', num_rows, 0);
    }
    // print status line if not full screen
    if (!fullscreen)
    {
      lcd.setCursor(0, D_ROWS - 1);
      if (editing) // editind mode status line
      {
        if (prompt)
          lcd.print((const char *)prompt); // if given prompt
        else
        {
          // input mode
          lcd.print(IW_MODES[iw_mode]);
          lcd.print(':');
          // cursor position / bufsize
          lcd.print(d_root + d_pos[1] * row_size + d_pos[0]);
          lcd.print('/');
          lcd.print(bufsize);
        }
        // input mode information
        if (IW_INPUT_METHODS[mode] == 'A')
        { // handle ASCII mode displays
          lcd.setCursor(D_ROWS - 6, D_COLS - 1);
          lcd.print("0x__");
          lcd.setCursor(D_ROWS - 4, D_COLS - 1);
          lcd.print(ascii_buf);
        }
        lcd.setCursor(D_COLS - 2, D_ROWS - 1);
        lcd.print(IW_INPUT_METHODS[mode]);       // input method
        shift ? lcd.print('S') : lcd.print(' '); // shift on/off
        // full buffer alert
        if (d_root + d_pos[1] * row_size + d_pos[0] == bufsize)
        {
          lcd.setCursor(D_COLS - 2, D_ROWS - 1);
          lcd.print('!');
        }
        lcd.setCursor(d_pos[0], d_pos[1]);
      }
      else // view mode status line
      {
        // print prompt
        if (prompt)
          lcd.print(prompt);
        else
          lcd.print(ED_MODES[ed_mode]);
        lcd.print(':');
        // print counter
        lcd.print(d_root + d_pos[1] * row_size + d_pos[0] + 1);
        lcd.print('/');
        lcd.print(bufsize);
        // display ASCII for the current byte
        lcd.setCursor(12, 0);
        lcd.print("0x");
        hex_print((byte)buf[d_root + d_pos[0]]);
      }
    }

    /*===== handle cursor =====*/
    // catch case: cursor out of window
    if (d_pos[0] >= row_size)
      d_pos[0] = row_size - 1;
    if (d_pos[1] >= num_rows)
      d_pos[1] = num_rows - 1;
    // catch case: cursor past end of buffer
    if (D_ROWS == 2 && d_pos[0] > bufsize - d_root) // 1-line mode
      d_pos[0] = bufsize - d_root;
    else if (bufsize - 1 - d_root < d_pos[1] * row_size + d_pos[0])
    { // multiple lines: just put the cursor at the right place
      d_pos[1] = (bufsize - 1 - d_root) / row_size;
      d_pos[0] = (bufsize - d_root) % row_size;
    }
    // set cursor
    if (hex)
      lcd.setCursor(3 * d_pos[0] + 4, d_pos[1]);
    else
      lcd.setCursor(d_pos[0], d_pos[1]);

    /*===== handle commands =====*/
    char ch = keypad_in();

    if (editing) // editing mode
    {
      /*===== handle control chars =====*/
      ch = keypad_in();
      if (ch == IW_MODE_KEY)
      {
        // only Dec and Hex modes in Numeric mode
        if (iw_mode == 2)
          mode = (mode) ? 0 : 1;
        else
          (mode == sizeof(IW_INPUT_METHODS) - 1) ? mode = 0 : mode++;
      }
      else if (ch == IW_SHIFT_KEY)
      {
        shift = shift ? 0 : 1;
        continue; // don't reset ascii buffer
      }
      else if (ch == IW_CLEAR_KEY)
      {
        memset(in_buf, 0, bufsize); // clear input buffer
        d_root = 0;
        d_pos = 0;
        lcd.setCursor(0, 1);
        lcd.print("                ");
        lcd.setCursor(0, 1);
      }
      else if (ch == IW_EXIT_KEY)
      {
        lcd.noBlink();
        free(in_buf);
        return -1;
      }
      else if (ch == IW_DEL_KEY)
      {
        // Note: case to clear ASCII mode buffer on press
        if (IW_INPUT_METHODS[mode] == 'A' && ascii_count > 0)
          ; //nop
        else if (d_root > 0)
        {                            // prevent reverse overflow
          in_buf[--d_root] = 0;      // erase last character in in_buf
          lcd.setCursor(--d_pos, 1); // go back 1 position
          lcd.print(' ');
          lcd.setCursor(d_pos, 1);
        }
      }
      else if (ch == IW_ENTER_KEY)
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
      else if (d_root < bufsize && isdigit(ch))
      {

        if (IW_INPUT_METHODS[mode] == 'D')
        { // Decimal-Calculator Mode
          if (shift && (ch - 1 - '0') < sizeof(IW_DEC_MAP))
          { //shift on
            in_buf[d_root] = IW_DEC_MAP[ch - 1 - '0'];
          }
          else
          { // shift off
            in_buf[d_root] = ch;
          }
        }

        else if (IW_INPUT_METHODS[mode] == 'H')
        { // Hex Mode
          if (shift && (ch - 1 - '0') < sizeof(IW_HEX_MAP))
          { // shift on
            in_buf[d_root] = IW_HEX_MAP[ch - 1 - '0'];
          }
          else
          { // shift off
            in_buf[d_root] = ch;
          }
        }

        else if (IW_INPUT_METHODS[mode] == 'A')
        { // ASCII Mode
          if (shift && (ch - 1 - '0') < sizeof(IW_HEX_MAP))
          { // shift on
            ascii_buf[ascii_count++] = IW_HEX_MAP[ch - 1 - '0'];
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
            in_buf[d_root] = (char)strtol(ascii_buf, NULL, 16);
            // ascii buffer cleared at the end of input loop
          }
        }

        else if (IW_INPUT_METHODS[mode] == 'K')
        { // Keypad Mode (self-contained)
          // init
          lcd.setCursor(11, 0);
          lcd.print("[ ]");
          char *key_map = (char *)IW_KEYPAD_MAP[ch - '0']; // get keymap
          int k = 0, wait_count = 0;
          // loop
          while (wait_count < IW_KEYPAD_WAIT)
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
                key_map = (char *)IW_KEYPAD_MAP[ch - '0'];
              }
            }
            lcd.setCursor(d_pos, 1); // just aesthetics
            delay(5);
            wait_count += 5; // NOTE: careful w/the Keypad lib
          }
          in_buf[d_root] = (k != 0 && shift) ? key_map[k] - 32 : key_map[k];
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
        d_root++;
        d_pos = (d_pos < 15) ? d_pos + 1 : d_pos; // cap d_pos at 15
      }

      // clear ASCII mode buffer
      memset(ascii_buf, 0, 4);
      ascii_count = 0;
    }
    else // viewing mode
    {
      if (ch == VW_MODE_KEY) // switch between normal and fullscreen
        fullscreen = fullscreen ? 0 : 1;
      else if (ch == VW_HEX_KEY) // switch between text and hex modes
        hex = hex ? 0 : 1;
      else if (ch == VW_MENU_KEY) // handle menu operations
      {
        int res = menu(VW_MENU, VW_MENU_LEN, 0, NULL);
        switch (res)
        {
        case 0: // Edit from cursor position
        {
          if (read_only) // disallow edit if read_only
          {
            print_message("Read only buffer", 1000);
            break;
          }
          // get edit address
          char *edit_addr =
              buf + d_root + d_pos[1] * row_size + d_pos[0];
          // get edit length
          char op_buf[D_COLS] = {};
          input_window(op_buf, D_COLS, 2, "LENGTH:");
          int num = strtol(op_buf, NULL, 10);
          if (num <= 0 || edit_addr + num > buf + bufsize) // invalid edit length
          {
            print_message("Bad edit length", 1000);
            break;
          }
          input_window(edit_addr, num, iw_mode, NULL);
          break;
        }
        case 1: // edit entire buffer
          input_window(buf, bufsize, iw_mode, NULL);
          break;
        case 2: // settings
        {
          int s = menu(VW_SETTINGS, VW_SETTINGS_LEN, 0, "Settings ");
          if (s == 0)
          {
            char op_buf[1] = {};
            input_window(op_buf, 1, 2, "Force?");
            (strtol(op_buf, NULL, 10)) ? ed_mode = 1 : ed_mode = 0;
            lcd.clear();
            lcd.print(ed_mode);
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
      else if (ch == VW_EXIT_KEY)
      {
        lcd.noBlink();
        return -1;
      }
      else if (ch == VW_ENTER_KEY)
      {
        // TODO
      }
      else if (ch == VW_HOME_KEY)
      {
        d_root = 0;
        d_pos[0] = 0;
        d_pos[1] = 0;
      }
      else if (ch == VW_END_KEY)
      {
        d_root = ((bufsize - 1) / row_size) * row_size;
        d_pos[0] = (bufsize - 1) % row_size;
        if (num_rows * row_size - 1 < bufsize - d_root) // out of window
          d_pos[1] = num_rows - 1;
        else // end of buffer within window
          d_pos[1] = (bufsize - d_root - 1) / row_size;
      }
      else if (ch == VW_LEFT_KEY)
      {
        if (d_pos[0] > 0) // move cursor left
          d_pos[0]--;
        else if (D_ROWS == 2 && d_root > 0) // 2-line display, scroll left
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
        if (d_pos[0] < row_size - 1) // move cursor right
          d_pos[0]++;
        else if (D_ROWS == 2 && bufsize - d_root > row_size) // 2-line display
          d_root++;
        else if (d_pos[1] < num_rows - 1) // not yet on bottom line
        {
          d_pos[0] = 0;
          d_pos[1]++;
        }
        else if (bufsize - d_root > num_rows * row_size) // scroll down
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
          else
            d_root = 0; // reset to home
        }
      }
      else if (ch == VW_DOWN_KEY)
      {
        if (d_pos[1] < 1) // not on the bottom line
          d_pos[1]++;
        else // on the bottom line
        {
          if (d_root + row_size < bufsize - 1) // more buffer below
            d_root += row_size;
          else
            d_pos[0] = row_size - 1; // move to end of last line
        }
      }
      // ignore other keys

    } // end viewing command handling

  } // loop
}

/* input window */
// int input_window(char *buf, int bufsize,
//                  uint8_t iw_mode, const char *prompt)
// {
//   if (bufsize == 0) // watch for bad buf
//     bufsize = strlen(buf);
//   if (bufsize > MAX_BUFSIZE) // trim bufsize to MAX_BUFSIZE
//     bufsize = MAX_BUFSIZE;
//   if (!buf || bufsize < 1)
//     return -2;

//   // create and init input buffer
//   char *in_buf = (char *)calloc(bufsize + 1, 1); // staged input buf w/null
//   strncpy(in_buf, buf, bufsize);
//   int d_root = 0, d_pos[2] = {0, 0}; // screen and cursor positions

//   // setup vars
//   uint8_t mode = 0;
//   uint8_t shift = 0; // 0 for off, 1 for on
//   char ascii_buf[4] = {};
//   uint8_t ascii_count = 0; // buf for ASCII mode
//   char ch = 0;
//   lcd.blink();

//   // input loop
//   while (d_root <= bufsize) // Note: full buffer permissible
//   {

//     /*===== handle display =====*/
//     lcd.clear();
//     // display buffer
//     if (strlen(in_buf) < 15) // less than 15 chars
//     {
//       lcd.print(in_buf);
//       d_pos[0] = strlen(in_buf);
//     }
//     else // more than 15 characters
//     {
//       char temp[16]; // faster than displaying a long buffer
//       strncpy(temp, in_buf + strlen(in_buf) - 15, 15);
//       lcd.print(temp);
//       d_pos[0] = 15;
//     }
//     lcd.setCursor(d_pos[0], d_pos[1]);

//     // display status line
//     lcd.setCursor(0, D_ROWS - 1);
//     if (prompt)
//       lcd.print((const char *)prompt); // if given prompt
//     else
//     { // else display iw_mode
//       lcd.print(IW_MODES[iw_mode]);
//       lcd.print(':');
//       lcd.print(d_root);
//       lcd.print('/');
//       lcd.print(bufsize);
//     }
//     // input mode information
//     if (IW_INPUT_METHODS[mode] == 'A')
//     { // handle ASCII mode displays
//       lcd.setCursor(10, 0);
//       lcd.print("0x__");
//       lcd.setCursor(12, 0);
//       lcd.print(ascii_buf);
//     }
//     lcd.setCursor(D_COLS - 2, D_ROWS - 1);
//     lcd.print(IW_INPUT_METHODS[mode]);       // mode
//     shift ? lcd.print('S') : lcd.print(' '); // shift
//     // full buffer alert
//     if (d_root == bufsize)
//     {
//       lcd.setCursor(D_COLS - 2, D_ROWS - 1);
//       lcd.print('!');
//     }
//     lcd.setCursor(d_pos[0], d_pos[1]);

//     /*===== handle control chars =====*/
//     ch = keypad_in();
//     if (ch == IW_MODE_KEY)
//     {
//       // only Dec and Hex modes in Numeric mode
//       if (iw_mode == 2)
//         mode = (mode) ? 0 : 1;
//       else
//         (mode == sizeof(IW_INPUT_METHODS) - 1) ? mode = 0 : mode++;
//     }
//     else if (ch == IW_SHIFT_KEY)
//     {
//       shift = shift ? 0 : 1;
//       continue; // don't reset ascii buffer
//     }
//     else if (ch == IW_CLEAR_KEY)
//     {
//       memset(in_buf, 0, bufsize); // clear input buffer
//       d_root = 0;
//       d_pos = 0;
//       lcd.setCursor(0, 1);
//       lcd.print("                ");
//       lcd.setCursor(0, 1);
//     }
//     else if (ch == IW_EXIT_KEY)
//     {
//       lcd.noBlink();
//       free(in_buf);
//       return -1;
//     }
//     else if (ch == IW_DEL_KEY)
//     {
//       // Note: case to clear ASCII mode buffer on press
//       if (IW_INPUT_METHODS[mode] == 'A' && ascii_count > 0)
//         ; //nop
//       else if (d_root > 0)
//       {                            // prevent reverse overflow
//         in_buf[--d_root] = 0;      // erase last character in in_buf
//         lcd.setCursor(--d_pos, 1); // go back 1 position
//         lcd.print(' ');
//         lcd.setCursor(d_pos, 1);
//       }
//     }
//     else if (ch == IW_ENTER_KEY)
//     {
//       if (iw_mode == 0)
//         strncpy(buf, in_buf, bufsize); // flush to buf
//       else
//         strncpy(buf, in_buf, strlen(in_buf)); // DON'T null terminate
//       lcd.noBlink();
//       free(in_buf);
//       return 1;
//     }

//     /*===== handle input to in_buf =====*/
//     // Note: catch non-digit characters; catch buffer overflow
//     else if (d_root < bufsize && isdigit(ch))
//     {

//       if (IW_INPUT_METHODS[mode] == 'D')
//       { // Decimal-Calculator Mode
//         if (shift && (ch - 1 - '0') < sizeof(IW_DEC_MAP))
//         { //shift on
//           in_buf[d_root] = IW_DEC_MAP[ch - 1 - '0'];
//         }
//         else
//         { // shift off
//           in_buf[d_root] = ch;
//         }
//       }

//       else if (IW_INPUT_METHODS[mode] == 'H')
//       { // Hex Mode
//         if (shift && (ch - 1 - '0') < sizeof(IW_HEX_MAP))
//         { // shift on
//           in_buf[d_root] = IW_HEX_MAP[ch - 1 - '0'];
//         }
//         else
//         { // shift off
//           in_buf[d_root] = ch;
//         }
//       }

//       else if (IW_INPUT_METHODS[mode] == 'A')
//       { // ASCII Mode
//         if (shift && (ch - 1 - '0') < sizeof(IW_HEX_MAP))
//         { // shift on
//           ascii_buf[ascii_count++] = IW_HEX_MAP[ch - 1 - '0'];
//         }
//         else
//         { // shift off
//           ascii_buf[ascii_count++] = ch;
//         }
//         if (ascii_count < 2)
//           continue; // NOT updating display
//         else
//         { // have all 2 hex digits of the ASCII
//           lcd.setCursor(12, 0);
//           lcd.print(ascii_buf);
//           delay(400);
//           in_buf[d_root] = (char)strtol(ascii_buf, NULL, 16);
//           // ascii buffer cleared at the end of input loop
//         }
//       }

//       else if (IW_INPUT_METHODS[mode] == 'K')
//       { // Keypad Mode (self-contained)
//         // init
//         lcd.setCursor(11, 0);
//         lcd.print("[ ]");
//         char *key_map = (char *)IW_KEYPAD_MAP[ch - '0']; // get keymap
//         int k = 0, wait_count = 0;
//         // loop
//         while (wait_count < IW_KEYPAD_WAIT)
//         {
//           lcd.setCursor(12, 0);
//           if (k != 0 && shift)
//             lcd.print((char)(key_map[k] - 32));
//           else
//             lcd.print(key_map[k]);
//           char k_ch = kpd.getKey(); // get raw char
//           if (isdigit(k_ch))
//           { // catch non-numeric chars
//             wait_count = 0;
//             if (k_ch == ch)
//             {
//               k = (key_map[k + 1] == 0) ? 0 : k + 1;
//             }
//             else
//             { // new input
//               ch = k_ch;
//               k = 0;
//               key_map = (char *)IW_KEYPAD_MAP[ch - '0'];
//             }
//           }
//           lcd.setCursor(d_pos, 1); // just aesthetics
//           delay(5);
//           wait_count += 5; // NOTE: careful w/the Keypad lib
//         }
//         in_buf[d_root] = (k != 0 && shift) ? key_map[k] - 32 : key_map[k];
//       }

//       else
//       { // erorr: mode not defined
//         lcd.clear();
//         lcd.print("Error");
//         lcd.setCursor(0, 1);
//         lcd.print("Inpt Mthd DNE");
//         delay(2000);
//         free(in_buf);
//         return -1;
//       }
//       d_root++;
//       d_pos = (d_pos < 15) ? d_pos + 1 : d_pos; // cap d_pos at 15
//     }

//     // clear ASCII mode buffer
//     memset(ascii_buf, 0, 4);
//     ascii_count = 0;

//   } // end input loop

//   // Error: didn't enter loop
// }

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
  char pass_buf[MAX_PASS_LEN] = {};
  while (count < MAX_PASS_LEN)
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

// print lines
void print_lines(char *const buf, int bufsize, uint8_t force,
                 int num_rows, int start_row)
{
  if (!force) // if not in force mode, cut bufsize to first null
    bufsize = strlen(buf);
  int row_count = 0;
  // print num_lines or until end of buf
  for (int i = 0; i < bufsize && i < num_rows * D_COLS; i++)
  {
    // line wrap
    if (i % D_COLS == 0)
      lcd.setCursor(0, start_row + row_count++);
    // print character
    if (force && buf[i] == 0) // if force, substitute nulls with spaces
      lcd.write(' ');
    else
      lcd.write(buf[i]);
  }
}
void print_line(char *const buf, uint8_t force, int start_row)
{
  print_lines(buf, D_COLS, force, 1, start_row);
}

// TODO: fancy view
int fancy_view(char *buf, int bufsize, int roll_delay, int end_delay)
{
  // while (d_root + 32 < bufsize) roll_lines();
}

// simple message viewer
void print_message(const char *buf, int message_delay)
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