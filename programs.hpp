/*
  Header for functions/applications

  Note: functions below should take no args and return void. 
  Note: Remember to change program names and pointers at the end! 
*/

#ifndef PROGRAMS_HPP
#define PROGRAMS_HPP

#include "ComTerminal.h"
#include "sysutils.hpp"
#include "data.h"

/*===== functions =====*/

// placeholder
void test_program()
{
  char view1[DEFAULT_BUFSIZE] = "Ministry of Arcane Sciences, United Equestria.";
  int v_res = buffered_editor(view1, DEFAULT_BUFSIZE, 0, 0, 0, 1, NULL);

  print_message(view1, DEFAULT_DELAY_TIME);

  if (SD.begin(PA4))
  {
    print_message("SD test: %d", DEFAULT_DELAY_TIME, SD.exists("Test.txt"));
    SD.mkdir("test");
    SD.end();
  }
  else
    print_message("Failed to connect SD", 1000);
}

// SD file editor
void file_editor()
{
  if (SD.begin(SD_CS_PIN))
  {
    //
  }
}

/* System settings menu */
const int MAIN_SETTINGS_LEN = 7;
const char *MAIN_SETTINGS[] = {
    "Splash on/off",
    "Fancy on/off",
    "Fancy delay",
    "Login PW on/off",
    "Admin password",
    "Device name",
    "Reset device",
};
void sys_settings()
{
  int s = menu(MAIN_SETTINGS, MAIN_SETTINGS_LEN, 0, "Sys Stgs");
  if (MAIN_SETTINGS[s] == "Splash on/off")
  {
    conf->splash = conf->splash ? 0 : 1;
    print_message("Splash: %d", DEFAULT_DELAY_TIME, conf->splash);
  }
  else if (MAIN_SETTINGS[s] == "Fancy on/off")
  {
    conf->fancy = conf->fancy ? 0 : 1;
    print_message("Fancy: %d", DEFAULT_DELAY_TIME, conf->fancy);
  }
  else if (MAIN_SETTINGS[s] == "Fancy delay")
  {
    char buf[DEFAULT_BUFSIZE] = {};
    simple_input(buf, DEFAULT_BUFSIZE, "Fancy delay:", false);
    conf->fancy_delay = strtol(buf, NULL, 10);
    print_message("Fancy delay: %d", DEFAULT_DELAY_TIME, conf->fancy_delay);
  }
  else if (MAIN_SETTINGS[s] == "Login PW on/off")
  {
    conf->req_pass = conf->req_pass ? 0 : 1;
    print_message("PW on/off: %d", DEFAULT_DELAY_TIME, conf->req_pass);
  }
  else if (MAIN_SETTINGS[s] == "Admin password")
  {
    int res = password(conf->admin_pass, "Curr Admin PW");
    if (res == 1)
    {
      char buf[MAX_PASS_LEN] = {};
      buffered_editor(buf, MAX_PASS_LEN, 0, 0, 1, 1, "New PW");
      if (strtol(buf, NULL, 10) == 0)
        print_message("New PW Invalid!", DEFAULT_DELAY_TIME);
      else
      {
        strncpy(conf->admin_pass, buf, MAX_PASS_LEN);
        print_message("New PW:\n%s", DEFAULT_DELAY_TIME, conf->admin_pass);
      }
    }
    else
      print_message("Failed!", DEFAULT_DELAY_TIME);
  }
  else if (MAIN_SETTINGS[s] == "Device name")
  {
    buffered_editor(conf->device_name, MAX_NAME_LEN, 0, 0, 0, 0, "Dev Name");
    print_message("Device name:\n%s", DEFAULT_DELAY_TIME, conf->device_name);
  }
  else if (MAIN_SETTINGS[s] == "Reset device")
  {
    reset_system();
  }
  write_config();
}

/*===== program list/pointers =====*/
// Useful for calling programs from main menu
const int PROGRAM_LIST_LEN = 2;
const char *PROGRAM_NAMES[] = {
    "Test Program",
    "System Settings",
};
void (*program_ptrs[])() = {
    &test_program,
    &sys_settings,
};

#endif //PROGRAMS_HPP
