/*
 * Infrared Lap Counter for Radio-Controlled Cars
 * Copyright 2014-2015 Team RCFree44 
 */


#include "IRCarInt.h"

// System
U16  IRCar_TimeMs;
bool IRCar_Power_OK;
U8   IRCar_AutoTest_LastID;

// IR
IRTagDecod_t TagDecod;
IRTagRecv    TagRecv;

// Data
IRRace_t IRRace;
IRCar_t  IRCars[TAG_ID_MAX];
IRSave_t IRSave;

// Refresh
static U16 FastRefreshTimeRef;
static U16 NormRefreshTimeRef;
static U16 SlowRefreshTimeRef;

// LoopCounter
#ifdef IR_CAR_LOOP_COUNTER
static U16 LoopCounter;
#endif

// Battery Check
static U8 Alarm_Count;

// Init
void IRCar_Init(void)
{
  // Reset Alarm
	IRCar_Battery_Init();

	// Alarm OFF
	Alarm_Count = 0;

	// Check the Battery level
	IRCar_Power_OK = IRCar_Battery_Check();
	// Serial.print("Initial Power State: "); Serial.println(IRCar_Power_OK);
	
	// Reset Race + Send initial Status
	IRCar_Race_Reset(RESET_INIT);
	IRCar_Status_Race_Send();
	
	// Save
	memset(&IRSave, 0, sizeof(IRSave_t));
	IRSave.MaxLap = 3; // default value

	// Time
	IRCar_TimeMs       = millis();
	FastRefreshTimeRef = IRCar_TimeMs;
	NormRefreshTimeRef = IRCar_TimeMs;
	SlowRefreshTimeRef = IRCar_TimeMs;

	// Debug
#ifdef IR_CAR_LOOP_COUNTER
	LoopCounter = 0;
#endif
#ifdef IR_CAR_LOOP_LOAD_PIN
	pinMode(IR_CAR_LOOP_LOAD_PIN, OUTPUT);
#endif

  // Reset Test
  IRCar_AutoTest_LastID = TAG_ID_MAX;

	// Enable Reception
	TagRecv.enableIRIn();
}

// AllUpdate
void IRCar_AllUpdates(bool AllowNewCommand /* = false */)
{
	// Update Command
	IRCar_Command_Process(AllowNewCommand);

	// Update Global Time
	IRCar_TimeMs = millis();

	// Update Signal
	IRCar_Signal_Process();

	// Update Fast Process
	if ((IRCar_TimeMs - FastRefreshTimeRef) >= IR_CAR_FAST_REFRESH_PERIOD)
	{
		// Process Status (send)
		IRCar_Status_Process();

		// Reset timer at the end of operation (to avoid overload)
		FastRefreshTimeRef = IRCar_TimeMs;
	}
	// Update Norm Process
	else if ((IRCar_TimeMs - NormRefreshTimeRef) >= IR_CAR_NORM_REFRESH_PERIOD)
	{
		// Update Light & DotMatrix
		IRCar_Light_Process();
		IRCar_DotMatrix_Process();

		// Reset timer at the end of operation (to avoid overload)
		NormRefreshTimeRef = IRCar_TimeMs;
	}
	// Update Slow Process
	else if ((IRCar_TimeMs - SlowRefreshTimeRef) >= IR_CAR_SLOW_REFRESH_PERIOD)
	{
		// Check the Battery condition (returns "true" is alright)
		if (!IRCar_Battery_Check())
		{
			// Count Alarm
			if (Alarm_Count < IR_CAR_ALARM__COUNT_DETECTION)
			{
				++Alarm_Count;
				// Serial.print(F("Alarm Count:")); Serial.println(Alarm_Count);
			}
			// Cut the "big" Power
			else
			{
				if (IRCar_Power_OK)
				{
					// Reset Power
					// Serial.println(F("Power NOK"));
					IRCar_Power_OK = false;

					// Force Update
					IRCar_Race_NewStatus();
				}
				// Force Reset Hold
				Alarm_Count = IR_CAR_ALARM__COUNT_RESETHOLD;
			}

			// 10 short Bips !
			IRCar_Signal_Start(150, 150, 4);
		}
		else if (Alarm_Count > 0)
		{
			--Alarm_Count;
			// Serial.print(F("Alarm Count:")); Serial.println(Alarm_Count);
			if (0 == Alarm_Count)
			{
				// restart the "big" power
				// Serial.println(F("Power OK"));
				IRCar_Power_OK = true;

				// Force Update
				IRCar_Race_NewStatus();
			}
		}

		// Update Menu (CAUTION time consumming !!!)
		IRCar_Menu_Process(AllowNewCommand);

		// Reset timer at the end of operation (to avoid overload)
		SlowRefreshTimeRef = IRCar_TimeMs;

		// Debug
	#ifdef IR_CAR_LOOP_COUNTER
		Serial.print(F("Loop Counter: ")); Serial.println(LoopCounter);
		LoopCounter = 0;
	#endif
	}
	else if (IRRace.lap_status == IR_CAR_LAP_STATUS_NONE)
	{
		// Full Update for Menu
		IRCar_Menu_Process(AllowNewCommand);
	}
}

// process
void IRCar_Process(void)
{
	// Debug
#ifdef IR_CAR_LOOP_COUNTER
	++LoopCounter;
#endif
#ifdef IR_CAR_LOOP_LOAD_PIN
	if (LoopCounter % 2)
	{
		digitalWrite(IR_CAR_LOOP_LOAD_PIN, HIGH);
	}
	else
	{
		digitalWrite(IR_CAR_LOOP_LOAD_PIN, LOW);
	}
#endif

	// Race is running ?
	if (IRRace.lap_status != IR_CAR_LAP_STATUS_NONE)
	{
		// Get common Time
		U32 Time = IRCar_Race_GetTime();

		// Tag Reception
		if (TagRecv.decodeTAG(&TagDecod))
		{
			U8 NewCar = 0;

			// Process Tag 1 to 4
			if (IRCar_Car_Process(TagDecod.channel_1, Time))
			{
				++NewCar;
			}
			if (IRCar_Car_Process(TagDecod.channel_2, Time))
			{
				++NewCar;
			}
			if (IRCar_Car_Process(TagDecod.channel_3, Time))
			{
				++NewCar;
			}
			if (IRCar_Car_Process(TagDecod.channel_4, Time))
			{
				++NewCar;
			}

			// Signal new cars
			if (NewCar)
			{
				IRCar_Signal_AddBip(NewCar);
			}
		}

		// Reset Test
    IRCar_AutoTest_LastID = TAG_ID_MAX;
	}
	else
	{
		// IR AutoTest
		if (TagRecv.decodeTAG(&TagDecod))
		{
			// Test Mode (save & bip)
			U8 LastID = TAG_ID_MAX;
			if (((LastID = TagDecod.channel_1) < TAG_ID_MAX) ||
					((LastID = TagDecod.channel_2) < TAG_ID_MAX) ||
					((LastID = TagDecod.channel_3) < TAG_ID_MAX) ||
					((LastID = TagDecod.channel_4) < TAG_ID_MAX)  )
			{
  			// Save for further printing
				IRCar_AutoTest_LastID = LastID;
  			
  			// Bip
				IRCar_Signal_TryBip();
			}
		}
	}

	// Updates
	IRCar_AllUpdates(true /* AllowNewCommand */);
}

