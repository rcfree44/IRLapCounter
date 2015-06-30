/*
 * Infrared Lap Counter for Radio-Controlled Cars
 * Copyright 2014-2015 Team RCFree44 
 */

#include "IRCarInt.h"

// Command
static char CmdBuffer[IR_CAR_CMD_MAX_SIZE];
static U8 CmdBuffer_Size;
static U8 CmdBuffer_Time;

// NewCmd
static U8  Cmd_Param0;
static U8  Cmd_Param1;
static U32 Cmd_Param2;
static U16 Cmd_Param3;

// Reset
void IRCar_Command_Reset(Reset_t Reset)
{
	CmdBuffer_Size = 0;
	CmdBuffer_Time = millis();
}

// Execute
static void IRCar_Command_Execute(bool AllowNewCommand)
{
	switch(Cmd_Param0 /* Mode */)
	{
		// >00|00:00000000@0000
		case IR_CAR_COMMAND_RESET:
		{
			//Serial.println("IRCar Reset");
			IRCar_Race_Reset();
		}
		break;

		// >01|00:00000000@0000
		case IR_CAR_COMMAND_SEND_STATUS:
		{
			//Serial.println("IRCar Send Status");
			IRCar_Status_Race_Send();
		}
		break;

		// >02|00:00000000@0000
		case IR_CAR_COMMAND_START_STATUS:
		{
			//Serial.println("IRCar Start Status");
			IRRace.last_time_status = IRCar_Race_GetTime();
		}
		break;

		// >03|00:00000000@0000
		case IR_CAR_COMMAND_STOP_STATUS:
		{
			//Serial.println("IRCar Stop Status");
			IRRace.last_time_status = 0;
		}
		break;

		// >10|00:00000000@0000
		//     11:22222222 3333
		// 11: max lap (0: no limit)
		// 22: max time in ms (0 : no limit)
		// 33: verbose mode (0 : no, 1 : yes)

		// >10|04:00000000@0001 (4 lap race with debug traces)
		// >10|00:0000EA60@0001 (1mn race with debug traces)
		// >10|03:0000AFC8@0001 (45sec/3lap race with debug traces)
		// >10|00:00000000@0000 (infinit race w/o debug traces)

		case IR_CAR_COMMAND_START_RACE:
		{
			//Serial.println("IRCar Start Race");
			if (AllowNewCommand)
			{
				IRCar_Race_Start(Cmd_Param1/*MaxLap*/, Cmd_Param2/*MaxTime*/, Cmd_Param3/*DebugMode*/);
			}
		}
		break;

		// >20|00:00000000@0000
		/*
		case IR_CAR_COMMAND_LAST_LAP:
		{
			//Serial.println("IRCar Last Lap");
			if (IRRace.lap_status == IR_CAR_LAP_STATUS_RUN)
			{
				IRRace.lap_time_max = 1;
			}
		}
		break;
		*/

		// >21|00:00000000@0000
		/*
		case IR_CAR_COMMAND_FINISH:
		{
			// Serial.print("Race Finished, winner is: "); Serial.println(IRRace.leaded_car, DEC);
			IRCar_Signal_Start(2000, 500, 1); // not blocking
			IRCar_Race_NewStatus(IR_CAR_LAP_STATUS_FINISHED);
		}
		break;
		*/

		// >30|00:00000000@0000
		/*case IR_CAR_COMMAND_TEST_MODE:
		{
		}
		break;*/

		// >31|00:00000000@0000
		//     11:22222222 3333
		// 11: max lap (0: no limit)
		// 22: max time in ms (0 : no limit)
		// 33: verbose mode (0 : no, 1 : yes)
		//
		// >31|04:00000000@0001 (4 lap race with debug traces)
		// >31|00:0000EA60@0001 (1mn race with debug traces)
		// >31|03:0000AFC8@0001 (45sec/3lap race with debug traces)
		// >31|00:00000000@0000 (infinit race w/o debug traces)
		case IR_CAR_COMMAND_SIMULATION:
		{
			if (AllowNewCommand)
			{
				IRCar_Race_Simulation(Cmd_Param1, Cmd_Param2, Cmd_Param3);
			}
		}
		break;

		default:
		{
		//  Serial.println("ERROR");
		}
		break;
	}
}

// Decod
static void IRCar_Command_Decod(bool AllowNewCommand)
{
	char * ptr = CmdBuffer;
	U8 size  = CmdBuffer_Size;

	// >MM|LL:TTTTTTTT@PPPP
	// >12|34:45678ABC@DEF0
	if (size-- && *ptr++ == '>' && size >= 2)
	{
		Cmd_Param0 = GetHexNumber8(ptr); ptr+= 2; size-=2;
		if (size-- && *ptr++ == '|' && size >= 2)
		{
			Cmd_Param1 = GetHexNumber8(ptr); ptr+= 2; size-=2;
			if (size-- && *ptr++ == ':' && size >= 8)
			{
				Cmd_Param2 = GetHexNumber32(ptr); ptr+= 8; size-=8;
				if (size-- && *ptr++ == '@' && size >= 4)
				{
					Cmd_Param3 = GetHexNumber16(ptr); ptr+= 4; size-=4;

					// Serial.print("Param0: 0x");  Serial.println(Cmd_Param0, HEX);
					// Serial.print("Param1: 0x");   Serial.println(Cmd_Param1, HEX);
					// Serial.print("Param2: 0x");  Serial.println(Cmd_Param2, HEX);
					// Serial.print("Param3: 0x"); Serial.println(Cmd_Param3, HEX);
					// Serial.println();

					// Execute
					IRCar_Command_Execute(AllowNewCommand);
				}
			}
		}
	}
	CmdBuffer_Size = 0;
}

// Process
void IRCar_Command_Process(bool AllowNewCommand /* = false */)
{
	int c = Serial.read();
	if (c >= 0)
	{
		if (c == '\r' || c == '\n')
		{
			IRCar_Command_Decod(AllowNewCommand);
		}
		else if (CmdBuffer_Size < IR_CAR_CMD_MAX_SIZE)
		{
			CmdBuffer[CmdBuffer_Size++] = (char)c;
		}
		if (CmdBuffer_Size == IR_CAR_CMD_MAX_SIZE)
		{
			IRCar_Command_Decod(AllowNewCommand);
		}
		CmdBuffer_Time = (U8)millis();
	}
	// Reset
	else if ((CmdBuffer_Size > 0) &&
					(((U8)millis() - CmdBuffer_Time) >= IR_CAR_CMD_TIMEOUT))
	{
		IRCar_Command_Decod(AllowNewCommand);
	}
}
