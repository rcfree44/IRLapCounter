/*
 * Infrared Lap Counter for Radio-Controlled Cars
 * Copyright 2014-2015 Team RCFree44 
 */


#include "IRCarInt.h"

bool IRCar_Battery_LipoDetected;

#define USE_32BITS_COMPUTATION 0

// Returns Vin in mV [0 to 15000]
#if USE_32BITS_COMPUTATION == 1
	#define VIN_FACTOR 1120121UL // average Factor value mesured with 3 different 2S Lipo batteries
	static U16 ReadVin(void)
	{
	#if !defined(__AVR_ATmega2560__)
		// Read Raw Sensor after 4.7K/10K divisor
		U32 val = analogRead(IO_ANA_VIN);
		// Apply correction
		return ((val * VIN_FACTOR) + 32768UL) >> 16;
	#else
		return 5000;
	#endif
	}
#else
	static U16 ReadVin(void)
	{
	#if !defined(__AVR_ATmega2560__)
		// Read Raw Sensor after 4.7K/10K divisor
		// with a un-stable Vanalog ref
		U16 Vin = analogRead(IO_ANA_VIN);
		return (Vin << 4) + Vin;
	#else
		return 5000;
	#endif
	}
#endif

// ----------------------------
void IRCar_Battery_Init(void)
{
	IRCar_Battery_LipoDetected = false;
}

// ----------------------------
bool IRCar_Battery_Check(void)
{
	// Read Vin voltage
	U16 mV = ReadVin();
	// Serial.print("Vin in mV: "); Serial.print(mV); Serial.print(", Lipo: "); Serial.println(IRCar_Battery_LipoDetected);

	// Check it inside correct values
	if ((mV >= 4800) && (mV <= 5000))
	{
		// USB Powered: dont set the alarm
		if (!IRCar_Battery_LipoDetected)
		{
			return true;
		}
	}
	else if ((mV >= IR_CAR_ALARM__2SLIPO_MV_MIN) &&
					 (mV <= IR_CAR_ALARM__2SLIPO_MV_MAX)  )
	{
		// Correction 2S Voltage
		IRCar_Battery_LipoDetected = true;
		return true;
	}
	// Serial.print("Vin in mV: "); Serial.print(mV); Serial.print(", Lipo: "); Serial.println(IRCar_Battery_LipoDetected);

	// Error Case
	return false;
}

// ----------------------------
U8 IRCar_Battery_Voltage(void)
{
	// Read Vin voltage
	U16 mV = ReadVin();

	// Check it inside correct values
	if ((mV >= 4800) && (mV <= 5000))
	{
		// USB Powered: no level
		if (!IRCar_Battery_LipoDetected)
		{
			return 99;
		}
	}
	else if ((mV >= IR_CAR_ALARM__2SLIPO_MV_MIN) &&
					 (mV <= IR_CAR_ALARM__2SLIPO_MV_MAX)  )
	{
		// compute level
		U8 Level_100 = (100UL * (mV - IR_CAR_ALARM__2SLIPO_MV_MIN) +
										 ((IR_CAR_ALARM__2SLIPO_MV_MAX - IR_CAR_ALARM__2SLIPO_MV_MIN)/2))
												/ (IR_CAR_ALARM__2SLIPO_MV_MAX - IR_CAR_ALARM__2SLIPO_MV_MIN);
		if (Level_100 >= 100) Level_100 = 99;
		return Level_100; // level 99
	}

	// Error Case
	return 0;
}