/*
 * Infrared Lap Counter for Radio-Controlled Cars
 * Copyright 2014-2015 Team RCFree44 
 */

#include "IRCarInt.h"

// Time Management
void IRCar_Race_SetTime()
{
	IRRace.time_ref = (millis()+1);
}

U32 IRCar_Race_GetTime()
{
	if (IRRace.time_ref > 0)
	{
		return (millis()+1) - IRRace.time_ref;
	}
	return 0;
}

// Reset
void IRCar_Race_Reset(Reset_t Reset)
{
	// Serial.print(F("Reset Begin: ")); Serial.println(Reset);

	// Reset Cmd
	IRCar_Command_Reset(Reset);

	// Reset Signal
	IRCar_Signal_Reset(Reset);

	// Reset Light
	IRCar_Light_Reset(Reset);

	// Reset DotMatrix
	IRCar_DotMatrix_Reset(Reset);

	// Reset Menu
	IRCar_Menu_Reset(Reset);

	if (RESET_NONE != Reset)
	{
		// Reset Car
		memset(IRCars, 0, sizeof(IRCars));
		for(U8 ID=0; ID<TAG_ID_MAX; ++ID)
		{
		#if IR_CAR_RACE__COUNT_SINCE_ZERO == 1
			// to count since zero at the first crossing
			IRCars[ID].lap_count = 0xFF;
		#endif

			// to count after the 10 first seconds
		#if IR_CAR_RACE__FIRST_LAP_MIN_TIME == 1
			IRCars[ID].reference_time = 1;
		#endif

			// reset best lap & crossing time
			IRCars[ID].best_lap_time = U32_LIMIT;
			IRCars[ID].best_crossing_time = U16_LIMIT;
		}

		// Reset Race
		memset(&IRRace, 0, sizeof(IRRace_t));
		IRRace.leaded_car = 0xFF;
	}
	// Reset Race Status
	IRRace.lap_status = IR_CAR_LAP_STATUS_NONE;

	// flush the recv
	TagRecv.decodeTAG(&TagDecod);

	// Serial.print(F("Reset End: ")); Serial.println(Reset);
}

// Sequence
static void IRCar_Race_Sequence_Start_Engine()
{
	// Start your engine !
	if (IRRace.debug_mode) { Serial.println(F("Start your engine !")); }

	// Status
	IRCar_Race_NewStatus(IR_CAR_LAP_STATUS_START_ENGINE);

	// Signal
	IRCar_Signal_Start(2000, 500, 1, true);
}
static bool IRCar_Race_Sequence_Start_Check()
{
	bool car_detected = false;

	// Status
	IRCar_Race_NewStatus(IR_CAR_LAP_STATUS_START_CHECK);

#if IR_CAR_RACE__CHECK_IDLE_AT_START == 1
	// flush the recv
	TagRecv.decodeTAG(&TagDecod);

	// wait end of detection of car Tag
	U32 TimeOut = millis();
	while((millis() - TimeOut) < IR_CAR_MIN_IDLE_TIME)
	{
		if (TagRecv.decodeTAG(&TagDecod))
		{
			// Reset Start Race in case of detection
			if ((TagDecod.channel_1 < TAG_ID_MAX) ||
					(TagDecod.channel_2 < TAG_ID_MAX) ||
					(TagDecod.channel_3 < TAG_ID_MAX) ||
					(TagDecod.channel_4 < TAG_ID_MAX)  )
			{
				car_detected = true;
				break;
			}
		}
	}
#else  // IR_CAR_RACE__CHECK_IDLE_AT_START

	// just wait the idle time
	delay(IR_CAR_CHECK_IDLE_TIME);

#endif // IR_CAR_RACE__CHECK_IDLE_AT_START

	// abort ?
	if (car_detected)
	{
		if (IRRace.debug_mode) { Serial.println(F("Canceled !")); }
		IRCar_Signal_Start(250, 250, 3, true);
		IRCar_Race_Reset();
		IRCar_Race_NewStatus();
		return false;
	}
	return true;
}
static void IRCar_Race_Sequence_Start_ReadyGo()
{
	// Ready ?
	if (IRRace.debug_mode) { Serial.println(F("Ready ?")); }
	IRCar_Race_NewStatus(IR_CAR_LAP_STATUS_START_3);
	IRCar_Signal_Start(500, 500, 1, true);
	IRCar_Race_NewStatus(IR_CAR_LAP_STATUS_START_2);
	IRCar_Signal_Start(500, 500, 1, true);
	IRCar_Race_NewStatus(IR_CAR_LAP_STATUS_START_1);
	IRCar_Signal_Start(500, 500, 1, true);

	// flush the recv
	TagRecv.decodeTAG(&TagDecod);

	// Go  !
	if (IRRace.debug_mode) { Serial.println(F("Go !")); }
	IRCar_Race_SetTime();
	IRCar_Race_NewStatus(IR_CAR_LAP_STATUS_RUN);
	IRCar_Signal_Start(1000, 0, 1);
}

