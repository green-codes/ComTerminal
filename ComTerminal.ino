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
  // TODO: setup a I2c I/O expander for all LEDs except PC13
  pinMode(LED_WAIT, OUTPUT);
  pinMode(LED_STATUS, OUTPUT);
  digitalWrite(LED_WAIT, LOW);
  digitalWrite(LED_STATUS, LOW);

  // setup external I/O interrupt
  // TODO: debug
  // modes: RISING, FALLING, CHANGE
  pinMode(PA0, INPUT_PULLUP);
  attachInterrupt(PA0, handle_exi, FALLING);

  // set up display/serial
  lcd.begin(D_COLS, D_ROWS);
  lcd.clear();
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
    print_message((char *)TEST_STR, 2000, conf->device_name);
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

  // DEBUG stuff
}

/*===== Main Loop =====*/
void loop()
{
  // menu
  menu(PROGRAM_NAMES, program_ptrs, PROGRAM_LIST_LEN, 0, "Program");
}
