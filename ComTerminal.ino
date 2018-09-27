/*
   Main file for the DSKY project; using serial input for now in place
   of the matrix keypad.

*/

#include "ComTerminal.h"
#include "data.h"
#include "sysutils.hpp"
#include "programs.hpp"

/* ===== Init ===== */
void setup() // Note: keeping setup explicit
{
  // setup LED pins
  pinMode(LED_WAIT, OUTPUT);
  pinMode(LED_WORK, OUTPUT);
  digitalWrite(LED_WAIT, LOW);
  digitalWrite(LED_WORK, LOW);

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
    print_message("M.A.S. Testing\n%s", 2000);
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
      print_message("PASSWORD REQ'D\nFOR LOGIN", 1000);
  }
  if (conf->splash)
    print_message("    Welcome!    ", 1000);

  //
}

/*===== Main Loop =====*/
void loop()
{
  // menu
  int item = menu(PROGRAM_NAMES, program_ptrs, PROGRAM_LIST_LEN, 0, "Program");
}
