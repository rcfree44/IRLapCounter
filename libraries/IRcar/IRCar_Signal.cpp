/*
 * Infrared Lap Counter for Radio-Controlled Cars
 * Copyright 2014-2015 Team RCFree44 
 */

#include "IRCarInt.h"

// Signal
static U8   Signal_CountDown;
static U8   Signal_BipCount;
static U16  Signal_Duration_ON;
static U16  Signal_Duration_OFF;
static U16  Signal_TimeRef; 
static bool Signal_State;

static void Signal_ON(void)
{
  Signal_State   = true;      
  digitalWrite(IR_CAR_SIGNAL_LED, HIGH);
  //Serial.print("Signal ON: "); Serial.println((U16)millis());
}

static void Signal_OFF(void)
{
  Signal_State   = false;      
  digitalWrite(IR_CAR_SIGNAL_LED, LOW);
  //Serial.print("Signal OFF: "); Serial.println((U16)millis());
}

// Reset
void IRCar_Signal_Reset(Reset_t Reset)
{
  if (RESET_INIT == Reset)
  {
    // pin config
    pinMode(IR_CAR_SIGNAL_LED, OUTPUT);
  }

  if (RESET_NONE != Reset)
  {
    // Off  
    Signal_CountDown = Signal_BipCount = 0;
    Signal_OFF();
  }
}

// Start
void IRCar_Signal_Start(U16 ON_Duration, U16 OFF_Duration, U8 Repeat, bool WaitEnd)
{
  // reset
  IRCar_Signal_Reset(RESET_RACE);
  
  // Update Global Time
  IRCar_TimeMs = millis();
  
  // start
  Signal_Duration_ON  = ON_Duration;
  Signal_Duration_OFF = OFF_Duration;
  Signal_CountDown    = (2* Repeat) + 1;
  Signal_TimeRef      = IRCar_TimeMs - Signal_Duration_OFF;
  
  // Wait End ?
  if (WaitEnd)
  {
    while((Signal_CountDown > 0) || (Signal_BipCount > 0))
    {
      IRCar_TimeMs = millis(); // refresh global time 
      IRCar_Signal_Process();
    }
  }
}

// AddBip (low priority)
void IRCar_Signal_AddBip(U8 Count)
{
  Signal_BipCount += Count;
}

// TryBip (low priority)
void IRCar_Signal_TryBip(void)
{
  if (Signal_BipCount == 0)
  {
    Signal_BipCount = 1;
  }
}

// PlayBit (blocking access)
void IRCar_Signal_PlayBip(void)
{
  IRCar_Signal_Reset(RESET_RACE);
  Signal_BipCount = 1;
  while((Signal_CountDown > 0) || (Signal_BipCount > 0))
  {
    IRCar_TimeMs = millis(); // refresh global time 
    IRCar_Signal_Process();
  }
}  

// Process
void IRCar_Signal_Process(void)
{
  if (Signal_CountDown > 0)
  {
    U16 FlashTime = IRCar_TimeMs - Signal_TimeRef;
    // Set ON ?
    if (!Signal_State && FlashTime >= Signal_Duration_OFF)
    {
      //Serial.print("0->1 ("); Serial.print(Signal_CountDown, DEC); Serial.print(") : "); Serial.println(FlashTime, DEC);
      --Signal_CountDown;
      if (Signal_CountDown > 0)
      {
        Signal_ON();
      }
      Signal_TimeRef = IRCar_TimeMs;
    }
    // Set OFF ?
    else if (Signal_State && FlashTime >= Signal_Duration_ON)
    {
      //Serial.print("1->0 ("); Serial.print(Signal_CountDown, DEC); Serial.print(") : "); Serial.println(FlashTime, DEC);
      --Signal_CountDown;
      Signal_OFF();
      Signal_TimeRef = IRCar_TimeMs;
    }
  }
  else if (Signal_BipCount > 0)
  {
    --Signal_BipCount;

    // start new bip
    Signal_Duration_ON  = IR_CAR_SIGNAL_BIP_ON;
    Signal_Duration_OFF = IR_CAR_SIGNAL_BIP_OFF;
    Signal_CountDown    = 2 + 1;
    Signal_TimeRef      = IRCar_TimeMs;
  }
}

