/*
   Main file for the DSKY project; using serial input for now in place
   of the matrix keypad.

   TODO: REALLY messy code; clean up when possible
*/

// include the library code:
#include "ComTerminal.hpp"
#include "sysutils.hpp"
#include "functions.hpp"
#include "data.h"

/*===== Init =====*/
void setup()
{

  // set up outputs
  lcd.begin(d_cols, d_rows);
  if (serial)
  {
    Serial.begin(9600);
    Serial.println("Starting serial");
  }

  // get configs from flash storage
  if (reset_conf)
  { // reset; read from default and write to flash
    EEPROM.format();
    conf = new ct_config; // Note: config_len always even
    ee_write(config_address, (byte *)conf, config_len);
  }
  else
  {                                         // read from flash into memory
    conf = (ct_config *)malloc(config_len); // allocate on heap!
    ee_read(config_address, (byte *)conf, config_len);
  }

  // LCD backlight
  pinMode(backlight_pin, OUTPUT);
  digitalWrite(backlight_pin, (conf->lcd_backlight) ? HIGH : LOW);

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
  //int m_res = menu(main_menu, main_menu_len, 0, NULL);

  // view
  char view1[] = "Ministry of Arcane Sciences, United Equestria.";
  int v_res = view_window(view1, 0, 0, 0, NULL);

  // input
  char input[64] = "";
  int res = input_window(input, 64, 0, NULL);
  lcd.clear();
  (res == 1) ? fancy_print("OK") : fancy_print("Exited!");
  lcd.setCursor(0, 1);
  fancy_print(input);
  delay(1000);
}
