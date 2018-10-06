/*
 * higher level driver modules
 * Note: add device-specific libraries here
 * 
 */

#ifndef DEVICES_H
#define DEVICES_H

#include "base.h"

// hardware libs
#include <LiquidCrystal.h> // parallel
#include <Key.h>
#include <Keypad.h> // parallel
#include <SD.h>     // hardcoded to use SPI1
#include <Adafruit_MCP23008.h>
#include <Adafruit_GPS.h>

/*===== global vars =====*/
// Note: device instances must NOT reference each other during init!
Adafruit_MCP23008 mcp = Adafruit_MCP23008(); // first MCP expander
Adafruit_GPS gps(&Serial1);
//RTClock rtc = RTClock(RTCSEL_LSE); // the GPS module has a reliable RTC.
LiquidCrystal lcd(LCD_RS, LCD_EN, LCD_D4, LCD_D5, LCD_D6, LCD_D7);
Keypad kpd = Keypad(makeKeymap(KEYPAD_KEYS), KEYPAD_ROW_PINS,
                    KEYPAD_COL_PINS, KEYPAD_ROWS, KEYPAD_COLS);
CT_Config *conf = NULL; // pointer to config struct

/* ===== LCD helpers ===== */
// print lines
void print_lines(char *const buf, int bufsize, byte force,
                 int num_rows, int row_size, int start_row)
{
  // if not in force mode, cut bufsize to first null
  if (!force || bufsize == 0)
    bufsize = strlen(buf);
  int row_count = 0, num_printed = 0;
  // print num_lines or until end of buf
  for (int i = 0; i < bufsize && num_printed < num_rows * row_size; i++)
  {
    // handle new line character
    // TODO: breaks buffered_editor
    // if (buf[i] == '\n')
    // {
    //   if (row_count < num_rows)
    //     lcd.setCursor(0, start_row + row_count++);
    //   num_printed += row_size - (i % row_size);
    //   i++;
    // }
    // else
    num_printed++;
    // line wrap
    if (i % row_size == 0 && row_count < num_rows)
      lcd.setCursor(0, start_row + row_count++);
    // print character
    if (buf[i] == 0) // always substitute nulls with spaces
      lcd.write(' ');
    else
      lcd.write(buf[i]);
  }
}
void print_line(char *const buf, byte force, int start_row)
{
  print_lines(buf, D_COLS, force, 1, D_COLS, start_row);
}
// lcd printf
void printf_lcd(char *format, ...)
{
  char *p_buf = (char *)calloc(D_COLS + 2, 1);
  va_list args;
  va_start(args, format);
  vsnprintf(p_buf, D_COLS + 1, format, args);
  va_end(args);
  lcd.print(p_buf);
  free(p_buf);
}

// print helpers
void fancy_print(const char *buf)
{
  if (!buf)
    return;
  if (!conf->fancy)
  {
    lcd.print(buf);
    return;
  }
  int len = strlen(buf);
  for (int i = 0; i < len; i++)
  {
    lcd.print(buf[i]);
    delay(conf->fancy_delay);
  }
}
void fancy_print(const long num)
{
  char temp[12] = {}; // max 12 digits
  sprintf(temp, "%d", num);
  fancy_print(temp);
}
void fancy_print(const double num)
{
  char temp[12] = {}; // max 12 digits
  sprintf(temp, "%f", num);
  fancy_print(temp);
}
void hex_print(const char data)
{
  char temp[2];
  sprintf(temp, "%.2x", (byte)data);
  lcd.print(temp);
}
void hex_print(const char *data, int num)
{
  for (int i = 0; i < num; i++)
  {
    hex_print(data[i]);
  }
}

/* ===== keypad helpers ===== */
// NOTE: watch for serial bus conflicts!
char keypad_wait()
{
  if (LED_ENABLE)
    mcp.digitalWrite(LED_WAIT, HIGH); // indicate waiting for input
  while (true)
  {
    if (SERIAL_ENABLE)
    { // Note: serial input takes precedence if enabled
      unsigned char ch_s = Serial.read();
      if (ch_s != 255)
      {
        mcp.digitalWrite(LED_WAIT, LOW);
        return ch_s;
      }
    }
    unsigned char ch_k = kpd.getKey();
    if (ch_k != 0)
    {
      mcp.digitalWrite(LED_WAIT, LOW);
      if (conf->tone_en)
        tone(TONE_PIN, TONE_KEY_FREQ, TONE_KEY_DELAY);
      return ch_k;
    }
    delay(10);
  }
}

/*===== Emulated EEPROM handling ===== */
// Note: possible 1-byte overflow; make sure num is even
// Note: ptr is unprotected; know what you're doing
void ee_write(uint16_t address, byte *ptr, int num)
{
  uint16_t buf; // 2-byte buffer for writing
  while (num > 0)
  {
    buf = *(uint16_t *)ptr;
    EEPROM.write(address, buf);
    address++;
    ptr += 2;
    num -= 2;
  }
}
byte *ee_read(uint16_t address, byte *ptr, int num)
{
  uint16_t buf; // 2-byte buffer for reading
  byte *orig_ptr = ptr;
  while (num > 0)
  {
    EEPROM.read(address, &buf);
    *(uint16_t *)ptr = buf;
    address++;
    ptr += 2;
    num -= 2;
  }
  return orig_ptr; // for convenience
}

/* ===== SD card functions ===== */
int sd_log(char *buf, char *filename)
{
  if (!buf || !filename || !SD.begin(SD_CS_PIN))
    return -1;
  mcp.digitalWrite(LED_IO, HIGH);
  File f = SD.open(filename, FILE_WRITE);
  f.write(buf);
  f.close();
  SD.end();
  mcp.digitalWrite(LED_IO, LOW);
  return 0;
}

