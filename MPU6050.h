/*
    MPU6050 handling and data processing

    Source (partial): https://www.stm32duino.com/viewtopic.php?f=9&t=4048&p=48546&hilit=mpu6050#p48546
*/

#ifndef MPU6050_H
#define MPU6050_H

//#include <SoftWire.h>
#include <Wire.h>
#include "ComTerminal.h"

//use IIC1
TwoWire Wire1(1, I2C_FAST_MODE);

#define MPU_ADDRESS 0b1101000
#define MPU_CLOCK 400000
#define MPU_TIMEOUT 1000000 // in microseconds
#define MPU_TIMEOUT_TIMER 2

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
  lcd.clear();
  lcd.print("MPU init...");
  led_write(LED_IO, HIGH);
  Wire1.setClock(MPU_CLOCK);
  Wire1.begin();
  delay(50); // wait for MPU wakeup
  Wire1.beginTransmission(0x68);
  Wire1.write(0x6B);
  Wire1.write(0x00);
  Wire1.endTransmission();
  // begin MPU initialization
  Wire1.beginTransmission(MPU_ADDRESS); //This is the I2C address of the MPU (b1101000/b1101001 for AC0 low/high datasheet sec. 9.2)
  Wire1.write(0x6B);                    //Accessing the register 6B - Power Management (Sec. 4.28)
  Wire1.write(0b00000000);              //Setting SLEEP register to 0. (Required; see Note on p. 9)
  Wire1.endTransmission();
  Wire1.beginTransmission(MPU_ADDRESS); //I2C address of the MPU
  Wire1.write(0x1B);                    //Accessing the register 1B - Gyroscope Configuration (Sec. 4.4)
  Wire1.write(0x00000000);              //Setting the gyro to full scale +/- 250deg./s
  Wire1.endTransmission();
  Wire1.beginTransmission(MPU_ADDRESS); //I2C address of the MPU
  Wire1.write(0x1C);                    //Accessing the register 1C - Acccelerometer Configuration (Sec. 4.5)
  Wire1.write(0b00000000);              //Setting the accel to +/- 2g
  Wire1.endTransmission();
  led_write(LED_IO, LOW);
  print_message("MPU initialized", DEFAULT_DELAY_TIME);
}

MPU_readout MPU_request()
{
  led_write(LED_IO, HIGH);
  Wire1.beginTransmission(MPU_ADDRESS); //I2C address of the MPU
  Wire1.write(0x3B);                    //Starting register for Readings
  Wire1.endTransmission();
  Wire1.requestFrom(MPU_ADDRESS, 14); //Request Accel Registers (3B - 48)
  while (Wire1.available() < 14)
    delay(1);                                     //wait for buffer to fill
  long accelX = Wire1.read() << 8 | Wire1.read(); //Store into accelX
  long accelY = Wire1.read() << 8 | Wire1.read(); //Store into accelY
  long accelZ = Wire1.read() << 8 | Wire1.read(); //Store into accelZ
  long temp = Wire1.read() << 8 | Wire1.read();   // Store temp
  long gyroX = Wire1.read() << 8 | Wire1.read();  //Store into accelX
  long gyroY = Wire1.read() << 8 | Wire1.read();  //Store into accelY
  long gyroZ = Wire1.read() << 8 | Wire1.read();  //Store into accelZ
  MPU_readout res = MPU_readout();
  res.x_gyro = gyroX / 131.0;
  res.y_gyro = gyroY / 131.0;
  res.z_gyro = gyroZ / 131.0;
  res.temp = (temp + 12412.0) / 340.0;
  res.x_accel = accelX / 16384.0;
  res.y_accel = accelY / 16384.0;
  res.z_accel = accelZ / 16384.0;
  led_write(LED_IO, LOW);
  return res;
}

#endif