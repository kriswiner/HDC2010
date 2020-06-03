/* 06/02/2020 Copyright Tlera Corporation
 *  
 *  Created by Kris Winer
 *  
 This sketch uses SDA/SCL on pins 20/21, respectively, and it uses the Ladybug STM32L432KC Breakout Board.
 The HDC2010 is an ultra-low power humidity/temperature sensor.
 
 Library may be used freely and without limit with attribution.
 
*/

#include "HDC2010.h"

  HDC2010::HDC2010(I2Cdev* i2c_bus){
  _i2c_bus = i2c_bus;
  }


  void HDC2010::reset(uint8_t HDC2010_ADDRESS) {
  _i2c_bus->writeByte(HDC2010_ADDRESS, HDC2010_INT_STATUS, 0x80);  // soft reset device
  delay(1);
  }


  uint16_t HDC2010::getDevID(uint8_t HDC2010_ADDRESS)
  {
  uint8_t rawData[2] = {0, 0};
  rawData[0] = _i2c_bus->readByte(HDC2010_ADDRESS, HDC2010_DEV_ID_L); // read Dev ID LSByte
  rawData[1] = _i2c_bus->readByte(HDC2010_ADDRESS, HDC2010_DEV_ID_H); // read Dev ID HSByte
  uint16_t devID = ( (uint16_t) rawData[1] << 8) | rawData[0];
  return devID;
  }


  uint16_t HDC2010::getManuID(uint8_t HDC2010_ADDRESS)
  {
  uint8_t rawData[2] = {0, 0};
  rawData[0] = _i2c_bus->readByte(HDC2010_ADDRESS, HDC2010_MANU_ID_L); // read Dev ID LSByte
  rawData[1] = _i2c_bus->readByte(HDC2010_ADDRESS, HDC2010_MANU_ID_H); // read Dev ID HSByte
  uint16_t manuID = ( (uint16_t) rawData[1] << 8) | rawData[0];
  return manuID;
  }


  uint8_t HDC2010::getIntStatus(uint8_t HDC2010_ADDRESS)
  {
  uint8_t c  = _i2c_bus->readByte(HDC2010_ADDRESS, HDC2010_INT_STATUS); // read int status register
  return c;
  }
  

  void HDC2010::init(uint8_t HDC2010_ADDRESS, uint8_t hres, uint8_t tres, uint8_t freq)
  {
  // set sample frequency (bits 6 - 4), enable interrupt (bit 2 = 1), active HIGH (bit 1 = 1)
  _i2c_bus->writeByte(HDC2010_ADDRESS, HDC2010_CONFIG1, freq << 4 | 0x04 | 0x02);  
  // set temperature resolution (bits 7:6), set humidity resolution (bits 5:4), measure both H and T
  _i2c_bus->writeByte(HDC2010_ADDRESS, HDC2010_CONFIG2, tres << 6 | hres << 4 | 0x01);  
  _i2c_bus->writeByte(HDC2010_ADDRESS, HDC2010_INT_EN, 0x80); // enable data ready interrupt
  }


  float HDC2010::getTemperature(uint8_t HDC2010_ADDRESS)
  {
  uint8_t rawData[2] = {0, 0};
  rawData[0] = _i2c_bus->readByte(HDC2010_ADDRESS, HDC2010_TEMP_L); // read Temp LSByte
  rawData[1] = _i2c_bus->readByte(HDC2010_ADDRESS, HDC2010_TEMP_H); // read Temp HSByte
  float temp = (float) ( ((uint16_t) rawData[1] << 8 ) | rawData[0]);
  temp = temp *(165.0f/65536.0f) - 40.0f;
  return temp;
  }


  float HDC2010::getHumidity(uint8_t HDC2010_ADDRESS)
  {
  uint8_t rawData[2] = {0, 0};
  rawData[0] = _i2c_bus->readByte(HDC2010_ADDRESS, HDC2010_HUM_L); // read Temp LSByte
  rawData[1] = _i2c_bus->readByte(HDC2010_ADDRESS, HDC2010_HUM_H); // read Temp HSByte
  float hum = (float) ( ((uint16_t) rawData[1] << 8 ) | rawData[0]);
  hum = hum*(100.0f/65536.0f);
  return hum;
  }


  


  