/* ===== MPU6050 driver ===== 
Source (partial): https://www.stm32duino.com/viewtopic.php?f=9&t=4048&p=48546&hilit=mpu6050#p48546 */
#define MPU_ADDRESS 0b1101000
typedef struct MPU_readout
{
  double x_accel;
  double y_accel;
  double z_accel;
  double temp;
  double x_gyro;
  double y_gyro;
  double z_gyro;
} MPU_readout;
void setupMPU()
{
  mcp.digitalWrite(LED_IO, HIGH);
  // delay(50); // ???
  Wire.beginTransmission(0x68);
  Wire.write(0x6B);
  Wire.write(0x00);
  Wire.endTransmission();
  // begin MPU initialization
  Wire.beginTransmission(MPU_ADDRESS); //This is the I2C address of the MPU (b1101000/b1101001 for AC0 low/high datasheet sec. 9.2)
  Wire.write(0x6B);                    //Accessing the register 6B - Power Management (Sec. 4.28)
  Wire.write(0b00000000);              //Setting SLEEP register to 0. (Required; see Note on p. 9)
  Wire.endTransmission();
  Wire.beginTransmission(MPU_ADDRESS); //I2C address of the MPU
  Wire.write(0x1B);                    //Accessing the register 1B - Gyroscope Configuration (Sec. 4.4)
  Wire.write(0x00000000);              //Setting the gyro to full scale +/- 250deg./s
  Wire.endTransmission();
  Wire.beginTransmission(MPU_ADDRESS); //I2C address of the MPU
  Wire.write(0x1C);                    //Accessing the register 1C - Acccelerometer Configuration (Sec. 4.5)
  Wire.write(0b00000000);              //Setting the accel to +/- 2g
  Wire.endTransmission();
  mcp.digitalWrite(LED_IO, LOW);
}
MPU_readout MPU_request()
{
  mcp.digitalWrite(LED_IO, HIGH);
  Wire.beginTransmission(MPU_ADDRESS); //I2C address of the MPU
  Wire.write(0x3B);                    //Starting register for Readings
  Wire.endTransmission();
  Wire.requestFrom(MPU_ADDRESS, 14); //Request Accel Registers (3B - 48)
  while (Wire.available() < 14)
    delay(1);                                   //wait for buffer to fill
  long accelX = Wire.read() << 8 | Wire.read(); //Store into accelX
  long accelY = Wire.read() << 8 | Wire.read(); //Store into accelY
  long accelZ = Wire.read() << 8 | Wire.read(); //Store into accelZ
  long temp = Wire.read() << 8 | Wire.read();   // Store temp
  long gyroX = Wire.read() << 8 | Wire.read();  //Store into accelX
  long gyroY = Wire.read() << 8 | Wire.read();  //Store into accelY
  long gyroZ = Wire.read() << 8 | Wire.read();  //Store into accelZ
  MPU_readout res = MPU_readout();
  res.x_gyro = gyroX / 131.0;
  res.y_gyro = gyroY / 131.0;
  res.z_gyro = gyroZ / 131.0;
  res.temp = (temp + 12412.0) / 340.0;
  res.x_accel = accelX / 16384.0;
  res.y_accel = accelY / 16384.0;
  res.z_accel = accelZ / 16384.0;
  mcp.digitalWrite(LED_IO, LOW);
  return res;
}
void MPU_display() // for 4-line displays
{
  MPU_readout res = MPU_request();
  lcd.clear();
  printf_lcd((char *)F("MPU6050 T:%5.1f"), res.temp);
  lcd.setCursor(0, 1);
  printf_lcd((char *)F("aX:%4.2f gX:%5.3f"), res.x_accel, res.x_gyro);
  lcd.setCursor(0, 2);
  printf_lcd((char *)F("aY:%4.2f gY:%5.3f"), res.y_accel, res.y_gyro);
  lcd.setCursor(0, 3);
  printf_lcd((char *)F("aZ:%4.2f gZ:%5.3f"), res.z_accel, res.z_gyro);
}

/* ===== GPS functions ===== */
// get processed GPS data
void GPS_display()
{
  lcd.clear();
  // print regardless if there's new data
  printf_lcd((char *)F("GPS:%d %.2d:%.2d:%.2d"), gps.satellites,
             gps.hour, gps.minute, gps.seconds);
  lcd.setCursor(0, 1);
  printf_lcd((char *)F("lat:%f"), gps.latitude_fixed / 10000000.0);
  lcd.setCursor(0, 2);
  printf_lcd((char *)F("lon:%f"), gps.longitude_fixed / 10000000.0);
  lcd.setCursor(0, 3);
  printf_lcd((char *)F("alt: %f"), gps.altitude);
}
void GPS_update()
{
  gps.read();
  if (gps.newNMEAreceived())
    gps.parse(gps.lastNMEA());
}

/* ===== System functions ===== */
void handle_exi() // handle external interrupt
{
  tone(TONE_PIN, TONE_CONFIRM_FREQ, 1000);
  // TODO
}
void handle_tmi() // handle scheduled interrupt
{
  gpio_write_bit(GPIOC, 13, HIGH);
  // add system interrupts here
  GPS_update();
  gpio_write_bit(GPIOC, 13, LOW);
}
void reset_system()
{
  lcd.clear();
  lcd.print((char *)F("Reseting system"));
  nvic_sys_reset();
}

#endif