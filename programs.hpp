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

#include "MPU6050.h"

/*===== programs =====*/

// placeholder
void test_program()
{
  MPU_readout res = MPU_request();
  print_message("MPU: %f", DEFAULT_DELAY_TIME, res.temp);
}

// SD card operations
void sd_ls()
{
  char dir[13] = "/";
  buffered_editor(dir, 12, 0, 0, 1, 1, (char *)F("Dir:"));
  if (!strlen(dir))
    return;
  File root = SD.open(dir);
  File f = root.openNextFile();
  while (f)
  {
    lcd.clear();
    led_write(LED_IO, HIGH);
    for (int i = 0; f && i < D_ROWS; i++)
    {
      lcd.setCursor(0, i);
      lcd.print(f.name());
      if (f.isDirectory())
        lcd.print("/");
      lcd.setCursor(D_COLS - 4, i);
      int s = f.size();
      if (s < 1024)
      {
        lcd.print(s);
        lcd.print('B');
      }
      else if (s >= 1024 && s < 1048576)
      {
        lcd.print(s / 1024);
        lcd.print('K');
      }
      else if (s >= 1048576)
      {
        lcd.print(s / 1048576);
        lcd.print('M');
      }
      f = root.openNextFile();
    }
    led_write(LED_IO, LOW);
    char ch = keypad_wait();
    if (ch == IW_DEL_KEY || ch == ED_EXIT_KEY)
      break;
  }
}
void sd_cp()
{
  // open src file
  char src_filename[13] = {};
  buffered_editor(src_filename, 12, 0, 0, 1, 1, (char *)F("Src:"));
  File src_file = SD.open(src_filename, FILE_READ);
  if (!src_file)
  {
    print_message((char *)F("Can't open src"), DEFAULT_DELAY_TIME);
    SD.end();
    return;
  }
  // open dest file
  char dest_filename[13] = {};
  buffered_editor(dest_filename, 12, 0, 0, 1, 1, (char *)F("Dest:"));
  if (SD.exists(dest_filename))
  {
    print_message((char *)F("Dest exists"), DEFAULT_DELAY_TIME);
    if (simple_input((char *)F("Overwrite?")))
      SD.remove(dest_filename);
    else
    {
      SD.end();
      return;
    }
  }
  File dest_file = SD.open(dest_filename, FILE_WRITE);
  if (!dest_file)
  {
    print_message((char *)F("Can't open dest"), DEFAULT_DELAY_TIME);
    SD.end();
    return;
  }
  // start copying (the slow way)
  led_write(LED_IO, HIGH);
  while (src_file.available())
  {
    byte buf = src_file.read();
    dest_file.write(buf);
  }
  src_file.close();
  dest_file.close();
  if (SD.exists(dest_filename))
    print_message((char *)F("File copied"), DEFAULT_DELAY_TIME);
  led_write(LED_IO, LOW);
}
void sd_rm()
{
  // open src file
  char filename[13] = {};
  buffered_editor(filename, 12, 0, 0, 1, 1, "rm:");
  if (!SD.exists(filename))
    print_message((char *)F("File DNE"), DEFAULT_DELAY_TIME);
  else if (simple_input((char *)F("Confirm remove:")))
  {
    led_write(LED_IO, HIGH);
    SD.remove(filename);
    led_write(LED_IO, LOW);
    if (!SD.exists(filename)) // check
      print_message((char *)F("File removed"), DEFAULT_DELAY_TIME);
    else
      print_message((char *)F("Failed to remove"), DEFAULT_DELAY_TIME);
  }
}
void sd_mkdir()
{
  char dirname[9] = "";
  buffered_editor(dirname, 8, 0, 0, 1, 1, (char *)F("mkdir:"));
  if (SD.exists(dirname))
    print_message((char *)F("Dir exists"), DEFAULT_DELAY_TIME);
  else
  {
    led_write(LED_IO, HIGH);
    SD.mkdir(dirname);
    led_write(LED_IO, LOW);
    if (SD.exists(dirname))
      print_message((char *)F("Dir created"), DEFAULT_DELAY_TIME);
  }
}
void sd_rmdir()
{
  char dirname[9] = "";
  buffered_editor(dirname, 8, 0, 0, 1, 1, (char *)F("rmdir:"));
  if (!SD.exists(dirname))
    print_message((char *)F("Dir DNE"), DEFAULT_DELAY_TIME);
  else if (simple_input((char *)F("Confirm rmdir:")))
  {
    led_write(LED_IO, HIGH);
    SD.rmdir(dirname);
    led_write(LED_IO, LOW);
    if (!SD.exists(dirname)) // check to be sure
      print_message((char *)F("Dir removed"), DEFAULT_DELAY_TIME);
    else
      print_message((char *)F("Failed to remove"), DEFAULT_DELAY_TIME);
  }
}
const int SD_OPS_LEN = 5;
const char *SD_OPS[] = {
    "List dir",
    "Copy file",
    "Delete file",
    "Make dir",
    "Remove dir",
};
void (*sd_ops[])() = {
    &sd_ls,
    &sd_cp,
    &sd_rm,
    &sd_mkdir,
    &sd_rmdir,
};
void sd_ops_menu()
{
  if (!SD.begin(SD_CS_PIN))
  {
    print_message((char *)F("Failed to initialize SD"), DEFAULT_DELAY_TIME);
    return;
  }
  while (-1 != menu(SD_OPS, sd_ops, SD_OPS_LEN, 0, (char *)F("File ops")))
    0;
  SD.end();
}

