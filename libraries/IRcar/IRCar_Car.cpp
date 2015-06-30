/*
 * Infrared Lap Counter for Radio-Controlled Cars
 * Copyright 2014-2015 Team RCFree44 
 */

#include "IRCarInt.h"

// Car Process
bool IRCar_Car_Process(U8 ID, U32 Time)
{
  bool newcar = false;
  if (ID < TAG_ID_MAX)
  {
    switch(IRCars[ID].car_status)
    {
      case IR_CAR_CAR_WAITING:
      {
        if (
          #if IR_CAR_RACE__FIRST_LAP_MIN_TIME == 0        
            (0 == IRCars[ID].reference_time) ||
          #endif
            (IRCars[ID].reference_time + IR_CAR_MIN_LAP_TIME) <= Time)
        {
          // New Lap (not yet confirmed)
          IRCars[ID].reference_time     = Time;
          IRCars[ID].crossing_time      = 0;
          IRCars[ID].car_status         = IR_CAR_CAR_CROSSING;
  
          // Serial.print("New Lap: "); Serial.print(ID, HEX);
          // Serial.print(", Val: ");   Serial.println(IRCars[ID].lap_count, DEC);*/ 
          
          newcar = true;        
        }
      }
      break;

      case IR_CAR_CAR_CROSSING:
      {
        // Crossing
        U32 delta = (Time - IRCars[ID].reference_time);
        if (delta > 0)
        {
          IRCars[ID].reference_time = Time;
          if (((U32)IRCars[ID].crossing_time + delta) <= 0xFFFF)
          {
            IRCars[ID].crossing_time += delta;
            
            // Serial.print("Crossing: "); Serial.print(ID, HEX);
            // Serial.print(", Delta: ");   Serial.println(delta, DEC);
          }
        }
      }
      break;
    }
  }
  return newcar;
}

// Car Status Update & Send
void IRCar_Car_Status_Update_Send(U32 Time)
{
  // Update Status
  for(U8 ID=0; ID<TAG_ID_MAX; ++ID)
  {
    // Does this car confirmed ?
    if (IR_CAR_CAR_CROSSING == IRCars[ID].car_status)
    {
      // Compute RefTime
      U32 RefTime = IRCars[ID].reference_time - (U32)(IRCars[ID].crossing_time/2);
      
      // Is Long Enought to be "confirmed" ?
      if ((RefTime + IR_CAR_MIN_CONFIRMED_TIME) <= Time)
      {
        // Avoid null crossing time
        if (0 == IRCars[ID].crossing_time)
        {
          IRCars[ID].crossing_time = 1;
        }

        // Confirmed Car
        IRCars[ID].reference_time = RefTime;
        IRCars[ID].car_status     = IR_CAR_CAR_CONFIRMED;
      }
    }
  }
  
  // Update LapCount
  for(U8 ID=0; ID<TAG_ID_MAX; ++ID)
  {
    // Looking for the Best *confirmed* Car
    U8  BestID   = TAG_ID_MAX;
    U32 BestTime = U32_LIMIT;
    for(U8 subID=0; subID<TAG_ID_MAX; ++subID)
    {
      if ((IR_CAR_CAR_CONFIRMED == IRCars[subID].car_status) && (IRCars[subID].reference_time < BestTime))
      {
        BestTime = IRCars[subID].reference_time;
        BestID   = subID;
      }
    }

    // Found the best Car to count ?
    if (BestID < TAG_ID_MAX)
    {
      // Count New Lap ?
      if (IRCar_Race_CountNewLap(BestID))
      {
        // Update Race Time (if counted)
        IRCars[BestID].race_reference_time = IRCars[BestID].reference_time;

        // Send New Car status
        IRCar_Status_Car_Send(BestID);
      }

      // Reset Car status
      IRCars[BestID].car_status = IR_CAR_CAR_WAITING;
    }
    else
    {
      // exit update loop
      break;
    }
  }
}