// Start
void IRCar_Race_Start(U8 MaxLap, U32 MaxTime, U8 DebugMode)
{
	if (DebugMode) { Serial.print(F("Start Race, with MaxLap: ")); Serial.print(MaxLap); Serial.print(F(", MaxTime:")); Serial.println(MaxTime); }

	// reset & set
	IRCar_Race_Reset(RESET_RACE);
	IRRace.lap_max      = MaxLap;
	IRRace.lap_time_max = MaxTime;
	IRRace.debug_mode   = DebugMode;

	// sequence
	IRCar_Race_Sequence_Start_Engine();

	// sequence
	if (IRCar_Race_Sequence_Start_Check())
	{
		// sequence
		IRCar_Race_Sequence_Start_ReadyGo();
	}
}

// New Race's Status ?
void IRCar_Race_NewStatus(U8 State)
{
	if (State != IRRace.lap_status)
	{
		if (State < IR_CAR_LAP_STATUS_NB)
		{
			IRRace.lap_status = State;
		}
		IRCar_Status_Race_Send();
		IRCar_Light_Update();
		IRCar_DotMatrix_Update();
		IRCar_Menu_Update();
	}
}

// Live Race's Update ?
void IRCar_Race_RaceUpdate(U8 Lap_Downcount)
{
	IRCar_DotMatrix_RaceUpdate(Lap_Downcount);
}

