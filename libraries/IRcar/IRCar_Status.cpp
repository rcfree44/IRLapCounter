/*
 * Infrared Lap Counter for Radio-Controlled Cars
 * Copyright 2014-2015 Team RCFree44 
 */

#include "IRCarInt.h"

// Status
static char ComStatus[22+1];

// Car Status
static void IRCar_CarStatus(U8 ID)
{
  // init
  char *ptr = ComStatus;
  memset(ComStatus, 0, sizeof(ComStatus));

  // set
  if (ID < TAG_ID_MAX)
  {
    *ptr++ = '#';
    PutHexNumber8(ptr, ID); ptr+=2; 
    *ptr++ = '|';
    PutHexNumber8(ptr, IRCars[ID].lap_count); ptr+=2;
    *ptr++ = ':';
    PutHexNumber32(ptr, IRCars[ID].race_reference_time); ptr+=8; 
    *ptr++ = '@';
    PutHexNumber16(ptr, IRCars[ID].crossing_time); ptr+=4;
    *ptr++ = '\r';
    *ptr   = '\n';
    
    // Serial.print("ID: ");   Serial.print(ID, HEX); 
    // Serial.print(",LAP: "); Serial.print(IRCars[ID].lap_count, HEX);
    // Serial.print(",TIM: "); Serial.print(IRCars[ID].race_reference_time, HEX);
    // Serial.print(",CRO: "); Serial.print(IRCars[ID].crossing_time, HEX);
    // Serial.println(".");
  }
}

// Race Status
static void IRCar_RaceStatus(void)
{
  // init
  char *ptr = ComStatus;
  memset(ComStatus, 0, sizeof(ComStatus));
  
  // set
  *ptr++ = '<';
  PutHexNumber8(ptr, IRRace.lap_mode); ptr+=2;
  *ptr++ = '|';
  PutHexNumber8(ptr, IRRace.lap_status); ptr+=2;
  *ptr++ = ':';
  PutHexNumber32(ptr, IRCar_Race_GetTime()); ptr+=8;
  *ptr++ = '@';
  PutHexNumber8(ptr, IRRace.leaded_car); ptr+=2;
  PutHexNumber8(ptr, (IRRace.leaded_car < TAG_ID_MAX) ? (IRCars[IRRace.leaded_car].lap_count) : 0); ptr+=2;
  *ptr++ = '\r';
  *ptr   = '\n';
}

void IRCar_Status_Race_Send()
{
  IRCar_RaceStatus();
  Serial.print(ComStatus);  
}

void IRCar_Status_Car_Send(U8 ID)
{
  IRCar_CarStatus(ID);
  Serial.print(ComStatus);  
}

// process
void IRCar_Status_Process(void)
{
  U32 Time = IRCar_Race_GetTime();  

  // Car Status Update & Send
  IRCar_Car_Status_Update_Send(Time);

  // Race Status
  if ((IRRace.last_time_status > 0) && 
      ((Time - IRRace.last_time_status) >= IR_CAR_RACE_STATUS_PERIOD))
  {
    // Generate status
    IRCar_Status_Race_Send();
    IRRace.last_time_status = IRCar_Race_GetTime();
  }
}
