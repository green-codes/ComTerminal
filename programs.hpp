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
  // buffered_editor
  char view1[DEFAULT_BUFSIZE] = "Ministry of Arcane Sciences, United Equestria.";
  int v_res = buffered_editor(view1, DEFAULT_BUFSIZE, 0, 0, NULL);

  print_message(view1, DEFAULT_DELAY_TIME);
}

/* System settings menu */
const int MAIN_SETTINGS_LEN = 6;
const char *MAIN_SETTINGS[] = {
    "Splash on/off",
    "Fancy on/off",
    "Fancy delay",
    "Login PW on/off",
    "Admin password",
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
    buffered_editor(conf->admin_pass, 0, 0, 0, "Admin PW");
    print_message("PW on/off: %s", DEFAULT_DELAY_TIME, conf->admin_pass);
  }
  else if (MAIN_SETTINGS[s] == "Reset device")
  {
    reset_system();
  }
  write_config();
}

/*===== function list/pointers =====*/
// Useful for calling functions from main menu
const int PROGRAM_LIST_LEN = 2;
const char *PROGRAM_NAMES[] = {
    "Test Program",
    "Settings",
};
void (*program_ptrs[])() = {
    &test_program,
    &sys_settings,
};

#endif //PROGRAMS_HPP
