/*
   Main file for the DSKY project; using serial input for now in place
   of the matrix keypad.

*/

#include "ComTerminal.h"
#include "data.h"
#include "sysutils.hpp"
#include "functions.hpp"

/*===== Init =====*/
void setup()
{
  // set up display/serial
  lcd.begin(D_COLS, D_ROWS);
  if (serial)
  {
    Serial.begin(9600);
    Serial.println("Starting serial");
  }

  if (reset_EEPROM)
  {
    print_message("Reset EEPROM...", 500);
    EEPROM.format();
  }

  // get configs from flash storage?
  if (reset_conf)
  {
    print_message("Reset conf...", 500);
    conf = new CT_Config();
    write_config();
  }
  else
  {
    print_message("Read conf...", 500);
    read_config();
  }

  // Print welcome message and request password
  if (conf->splash)
  {
    print_message("M.A.S. Testing!", 2000);
  }

  // handle bootup password
  while (conf->req_pass)
  {
    int res = password(conf->admin_pass, "System Password:");
    lcd.clear();
    if (res == 1)
      break; // correct
    else if (res == -1)
      print_message("PASSWD INCORRECT", 1000);
    else if (res == -2)
      print_message("PASSWORD REQ'D  FOR LOGIN", 1000);
  }
  if (conf->splash)
  {
    print_message("    Welcome!    ", 1000);
  }
  lcd.clear();
}

/*===== Main Loop =====*/
void loop()
{
  // menu
  //int m_res = menu(MAIN_MENU, MAIN_MENU_LEN, 0, NULL);

  // buffered_editor
  char view1[DEFAULT_BUFSIZE] = "Ministry of Arcane Sciences, United Equestria.";
  int v_res = buffered_editor(view1, DEFAULT_BUFSIZE, 0, 0, NULL);

  print_message(view1, 1000);
}
