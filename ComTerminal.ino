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
    EEPROM.format();

  // get configs from flash storage
  if (reset_conf)
  {
    conf = new CT_Config();
    write_config();
  }
  else
    read_config();

  // Print welcome message and request password
  if (conf->splash)
  {
    fancy_print("M.A.S. Testing");
    delay(2000);
  }

  // handle bootup password
  while (conf->req_pass)
  {
    uint8_t res = password(conf->admin_pass);
    lcd.clear();
    if (res == 1)
      break; // correct
    else if (res == -1)
      fancy_print("PASSWD INCORRECT");
    else if (res == -2)
    {
      fancy_print("PASSWORD REQ'D");
      lcd.setCursor(0, 1);
      fancy_print("FOR LOGIN");
    }
    delay(1000);
  }
  if (conf->splash)
  {
    fancy_print("    Welcome!    ");
    delay(1000);
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