// SD file editor
void file_editor()
{
  if (SD.begin(SD_CS_PIN))
  {
    char filename[13] = {};
    char buf[MAX_BUFSIZE + 1] = {};
    // get filename
    buffered_editor(filename, 12, 0, 0, 1, 1, (char *)F("File:"));
    // read file
    led_write(LED_IO, HIGH);
    File f = SD.open(filename, FILE_READ);
    f.read(buf, f.size() > MAX_BUFSIZE ? MAX_BUFSIZE : f.size());
    f.close();
    led_write(LED_IO, LOW);
    // open editor (in-place buffer)
    buffered_editor(buf, MAX_BUFSIZE, 0, 0, 0, 1, NULL);
    // save to file?
    if (simple_input((char *)F("Save file?")))
    {
      led_write(LED_IO, HIGH);
      SD.remove(filename); // only way to overwrite a file...
      f = SD.open(filename, FILE_WRITE);
      f.write(buf, strlen(buf));
      f.close();
      led_write(LED_IO, LOW);
    }
    SD.end();
  }
  else
    print_message((char *)F("Failed to initialize SD card"), 1000);
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
  int s = menu(MAIN_SETTINGS, MAIN_SETTINGS_LEN, 0, (char *)F("Sys Stgs"));
  if (MAIN_SETTINGS[s] == (char *)F("Splash on/off"))
  {
    conf->splash = conf->splash ? 0 : 1;
    print_message((char *)F("Splash: %d"), DEFAULT_DELAY_TIME, conf->splash);
  }
  else if (MAIN_SETTINGS[s] == (char *)F("Fancy on/off"))
  {
    conf->fancy = conf->fancy ? 0 : 1;
    print_message((char *)F("Fancy: %d"), DEFAULT_DELAY_TIME, conf->fancy);
  }
  else if (MAIN_SETTINGS[s] == (char *)F("Fancy delay"))
  {
    int r = simple_input((char *)F("Fancy delay:"));
    conf->fancy_delay = r > -1 ? r : 0;
    print_message((char *)F("Fancy delay: %d"), DEFAULT_DELAY_TIME,
                  conf->fancy_delay);
  }
  else if (MAIN_SETTINGS[s] == (char *)F("Login PW on/off"))
  {
    conf->req_pass = conf->req_pass ? 0 : 1;
    print_message((char *)F("PW on/off: %d"), DEFAULT_DELAY_TIME, conf->req_pass);
  }
  else if (MAIN_SETTINGS[s] == (char *)F("Admin password"))
  {
    int res = password(conf->admin_pass, (char *)F("Curr Admin PW"));
    if (res == 1)
    {
      char buf[MAX_PASS_LEN] = {};
      buffered_editor(buf, MAX_PASS_LEN, 0, 0, 1, 1, (char *)F("New PW"));
      if (strtol(buf, NULL, 10) == 0)
        print_message((char *)F("New PW Invalid!"), DEFAULT_DELAY_TIME);
      else
      {
        strncpy(conf->admin_pass, buf, MAX_PASS_LEN);
        print_message((char *)F("New PW:\n%s"), DEFAULT_DELAY_TIME,
                      conf->admin_pass);
      }
    }
    else
      print_message((char *)F("Failed!"), DEFAULT_DELAY_TIME);
  }
  else if (MAIN_SETTINGS[s] == (char *)F("Device name"))
  {
    buffered_editor(conf->device_name, MAX_NAME_LEN, 0, 0, 0, 0,
                    (char *)F("Dev Name"));
    print_message((char *)F("Device name:\n%s"), DEFAULT_DELAY_TIME, conf->device_name);
  }
  else if (MAIN_SETTINGS[s] == (char *)F("Reset device"))
  {
    reset_system();
  }
  write_config();
}

/*===== program list/pointers =====*/
// Useful for calling programs from main menu
const int PROGRAM_LIST_LEN = 4; // CHANGE ME!
const char *PROGRAM_NAMES[] = {
    "Test Program",
    "File Managing",
    "File Editor",
    "System Settings",
};
void (*program_ptrs[])() = {
    &test_program,
    &sd_ops_menu,
    &file_editor,
    &sys_settings,
};

#endif //PROGRAMS_HPP
