/*
  Header for functions/applications

  Note: functions below should take no args and return void. 
*/

#ifndef FUNCTIONS_HPP
#define FUNCTIONS_HPP

#include "ComTerminal.h"
#include "sysutils.hpp"

/*===== functions =====*/

// placeholder
void sys_settings()
{
  return;
}

/*===== function list/pointers =====*/
// Useful for calling functions from main menu
char *function_names[] = {
    "Settings",
};
void (*function_pointers[])() = {
    &sys_settings,
};

#endif //FUNCTIONS_HPP
