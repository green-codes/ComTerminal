/*
 * Main file for the DSKY project; using serial input for now in place
 * of the matrix keypad.
 * 
*/

#include "base.h"
#include "data.h"
#include "devices.h"
#include "sysutils.h"
#include "programs.hpp"

/* ===== Init ===== */
void setup() // Note: keeping setup explicit
{
  // set up display/serial
  lcd.begin(D_COLS, D_ROWS);
  lcd.clear();
  if (serial)
    Serial.begin(9600);

  // setup external I/O interrupt
  pinMode(PA0, INPUT_PULLUP);
  attachInterrupt(PA0, handle_exi, FALLING);

  // setup the I2C interface
  Wire.begin();
  // setup MCP23008(s)
  print_message("Init MCP...", 0);
  mcp.begin(0); // address 0x20 + 0b000
  print_message("MCP initialized", 500);
  // init MPU6050
  if (MPU_enabled)
  {
    print_message("MPU init...", 0);
    setupMPU();
    print_message("MPU initialized", 500);
  }

  // setup LED pins
  pinMode(LED_STATUS, OUTPUT);
  digitalWrite(LED_STATUS, LOW);
  mcp.pinMode(LED_WAIT, OUTPUT);
  mcp.pinMode(LED_IO, OUTPUT);
  mcp.digitalWrite(LED_WAIT, LOW);
  mcp.digitalWrite(LED_IO, LOW);

  // EEPROM and conf options
  if (reset_EEPROM)
  {
    print_message("Reset EEPROM...", 500);
    EEPROM.format();
  }
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
    print_message((char *)TEST_STR, 2000, conf->device_name);

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
      print_message("PASSWORD REQ'D", 1000);
  }

  // startup program
  //pgm_tests();

  // DEBUG stuff
  //Serial.println("Hello!");
}

/*===== Main Loop =====*/
void loop()
{
  // menu
  menu(PROGRAM_NAMES, program_ptrs, PROGRAM_LIST_LEN, 0, "Program");
}
