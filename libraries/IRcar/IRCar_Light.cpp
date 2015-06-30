/*
 * Infrared Lap Counter for Radio-Controlled Cars
 * Copyright 2014-2015 Team RCFree44 
 */

#include "IRCarInt.h"

// Timer
static U16 Light_TimerON;

// Cmd
#define NONE    0
#define RED1    1
#define RED2    2
#define RED3    4
#define GREEN   8
static void Light_Cmd(U8 Bits)
{
	// Get the PMW value for On
	U8 OnPWM = IR_CAR_LIGHT_PWM_MAXON;

	// Force Reset ?
	if (!IRCar_Power_OK || !IRCar_Battery_LipoDetected)
	{
		Bits = 0;
	}

	// Set/Reset
	analogWrite(IR_CAR_LIGHT_PWM_RED1, (Bits & RED1) ? OnPWM : 0);
	analogWrite(IR_CAR_LIGHT_PWM_RED2, (Bits & RED2) ? OnPWM : 0);
	analogWrite(IR_CAR_LIGHT_PWM_RED3, (Bits & RED3) ? OnPWM : 0);
	analogWrite(IR_CAR_LIGHT_PWM_GREEN, (Bits & GREEN) ? OnPWM : 0);

	// Debug
	// Serial.print("Light: "); Serial.println(Bits, HEX);

	// Set/Reset Timer ?
	if (Bits)
	{
  	// Update Global Time
    IRCar_TimeMs = millis();
  	
		Light_TimerON = IRCar_TimeMs;
		// no null Timer => reserved for stopped state
		if (!Light_TimerON) Light_TimerON = 1;
	}
	else
	{
		Light_TimerON = 0;
	}
}

// Reset
void IRCar_Light_Reset(Reset_t Reset)
{
	if (RESET_NONE != Reset)
	{
		Light_Cmd(NONE);
	}
}

// Update
void IRCar_Light_Update(void)
{
	// Power Issue ?
	if (!IRCar_Power_OK)
	{
		Light_Cmd(NONE);
		return;
	}

	// Update on Status
	switch(IRRace.lap_status)
	{
		case IR_CAR_LAP_STATUS_START_ENGINE:
		{
			Light_Cmd(RED1 | RED2 | RED3 | GREEN);
		}
		break;

		case IR_CAR_LAP_STATUS_START_CHECK:
		{
			Light_Cmd(NONE);
		}
		break;

		case IR_CAR_LAP_STATUS_START_3:
		{
			Light_Cmd(RED1);
		}
		break;

		case IR_CAR_LAP_STATUS_START_2:
		{
			Light_Cmd(RED2);
		}
		break;

		case IR_CAR_LAP_STATUS_START_1:
		{
			Light_Cmd(RED3);
		}
		break;

		case IR_CAR_LAP_STATUS_RUN:
		{
			Light_Cmd(GREEN);
		}
		break;

		case IR_CAR_LAP_STATUS_LAST_LAP:
		{
			Light_Cmd(RED1 | GREEN);
		}
		break;

		case IR_CAR_LAP_STATUS_FINISHED:
		{
			Light_Cmd(RED1 | RED2 | RED3);
		}
		break;

		default:
		{
			Light_Cmd(NONE);
		}
		break;
	}
}

// Process
void IRCar_Light_Process(void)
{
	// Power Issue ?
	if (!IRCar_Power_OK)
	{
		Light_Cmd(NONE);
		return;
	}

	// Execute
	if (Light_TimerON && ((IRCar_TimeMs - Light_TimerON) >= IR_CAR_LIGHT_AUTO_STOP))
	{
		Light_Cmd(NONE);
	}
}