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

  // set up outputs
  lcd.begin(D_COLS, D_ROWS);
  if (serial)
  {
    Serial.begin(9600);
    Serial.println("Starting serial");
  }

  // get configs from flash storage
  if (reset_conf)
  { // reset; read from default and write to flash
    EEPROM.format();
    conf = new CT_Config; // Note: CONFIG_LEN always even
    ee_write(CONFIG_ADDRESS, (byte *)conf, CONFIG_LEN);
  }
  else
  {                                         // read from flash into memory
    conf = (CT_Config *)malloc(CONFIG_LEN); // allocate on heap!
    ee_read(CONFIG_ADDRESS, (byte *)conf, CONFIG_LEN);
  }

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

  // view
  char view1[] = "Ministry of Arcane Sciences, United Equestria.";
  int v_res = buffered_editor(view1, 0, 0, 0, NULL);

  // input
  char input[64] = "";
  int res = input_window(input, 64, 0, NULL);
  lcd.clear();
  (res == 1) ? fancy_print("OK") : fancy_print("Exited!");
  lcd.setCursor(0, 1);
  fancy_print(input);
  delay(1000);
}