// Count New Car's Lap ?
bool IRCar_Race_CountNewLap(U8 ID)
{
	bool UpdateRace = false;

	// When the Race is finished : terminate "lasts laps" counting (until it reach the leader score)
	if (IRRace.lap_status != IR_CAR_LAP_STATUS_RUN &&
			IRRace.lap_status != IR_CAR_LAP_STATUS_LAST_LAP)
	{
		if (IRCars[ID].lap_count < IRCars[IRRace.leaded_car].lap_count)
		{
			// Count Car
			if (IRRace.debug_mode) { Serial.print(F("Lasting Run for 0x")); Serial.println(ID, HEX); }
			// continue
		}
		else
		{
			// No more counting for *this* car
			if (IRRace.debug_mode) { Serial.print(F("Free Run for 0x")); Serial.println(ID, HEX); }
			return false;
		}
	}
	else
	{
		// Normal Race updating
		UpdateRace = true;
	}

	// Count Car & Performances
	++IRCars[ID].lap_count;

	// Best Lap ever ?
	U32 LapTime = IRCars[ID].reference_time - IRCars[ID].last_reference_time;
	if (LapTime < IRCars[ID].best_lap_time)
	{
		IRCars[ID].best_lap_time = LapTime;
		if (IRRace.debug_mode) { Serial.print(F("Best Lap-Time for 0x")); Serial.print(ID, HEX); Serial.print(F(", ")); Serial.print(IRCars[ID].best_lap_time); Serial.println(F(" ms"));}
	}
	IRCars[ID].last_reference_time = IRCars[ID].reference_time;

	// Best Crossing time ?
	if (IRCars[ID].crossing_time < IRCars[ID].best_crossing_time)
	{
		IRCars[ID].best_crossing_time = IRCars[ID].crossing_time;
		if (IRRace.debug_mode) { Serial.print(F("Best Crossing-Time for 0x")); Serial.print(ID, HEX); Serial.print(F(", ")); Serial.print(IRCars[ID].best_crossing_time); Serial.println(F(" ms"));}
	}
	IRCars[ID].total_crossing_time += IRCars[ID].crossing_time;

	//
	// Update the Race
	//
	if (UpdateRace)
	{
		// Update Leader
		if ((IRRace.leaded_car >= TAG_ID_MAX) ||
				(IRCars[ID].lap_count > IRCars[IRRace.leaded_car].lap_count))
		{
			// New Leader
			if (IRRace.debug_mode) { Serial.print(F("New Leader 0x")); Serial.print(IRRace.leaded_car, HEX); Serial.print(F(" -> 0x"));Serial.println(ID, HEX); }
			if (IRRace.leaded_car != ID)
			{
				IRRace.leaded_car = ID;
			#if IR_CAR_RACE__LEADER_SEND_STATUS == 1
				IRCar_Status_Race_Send();
			#endif
			}
		}

		// Update Race conditions when the Leader crossing
		if (ID == IRRace.leaded_car)
		{
			// Lap Limit
			if (IRRace.lap_max > 0)
			{
				// Notice for Lap Downcount
				IRCar_Race_RaceUpdate(IRRace.lap_max - IRCars[IRRace.leaded_car].lap_count);

				// End Lap ?
				if (IRCars[IRRace.leaded_car].lap_count == IRRace.lap_max)
				{
					if (IRRace.debug_mode) { Serial.print(F("(LapLimit) Race Finished, winner is 0x")); Serial.println(IRRace.leaded_car, HEX); }
					IRCar_Signal_Start(2000, 500, 3); // not blocking
					IRCar_Race_NewStatus(IR_CAR_LAP_STATUS_FINISHED);
				}
				// Final Lap ?
				else if (IRCars[IRRace.leaded_car].lap_count == (IRRace.lap_max-1))
				{
					if (IRRace.debug_mode) { Serial.print(F("(LapLimit) Final Lap, leader is 0x")); Serial.println(IRRace.leaded_car, HEX); }
					// IRCar_Signal_Start(1000, 500, 1); // not blocking
					IRCar_Race_NewStatus(IR_CAR_LAP_STATUS_LAST_LAP);
				}
			}

			// Time Limit
			if ((IRRace.lap_time_max > 0) && (IRCar_Race_GetTime() > IRRace.lap_time_max))
			{
				if (IRRace.lap_status == IR_CAR_LAP_STATUS_LAST_LAP)
				{
					if (IRRace.debug_mode) { Serial.print(F("(TimeLimit) Race Finished, winner is 0x")); Serial.println(IRRace.leaded_car, HEX); }
					IRCar_Signal_Start(2000, 500, 3); // not blocking
					IRCar_Race_NewStatus(IR_CAR_LAP_STATUS_FINISHED);
				}
				else if (IRRace.lap_status == IR_CAR_LAP_STATUS_RUN)
				{
					if (IRRace.debug_mode) { Serial.print(F("(TimeLimit) Final Lap, leader is 0x")); Serial.println(IRRace.leaded_car, HEX); }
					// IRCar_Signal_Start(1000, 500, 1); // not blocking
					IRCar_Race_NewStatus(IR_CAR_LAP_STATUS_LAST_LAP);
				}
			}
		}
	}

	// Counted
	return true;
}

