/* 06/02/2020 Copyright Tlera Corporation
 *  
 *  Created by Kris Winer
 *  
 This sketch uses SDA/SCL on pins 20/21, respectively, and it uses the Ladybug STM32L432KC Breakout Board.
 The HDC2010 is an ultra-low power humidity/temperature sensor.

 In this case we are using two different HDC2010 with two different I2C addresses on the same i2C bus.
 
 Library may be used freely and without limit with attribution.
 
*/
  
#include "HDC2010.h"
#include <RTC.h>

#define I2C_BUS          Wire               // Define the I2C bus (Wire instance) you wish to use

I2Cdev                   i2c_0(&I2C_BUS);   // Instantiate the I2Cdev object and point to the desired I2C bus

// Define pins
#define myLed    A0  // blue led
#define myVbat   A4  // LiPo battery voltage monitor
#define myButton 39
#define myVbatEn 39
#define intPin0   8
#define intPin1  A1

const char        *build_date = __DATE__;   // 11 characters MMM DD YYYY
const char        *build_time = __TIME__;   // 8 characters HH:MM:SS

float VDDA, Temperature;
uint32_t UID[3] = {0, 0, 0};
volatile bool USBConnected = false; 
volatile bool SerialDebug = true;

bool serialDebug = true;

// RTC time labels
uint8_t seconds, minutes, hours, day, month, year;  
uint8_t Seconds, Minutes, Hours, Day, Month, Year;
volatile bool alarmFlag = false;

// Configure HCD2010
// Choices are:
// freq = ForceMode, Freq_120s, Freq_60s, Freq_10s, Freq_5s, Freq_1s, Freq_0_5s, Freq_0_2s
// tres = TRES_14bit, TRES_11bit, TRES_9bit
// hres = HRES_14bit, HRES_11bit, HRES_9bit
uint8_t freq = Freq_5s, tres = TRES_14bit, hres = HRES_14bit;

float temperature0 = 0.0f, humidity0 = 0.0f, temperature1 = 0.0f, humidity1 = 0.0f;
uint8_t intStatus0, intStatus1;
volatile bool intFlag0 = false, intFlag1 = false;

HDC2010 HDC2010(&i2c_0);

void setup() {

  Serial.begin(115200);  
  delay(2000);
  if(SerialDebug)   Serial.println("Serial enabled!");

  STM32.getUID(UID);
  if(SerialDebug) {  Serial.print("STM32L4 MCU UID = 0x"); Serial.print(UID[0], HEX); Serial.print(UID[1], HEX); Serial.println(UID[2], HEX);} 

  pinMode(myLed, OUTPUT);
  digitalWrite(myLed, HIGH);  // Start with led off, active LOW 

  pinMode(myVbatEn, OUTPUT);
  digitalWrite(myVbatEn, LOW);
  
  pinMode(myVbat, INPUT);
  analogReadResolution(12);

  pinMode(intPin0, INPUT); // active HIGH
  pinMode(intPin1, INPUT); // active HIGH
 
  I2C_BUS.begin();                        // Set master mode, default on SDA/SCL for STM32L4
  delay(1000);
  I2C_BUS.setClock(400000);               // I2C frequency at 400 kHz
  delay(1000);
  
  if(serialDebug)Serial.println("Scan for I2C devices:");
  i2c_0.I2Cscan();                                      // should detect HDC2010   
  delay(1000);

  uint16_t devID0 = HDC2010.getDevID(HDC2010_0_ADDRESS);
  Serial.print("DeviceID of 0= 0x0"); Serial.print(devID0, HEX); Serial.println(". Should be 0x07D0");

  uint16_t manuID0 = HDC2010.getManuID(HDC2010_0_ADDRESS);
  Serial.print("Manufacturer's ID of 0 = 0x"); Serial.print(manuID0, HEX); Serial.println(". Should be 0x5449");

  uint16_t devID1 = HDC2010.getDevID(HDC2010_1_ADDRESS);
  Serial.print("DeviceID of 0= 0x0"); Serial.print(devID1, HEX); Serial.println(". Should be 0x07D0");

  uint16_t manuID1 = HDC2010.getManuID(HDC2010_1_ADDRESS);
  Serial.print("Manufacturer's ID of 1 = 0x"); Serial.print(manuID1, HEX); Serial.println(". Should be 0x5449");

  HDC2010.reset(HDC2010_0_ADDRESS);
  HDC2010.reset(HDC2010_1_ADDRESS);

  // Configure HCD2010 for auto measurement mode if freq not ForceMode
  // else measurement performed only once each time init is called
  HDC2010.init(HDC2010_0_ADDRESS, hres, tres, freq); 
  HDC2010.init(HDC2010_1_ADDRESS, hres, tres, freq); 
  
  // Set the time
  SetDefaultRTC();
  
  // set alarm to update the RTC every minute
//  RTC.enableAlarm(RTC.MATCH_SS); // alarm once a minute
  RTC.enableAlarm(RTC.MATCH_ANY); // alarm once a second

  RTC.attachInterrupt(alarmMatch);

  attachInterrupt(intPin0, myinthandler0, RISING);  // define interrupt for INT pin output of HDC2010_0
  attachInterrupt(intPin1, myinthandler1, RISING);  // define interrupt for INT pin output of HDC2010_1

  // read interrupt status register(s) to unlatch interrupt before entering main loop
  intStatus0  = HDC2010.getIntStatus(HDC2010_0_ADDRESS);
  intStatus1  = HDC2010.getIntStatus(HDC2010_1_ADDRESS);
 
  /* end of setup */
}


