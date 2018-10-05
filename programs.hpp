/*
 * Header for functions/applications
 * 
 * Note: functions below should take no args and return void. 
*/

#ifndef PROGRAMS_HPP
#define PROGRAMS_HPP

#include "base.h"
#include "devices.h"
#include "sysutils.h"
#include "data.h"

/*===== testing programs =====*/
void test_MPU()
{
  if (!MPU_enabled)
    return;
  monitor(MPU_display);
}
void test_SD()
{
  if (SD.begin())
  {
    print_message((char *)F("Writing to SD"), 0);
    mcp.digitalWrite(LED_IO, HIGH);
    File f = SD.open("3", FILE_WRITE);
    for (int i = 0; i < UINT16_MAX; i++)
      f.println(i);
    print_message((char *)F("SD test: %d"), DEFAULT_DELAY_TIME, f.size());
    f.close();
  }
  else
    print_message((char *)F("Failed SD init!"), DEFAULT_DELAY_TIME);
  SD.end();
  mcp.digitalWrite(LED_IO, LOW);
}
void jmp_stub()
{
  // should have enough space on stack to jump to another program
  print_message("Addr:0x%.8x", DEFAULT_DELAY_TIME, &jmp_stub);
}
const char *TEST_PGMS[] = {
    "MPU test",
    "SD write test",
    "JMP STUB",
};
void (*test_pgms[])() = {
    &test_MPU,
    &test_SD,
    &jmp_stub};
const int TEST_PGMS_LEN = sizeof(test_pgms) / sizeof(void(*));
void pgm_tests()
{
  while (-1 != menu(TEST_PGMS, test_pgms, TEST_PGMS_LEN,
                    0, (char *)F("Tests")))
    0;
}

/*===== SD card operations =====*/
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
    mcp.digitalWrite(LED_IO, HIGH);
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
    mcp.digitalWrite(LED_IO, LOW);
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
  // start copying
  char buf[MAX_BUFSIZE + 1] = {};
  int count = 0, num = src_file.size();
  lcd.clear();
  lcd.setCursor(0, 1);
  lcd.print((char *)F("Copying *=EXIT"));
  mcp.digitalWrite(LED_IO, HIGH);
  while (count < num)
  {
    if (kpd.getKey() == '*')
      break;
    int bytes_to_write = MAX_BUFSIZE < num - count ? MAX_BUFSIZE : num - count;
    src_file.read(buf, bytes_to_write);
    dest_file.write(buf, bytes_to_write);
    dest_file.flush(); // clear the dest_file buffer!
    memset(buf, 0, MAX_BUFSIZE + 1);
    count += bytes_to_write;
    lcd.setCursor(0, 0);
    lcd.print(count);
  }
  mcp.digitalWrite(LED_IO, LOW);
  src_file.close();
  dest_file.close();
  if (SD.exists(dest_filename))
    print_message((char *)F("File copied"), DEFAULT_DELAY_TIME);
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
    mcp.digitalWrite(LED_IO, HIGH);
    SD.remove(filename);
    mcp.digitalWrite(LED_IO, LOW);
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
    mcp.digitalWrite(LED_IO, HIGH);
    SD.mkdir(dirname);
    mcp.digitalWrite(LED_IO, LOW);
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
    mcp.digitalWrite(LED_IO, HIGH);
    SD.rmdir(dirname);
    mcp.digitalWrite(LED_IO, LOW);
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
    print_message((char *)F("Failed to initialize SD"), DEFAULT_DELAY_TIME);
  else
    while (-1 != menu(SD_OPS, sd_ops, SD_OPS_LEN, 0, (char *)F("File ops")))
      0;
  SD.end();
}

/* ===== SD file editor ===== */
void file_editor()
{
  if (SD.begin(SD_CS_PIN))
  {
    char filename[13] = {};
    char buf[MAX_BUFSIZE + 1] = {};
    // get filename
    buffered_editor(filename, 12, 0, 0, 1, 1, (char *)F("File:"));
    // read file
    mcp.digitalWrite(LED_IO, HIGH);
    File f = SD.open(filename, FILE_READ);
    f.read(buf, f.size() > MAX_BUFSIZE ? MAX_BUFSIZE : f.size());
    f.close();
    mcp.digitalWrite(LED_IO, LOW);
    // open editor (in-place buffer)
    buffered_editor(buf, MAX_BUFSIZE, 0, 0, 0, 1, NULL);
    // save to file?
    if (simple_input((char *)F("Save file?")))
    {
      mcp.digitalWrite(LED_IO, HIGH);
      SD.remove(filename); // only way to overwrite a file...
      f = SD.open(filename, FILE_WRITE);
      f.write(buf, strlen(buf));
      f.close();
      mcp.digitalWrite(LED_IO, LOW);
    }
    SD.end();
  }
  else
    print_message((char *)F("Failed to initialize SD card"), 1000);
}

/* ===== Memory editor (dangerous) ===== */
void memory_editor()
{
  char *addr = (char *)simple_input("ADDR:", 16);
  int num = simple_input("NUM:");
  buffered_editor(addr, num, 0, 1, 0, 0, NULL);
}

/* ===== System settings menu ===== */
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
const char *PROGRAM_NAMES[] = {
    "Test Programs",
    "File Managing",
    "File Editor",
    "Memory Editor",
    "System Settings",
};
void (*program_ptrs[])() = {
    &pgm_tests,
    &sd_ops_menu,
    &file_editor,
    &memory_editor,
    &sys_settings,
};
const int PROGRAM_LIST_LEN = sizeof(program_ptrs) / sizeof(void(*));

#endif //PROGRAMS_HPP
