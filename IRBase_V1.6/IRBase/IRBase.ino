/*
 * Infrared Lap Counter for Radio-Controlled Cars
 * Copyright 2014-2015 Team RCFree44 
 */

#include <EEPROM.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <LIBcommon.h>
#include <IRTag.h>
#include <IRCar.h>

// Main
void setup()
{
  // Serial Init for Command/Status/Debug interface (Serial / Bluetooth / Wifi / etc. in transparent mode)
  Serial.begin(115200);
  
  // Reset Race
  IRCar_Init();
}

void loop() 
{
  IRCar_Process();
}