void loop() {

  if(intFlag0)
  {
    intFlag0 = false; // reset HDC2010_0 interrupt flag
    
    temperature0 = HDC2010.getTemperature(HDC2010_0_ADDRESS);
    humidity0 = HDC2010.getHumidity(HDC2010_0_ADDRESS);

    if(SerialDebug) {
      Serial.print("HDC2010_0 Temperature is "); Serial.print(temperature0, 2); Serial.println(" degrees C");
    }
    if(SerialDebug) {
      Serial.print("HDC2010_0 Humidity is "); Serial.print(humidity0, 2); Serial.println(" %RH"); Serial.println(" ");
    }
  }

    if(intFlag1)
  {
    intFlag1 = false; // reset HDC2010_1 interrupt flag
    
    temperature1 = HDC2010.getTemperature(HDC2010_1_ADDRESS);
    humidity1 = HDC2010.getHumidity(HDC2010_1_ADDRESS);

    if(SerialDebug) {
      Serial.print("HDC2010_1 Temperature is "); Serial.print(temperature1, 2); Serial.println(" degrees C");
    }
    if(SerialDebug) {
      Serial.print("HDC2010_1 Humidity is "); Serial.print(humidity1, 2); Serial.println(" %RH"); Serial.println(" ");
    }
  }
  
  // RTC Alarm handling
  if(alarmFlag)
  { 
    alarmFlag = false;  // reset RTC alarm flag

  VDDA = STM32.getVREF();
  Temperature = STM32.getTemperature();
  USBConnected = USBDevice.connected();
  digitalWrite(myVbatEn, HIGH);
  float VBAT = 1.27f * VDDA * (float) analogRead(myVbat) / 4096.0f;
  digitalWrite(myVbatEn, LOW);

  if(SerialDebug)   Serial.print("VDDA = "); Serial.println(VDDA, 2); 
  if(SerialDebug)   Serial.print("STM32L4 MCU Temperature = "); Serial.println(Temperature, 2);
  if(USBConnected && SerialDebug) Serial.println("USB connected!");
  if(SerialDebug)   Serial.print("VBAT = "); Serial.println(VBAT, 2);

  digitalWrite(myLed, HIGH); delay(10); digitalWrite(myLed, LOW);

  } // end of RTC Alarm handling

  STM32.sleep(); // wait in STOP for an interrupt
}
 /* end of main loop */


/* Useful functions*/
     
void myinthandler0()
{
  intFlag0 = true;
}


void myinthandler1()
{
  intFlag1 = true;
}


void alarmMatch()
{
  alarmFlag = true;
}


void SetDefaultRTC()  // Sets the RTC to the FW build date-time...
{
  char Build_mo[3];

  // Convert month string to integer

  Build_mo[0] = build_date[0];
  Build_mo[1] = build_date[1];
  Build_mo[2] = build_date[2];

  String build_mo = Build_mo;

  if(build_mo == "Jan")
  {
    month = 1;
  } else if(build_mo == "Feb")
  {
    month = 2;
  } else if(build_mo == "Mar")
  {
    month = 3;
  } else if(build_mo == "Apr")
  {
    month = 4;
  } else if(build_mo == "May")
  {
    month = 5;
  } else if(build_mo == "Jun")
  {
    month = 6;
  } else if(build_mo == "Jul")
  {
    month = 7;
  } else if(build_mo == "Aug")
  {
    month = 8;
  } else if(build_mo == "Sep")
  {
    month = 9;
  } else if(build_mo == "Oct")
  {
    month = 10;
  } else if(build_mo == "Nov")
  {
    month = 11;
  } else if(build_mo == "Dec")
  {
    month = 12;
  } else
  {
    month = 1;     // Default to January if something goes wrong...
  }

  // Convert ASCII strings to integers
  day     = (build_date[4] - 48)*10 + build_date[5] - 48;  // ASCII "0" = 48
  year    = (build_date[9] - 48)*10 + build_date[10] - 48;
  hours   = (build_time[0] - 48)*10 + build_time[1] - 48;
  minutes = (build_time[3] - 48)*10 + build_time[4] - 48;
  seconds = (build_time[6] - 48)*10 + build_time[7] - 48;

  // Set the date/time

  RTC.setDay(day);
  RTC.setMonth(month);
  RTC.setYear(year);
  RTC.setHours(hours);
  RTC.setMinutes(minutes);
  RTC.setSeconds(seconds);
}