// Compute TopPos
U8 IRCar_Race_GetTopPos(U8 ID)
{
	U8 TopPos = 0;
	if ((ID < TAG_ID_MAX) && (IRCars[ID].lap_count > 0))
	{
		// Compute current Score
		U32 IDCar_Score = (U32)(IRCars[IRRace.leaded_car].lap_count - IRCars[ID].lap_count) << 20;
		IDCar_Score += IRCars[ID].race_reference_time;

		// Scan to find out better cars
		TopPos = 1;
		for(U8 i=0; i<TAG_ID_MAX; ++i)
		{
			// Car Score
			U32 Score = (U32)(IRCars[IRRace.leaded_car].lap_count - IRCars[i].lap_count) << 20;
			Score += IRCars[i].race_reference_time;

			// Best ?
			if (Score < IDCar_Score)
			{
				++TopPos;
			}
		}
	}
	// Serial.print("GetTopPos for 0x"); Serial.print(ID, HEX); Serial.print(" is:"); Serial.println(TopPos);
	return TopPos;
}

// Simulation
void IRCar_Race_Simulation(U8 MaxLap, U32 MaxTime, U8 DebugMode)
{
	// reset & set
	if (DebugMode) { Serial.print(F("Start Simu, with MaxLap: ")); Serial.print(MaxLap); Serial.print(F(", MaxTime:")); Serial.println(MaxTime); }
	IRCar_Race_Reset(RESET_RACE);
	IRRace.lap_max      = MaxLap;
	IRRace.lap_time_max = MaxTime;
	IRRace.debug_mode   = DebugMode;

	// sequence
	IRCar_Race_Sequence_Start_Engine();

	// sequence
	if (IRCar_Race_Sequence_Start_Check())
	{
		// sequence
		IRCar_Race_Sequence_Start_ReadyGo();

		// Generate random 4 cars ID
		U8  ID1 = random(0, TAG_ID_MAX-4);
		U8  ID2 = ID1+1;
		U8  ID3 = ID2+1;
		U8  ID4 = ID3+1;

		// Generate random 4 lap time (inital
		U32 LP1 = random(500, 1000);
		U32 LP2 = random(500, 1000);
		U32 LP3 = random(500, 1000);
		U32 LP4 = random(500, 1000);

		// simulation mode
		while(IRRace.lap_status != IR_CAR_LAP_STATUS_NONE)
		{
			U8  NewCar = 0;
			U32 Time   = IRCar_Race_GetTime();
			if (Time >= LP1)
			{
				if (IRCar_Car_Process(ID1, Time))
				{
					++NewCar;
					IRCar_Car_Process(ID1, Time+random(50, 100));
				}
				LP1 += random(IR_CAR_MIN_LAP_TIME, IR_CAR_MIN_LAP_TIME+2*1000UL);
			}
			if (Time >= LP2)
			{
				if (IRCar_Car_Process(ID2, Time))
				{
					++NewCar;
					IRCar_Car_Process(ID2, Time+random(50, 100));
				}
				LP2 += random(IR_CAR_MIN_LAP_TIME, IR_CAR_MIN_LAP_TIME+2*1000UL);
			}
			if (Time >= LP3)
			{
				if (IRCar_Car_Process(ID3, Time))
				{
					++NewCar;
					IRCar_Car_Process(ID3, Time+random(50, 100));
				}
				LP3 += random(IR_CAR_MIN_LAP_TIME, IR_CAR_MIN_LAP_TIME+2*1000UL);
			}
			if (Time >= LP4)
			{
				if (IRCar_Car_Process(ID4, Time))
				{
					++NewCar;
					IRCar_Car_Process(ID4, Time+random(50, 100));
				}
				LP4 += random(IR_CAR_MIN_LAP_TIME, IR_CAR_MIN_LAP_TIME+2*1000UL);
			}
			// Signal new cars
			IRCar_Signal_AddBip(NewCar);

			// Other Process update
			IRCar_AllUpdates();
		}
	}

	// End
	if (IRRace.debug_mode) { Serial.println(F("End of Simu.")); }
	IRCar_Race_Reset();// End
}