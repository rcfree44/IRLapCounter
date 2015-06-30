/*
 * Infrared Lap Counter for Radio-Controlled Cars
 * Copyright 2014-2015 Team RCFree44 
 */

#include "IRCar.h"
#include "IRCarInt.h"
#include "Wire.h"
#include "LiquidCrystal_I2C.h"

// LCD
static LiquidCrystal_I2C Menu_LCD(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);  // Set the LCD I2C address

// Buttons
typedef enum
{
	BUTTON_NONE,
	BUTTON_1,
	BUTTON_2,
	BUTTON_3,
	BUTTON_4,
} Button_t;

// Menu
typedef enum
{
	MENU_NONE,
	MENU_INIT,
	MENU_IDLE,
	MENU_RACE_STARTING,
	MENU_RACE_CHECKING,
	MENU_RACE_RUNNING,
	MENU_RACE_FINISHED,
	MENU_RACE_END,
} Menu_t;

//
static Menu_t Menu_Current;
static U16    Menu_TimeOut;
static U16    Menu_BackLightTimeON;
static bool   Menu_NoMoreInit;

// Backlight Management
static void IRCar_Menu_BackLight(bool On_Off)
{
  // Serial.print("Backlight, OnOff: "); Serial.print(On_Off); Serial.print(", Power: "); Serial.print(IRCar_Power_OK); Serial.print(", Timer: "); Serial.println(Menu_BackLightTimeON);
	if ((On_Off) && (IRCar_Power_OK))
	{
		if ((Menu_BackLightTimeON == 0))
		{
			Menu_LCD.backlight();
		}
		
		// Update Global Time
	  IRCar_TimeMs = millis();
		
	  // Set Timer
		Menu_BackLightTimeON = IRCar_TimeMs;
		// no null Timer => reserved for stopped state
		if (!Menu_BackLightTimeON) Menu_BackLightTimeON = 1;
	}
	else
	{
		Menu_BackLightTimeON = 0;
		Menu_LCD.noBacklight();
	}
}

// Functions
static Button_t IRCar_Menu_ReadButtons(bool WaitStableInput)
{
#if !defined(__AVR_ATmega2560__)
	// Waiting for a stable reading
	U16 AnalogKey = 0;
	do
	{
		S16 val1 = analogRead(IO_ANA_KEY);
		delay(1);
		S16 val2 = analogRead(IO_ANA_KEY);
		if (abs(val1 - val2) <= 5)
		{
			AnalogKey = val1;
			break;
		}
	}
	while (WaitStableInput);

	// Decod Analog Button
	if (AnalogKey > 100)
	{
		// Serial.println(F("Menu Buttons: ")); Serial.println(AnalogKey);
		if (AnalogKey < 186+50) // 184
		{
			// Serial.println(F("BUTTON_1"));
			return BUTTON_1;
		}
		if (AnalogKey < 355+50) // 354
		{
			// Serial.println(F("BUTTON_3"));
			return BUTTON_3;
		}
		if (AnalogKey < 508+50) // 508
		{
			// Serial.println(F("BUTTON_4"));
			return BUTTON_4;
		}
		if (AnalogKey < 680+50) // 680
		{
			// Serial.println(F("BUTTON_2"));
			return BUTTON_2;
		}
	}
#endif // !defined(__AVR_ATmega2560__)
	return BUTTON_NONE;
}
static Button_t IRCar_Menu_GetButtons()
{
	// Get stabilized buttons
	Button_t Buttons = IRCar_Menu_ReadButtons(true);

	// Re-arm BackLight ?
	if (BUTTON_NONE != Buttons)
	{
  	// Restart Backlight
  	const bool backlight_was_off = !Menu_BackLightTimeON;  	
		IRCar_Menu_BackLight(true);
		if (backlight_was_off && Menu_BackLightTimeON)
		{
  		// Cancel Press if the Backlight was off
			Buttons = BUTTON_NONE;
      while(BUTTON_NONE != IRCar_Menu_ReadButtons(true));
		}
	}
	return Buttons;
}

static Button_t IRCar_Menu_WaitNoButtons()
{
  // Get stabilized buttons
	while (BUTTON_NONE != IRCar_Menu_ReadButtons(true))
	{
	  // Restart Backlight
  	IRCar_Menu_BackLight(true);
  }
}

// Result Menu
static void Print_Time(U32 Time)
{
	/*
	Menu_LCD.print(Time/1000);
	Menu_LCD.print(F("."));
	Menu_LCD.print(Time%1000);
	Menu_LCD.print(F("s"));*/

	/* Convert into minutes / second / millis */
	U8 mn = 0;
	while(Time >= 60*1000UL)
	{
		Time -= 60*1000UL;
		++mn;
	}
	U8 sc = 0;
	while(Time >= 1000UL)
	{
		Time -= 1000UL;
		++sc;
	}
	U16 ms = Time;

	/* Print */
	char Dec3[4];
	PutDecNumber2(Dec3, mn); Dec3[2] = '\0'; Menu_LCD.print(Dec3); Menu_LCD.print(F(":"));
	PutDecNumber2(Dec3, sc); Dec3[2] = '\0'; Menu_LCD.print(Dec3); Menu_LCD.print(F("."));
	PutDecNumber3(Dec3, ms); Dec3[3] = '\0'; Menu_LCD.print(Dec3);
}

static void Print_CrossingSpeed(U32 CrossingTime)
{
	if (CrossingTime > 0)
	{
		// 1440 : 50Km @ 28.8ms x 10
		CrossingTime = (14400UL + CrossingTime/2) / CrossingTime;
		if (CrossingTime <= 9999)
		{
			Menu_LCD.print(CrossingTime/10);
			Menu_LCD.print(F("."));
			Menu_LCD.print(CrossingTime%10);
			Menu_LCD.print(F("Km/H"));
		}
	}
}

static void Print_LapSpeed(U32 LapTime, U16 RaceLenght)
{
  if (LapTime > 0)
  {
    // in Km/H x 10
    LapTime = ((U32)RaceLenght*360UL + (LapTime/2)) / LapTime;
    if (LapTime <= 9999)
    {
      Menu_LCD.print(LapTime/10);
			Menu_LCD.print(F("."));
			Menu_LCD.print(LapTime%10);
			Menu_LCD.print(F("Km/H"));
		}
	}
}

static void Print_Name7(U8 ID)
{
	switch(ID)
	{
		case 0:  Menu_LCD.print(F("Alex   ")); break;
		case 2:  Menu_LCD.print(F("Jacke  ")); break;
		case 3:  Menu_LCD.print(F("Inter44")); break;
    case 4:  Menu_LCD.print(F("Jean-M.")); break;
    case 5:  Menu_LCD.print(F("Etienne")); break;		
		case 6:  Menu_LCD.print(F("Lalou  ")); break;
    case 7:  Menu_LCD.print(F("Killian")); break;
		case 8:  Menu_LCD.print(F("Simon  ")); break;
		case 9:  Menu_LCD.print(F("Romain ")); break;
		case 10: Menu_LCD.print(F("Xavier ")); break;
    case 13: Menu_LCD.print(F("TAS    ")); break;
    case 14: Menu_LCD.print(F("Dimitri")); break;
    case 17: Menu_LCD.print(F("Hugo   ")); break;
		case 20: Menu_LCD.print(F("Gregory")); break;
		case 21: Menu_LCD.print(F("Alexis ")); break;
		case 23: Menu_LCD.print(F("Dimitri")); break;
		case 24: Menu_LCD.print(F("Dimitri")); break;
		case 25: Menu_LCD.print(F("Dimitri")); break;
		case 27: Menu_LCD.print(F("Xavier ")); break;
		default: Menu_LCD.print(F("       ")); break; // guest 
	}
}

static void IRCar_Menu_Result_Car(U8 ID, U8 TopPos, bool add_result)
{
  if ((ID < TAG_ID_MAX) && (IRCars[ID].lap_count > 0))
	{
    // Calculation
  	const U32 Avr_LapTime =
  		  (IRCars[ID].race_reference_time + (IRCars[ID].lap_count/2)) / IRCars[ID].lap_count;
    const U32 Avr_CrossTime =
        (IRCars[ID].total_crossing_time + (IRCars[ID].lap_count/2)) / IRCars[ID].lap_count;

    // Print
		Menu_LCD.clear();
		Menu_LCD.print(F("Pos: ")); Menu_LCD.print(TopPos); Menu_LCD.print(F(" \"")); Menu_LCD.print(ID); Menu_LCD.print(F(" ")); Print_Name7(ID); Menu_LCD.print(F("\""));
		if (!add_result)
		{
			Menu_LCD.setCursor(0,1);
			Menu_LCD.print(F("Race Lap: ")); Menu_LCD.print(IRCars[ID].lap_count); if (IRRace.lap_max > 0) { Menu_LCD.print(F(" / ")); Menu_LCD.print(IRRace.lap_max); }
			Menu_LCD.setCursor(0,2);
			Menu_LCD.print(F("Race Tim: ")); Print_Time(IRCars[ID].race_reference_time);
			Menu_LCD.setCursor(0,3);
			Menu_LCD.print(F("Avr. Tim: ")); Print_Time(Avr_LapTime);
		}
		else
		{
			Menu_LCD.setCursor(0,1);
			if ((IRSave.RaceLenght >= IR_CAR_MENU_RACE_LENGHT_MIN) && 
			    (IRSave.RaceLenght <= IR_CAR_MENU_RACE_LENGHT_MAX)  )
			{
  			// compute the average speed from the total time & race lenght
        Menu_LCD.print(F("Avr. Spd: ")); 
        Print_LapSpeed(Avr_LapTime, IRSave.RaceLenght);
      }
      else
      {
        // no RaceLenght => print average Speed "from sensor"
        Menu_LCD.print(F("Avr. Xpd: "));
  			Print_CrossingSpeed(Avr_CrossTime);
      }			
			Menu_LCD.setCursor(0,2);
			if ((IRSave.RaceLenght >= IR_CAR_MENU_RACE_LENGHT_MIN) && 
			    (IRSave.RaceLenght <= IR_CAR_MENU_RACE_LENGHT_MAX)  )
			{
  			// compute the best speed from the best time & race lenght
  			Menu_LCD.print(F("Best Spd: "));
  			Print_LapSpeed(IRCars[ID].best_lap_time, IRSave.RaceLenght);
			}
			else
			{
  			// no RaceLenght => print best Speed "from sensor"
			  Menu_LCD.print(F("Best Xpd: "));
  			Print_CrossingSpeed(IRCars[ID].best_crossing_time);
		  }
			Menu_LCD.setCursor(0,3);
			Menu_LCD.print(F("Best Tim: ")); Print_Time(IRCars[ID].best_lap_time);
		}
	}
	else
	{
		// Invalid
		IRCar_Signal_PlayBip();
		//Serial.println(F("Can't display car results !"));
	}
	
	// synchro
	IRCar_Menu_WaitNoButtons();
}

static U8 IRCar_Nagivate_Result(U8 ID, const S8 Pos)
{
	if (ID < TAG_ID_MAX)
	{
		// Compute Initial Position
		U8 InitialID = ID;
		U8 InitalTopPos;
		do
		{
			InitalTopPos = IRCar_Race_GetTopPos(ID);
			if (InitalTopPos > 0) break;

			// Find a valid Position
			++ID; if (ID >= TAG_ID_MAX) ID = 0;
		} while(ID != InitialID);

		// Navigate
		if(InitalTopPos > 0)
		{
			InitialID = ID;
			if (Pos > 0)
			{
				U8 CurrTopPos;
				//Serial.print(">Next ID: "); Serial.print(InitialID); Serial.print(", pos: "); Serial.println(InitalTopPos);
				do
				{
					++ID; if (ID >= TAG_ID_MAX) ID = 0;
					CurrTopPos = IRCar_Race_GetTopPos(ID);
				}
				while ((ID != InitialID) && (!CurrTopPos || CurrTopPos != InitalTopPos+1));
				//Serial.print("<Next ID:"); Serial.print(ID); Serial.print(", pos: "); Serial.println(CurrTopPos);
			}
			if (Pos < 0)
			{
				U8 CurrTopPos;
				//Serial.print(">Prev ID: "); Serial.print(InitialID); Serial.print(", pos: "); Serial.println(InitalTopPos);
				do
				{
					++ID; if (ID >= TAG_ID_MAX) ID = 0;
					CurrTopPos = IRCar_Race_GetTopPos(ID);
				}
				while ((ID != InitialID) && (!CurrTopPos || CurrTopPos+1 != InitalTopPos));
				//Serial.print("<Prev ID:"); Serial.print(ID); Serial.print(", pos: "); Serial.println(CurrTopPos);
			}
			return ID;
		}
	}
	return 0;
}

static void IRCar_Menu_Result()
{
	// Init
	Menu_LCD.clear();
	Menu_LCD.print(F("<<  LAST RESULTS  >>"));
	Menu_LCD.setCursor(0,1);
	Menu_LCD.print(F(">1:   Exit Result"));
	Menu_LCD.setCursor(0,2);
	Menu_LCD.print(F(">2/3: <..> Result"));
	Menu_LCD.setCursor(0,3);
	Menu_LCD.print(F(">4:   Info Result"));

	// Wait Button
	{
		Button_t Button;
		while((Button = IRCar_Menu_GetButtons()) == BUTTON_NONE);
		if (Button == BUTTON_1) return;
		
		// Update Global Time
	  IRCar_TimeMs = millis();

		// BackLight Time Out ?
		if (Menu_BackLightTimeON && ((IRCar_TimeMs - Menu_BackLightTimeON) >= IR_CAR_MENU_BACKLIGHT_ON))
		{
			IRCar_Menu_BackLight(false);
		}
	}

	// First Draw
	U8 ID     = IRCar_Nagivate_Result(IRRace.leaded_car, 0);
	U8 TopPos = IRCar_Race_GetTopPos(ID);
	bool MoreInfo  = false;
	IRCar_Menu_Result_Car(ID, TopPos, MoreInfo);

	// Loop
	for(;;)
	{
		// Update Global Time
	  IRCar_TimeMs = millis();

		// BackLight Time Out ?
		if (Menu_BackLightTimeON && ((IRCar_TimeMs - Menu_BackLightTimeON) >= IR_CAR_MENU_BACKLIGHT_ON))
		{
			IRCar_Menu_BackLight(false);
		}

		// Get Button & Escape
		switch(IRCar_Menu_GetButtons())
		{
			// Escape
			case BUTTON_1:
				return;

			// Navigate --
			case BUTTON_2:
			{
				// Look for the prev valid Car info
				ID = IRCar_Nagivate_Result(ID, -1);
				TopPos = IRCar_Race_GetTopPos(ID);

				// Print
				//MoreInfo = false;
				IRCar_Menu_Result_Car(ID, TopPos, MoreInfo);
			}
			break;

			// Navigate ++
			case BUTTON_3:
			{
				// Look for the next valid Car info
				ID = IRCar_Nagivate_Result(ID, +1);
				TopPos = IRCar_Race_GetTopPos(ID);

				// Print
				//MoreInfo = false;
				IRCar_Menu_Result_Car(ID, TopPos, MoreInfo);
			}
			break;

			// Add Info
			case BUTTON_4:
			{
				// Print
				MoreInfo ^= true;
				IRCar_Menu_Result_Car(ID, TopPos, MoreInfo);
			}
			break;

			default:
				break;
		}

		// Wait Tempo
		delay(250);
	}
}

// Race
static bool IRCar_Menu_Race_Conf(U8 * pLapLimit, U8 * pTimeLimit)
{
	// Init
	Menu_LCD.clear();
	Menu_LCD.print(F("<< NEW RACE CONF. >>"));
	Menu_LCD.setCursor(0,1);
	Menu_LCD.print(F(">1: Exit   >4: Valid"));
	Menu_LCD.setCursor(0,2);
	Menu_LCD.print(F(">2: Tim Limit:"));
	Menu_LCD.setCursor(0,3);
	Menu_LCD.print(F(">3: Lap Limit:"));
	
	// synchro
	IRCar_Menu_WaitNoButtons();

	// Initial Values
	U8 LapLimit  = *pLapLimit;
	U8 TimeLimit = *pTimeLimit;
	if (TimeLimit > IR_CAR_MENU_RACE_MAX_TIME) TimeLimit = 0;
	if (LapLimit  > IR_CAR_MENU_RACE_MAX_LAP)  LapLimit  = 0;	
	
	// Loop	
	bool Break   = false;
	bool ReDraw  = true;	
	while(!Break)
	{
		// Update Global Time
	  IRCar_TimeMs = millis();

		// BackLight Time Out ?
		if (Menu_BackLightTimeON && ((IRCar_TimeMs - Menu_BackLightTimeON) >= IR_CAR_MENU_BACKLIGHT_ON))
		{
			IRCar_Menu_BackLight(false);
		}

		// Get Button & Escape
		switch(IRCar_Menu_GetButtons())
		{
			// Escape
			case BUTTON_1:
				return false;

			// TimeLimit
			case BUTTON_2:
			{
				++TimeLimit;
				if (TimeLimit > IR_CAR_MENU_RACE_MAX_TIME) TimeLimit = 0;
				// Redraw
				ReDraw  = true;
			}
			break;

			// LapLimit
			case BUTTON_3:
			{
				++LapLimit;
				if (LapLimit > IR_CAR_MENU_RACE_MAX_LAP) LapLimit = 0;
				// Redraw
				ReDraw  = true;
			}
			break;

			// Valid
			case BUTTON_4:
				Break = true;
				break;

			default:
				break;
		}

		// ReDraw
		if (ReDraw)
		{
			ReDraw = false;
			
			// Time Limit
			Menu_LCD.setCursor(15,2);
			if (TimeLimit > 0)
			{
			  Menu_LCD.print(TimeLimit); Menu_LCD.print(F(" mn")); if (TimeLimit < 10) Menu_LCD.print(F(" "));
		  }
		  else
		  {
  		  Menu_LCD.print(F(" --- "));
		  }
  		 
		  // Lap limit 
			Menu_LCD.setCursor(15,3);
			if (LapLimit > 0)
			{
  			Menu_LCD.print(LapLimit);  Menu_LCD.print(F(" tr")); if (LapLimit < 10)  Menu_LCD.print(F(" "));
			}
			else
			{
  			Menu_LCD.print(F(" --- "));
		  }
		}

		// Wait Tempo
		delay(250);
	}

	// return config
	*pTimeLimit = TimeLimit;
	*pLapLimit  = LapLimit;
	return true;
}

// Set Race Lenght
bool IRCar_Menu_Race_Lenght(U16 * pRaceLenght)
{
  // Init
	Menu_LCD.clear();
	Menu_LCD.print(F("<<SET RACE  LENGHT>>"));
	Menu_LCD.setCursor(0,1);
	Menu_LCD.print(F(">1: Exit   >4: Valid"));
	Menu_LCD.setCursor(0,2);
  Menu_LCD.print(F(">2: Sub    >3: Add"));
	Menu_LCD.setCursor(0,3);
	Menu_LCD.print(F("Lenght: "));
	
	// synchro
	IRCar_Menu_WaitNoButtons();
	
	// Initial Values
	U16 RaceLenght  = *pRaceLenght;
  if (RaceLenght < IR_CAR_MENU_RACE_LENGHT_MIN ||
      RaceLenght > IR_CAR_MENU_RACE_LENGHT_MAX  )
  {
    RaceLenght = 0;
  }
      
	// Loop	
	bool Break   = false;
	bool ReDraw  = true;	
	U16  HighSpeed = 0;
	while(!Break)
	{
		// Update Global Time
	  IRCar_TimeMs = millis();

		// BackLight Time Out ?
		if (Menu_BackLightTimeON && ((IRCar_TimeMs - Menu_BackLightTimeON) >= IR_CAR_MENU_BACKLIGHT_ON))
		{
			IRCar_Menu_BackLight(false);
		}

		// Get Button & Escape
		switch(IRCar_Menu_GetButtons())
		{
			// Escape
			case BUTTON_1:
				return false;

			// TimeLimit
			case BUTTON_2:
			{
  			U16 Inc = (HighSpeed++ > 10) ? IR_CAR_MENU_RACE_LENGHT_INC_HIH : IR_CAR_MENU_RACE_LENGHT_INC_LOW;
  			if (RaceLenght > IR_CAR_MENU_RACE_LENGHT_MAX)
				{
  				RaceLenght = IR_CAR_MENU_RACE_LENGHT_MAX;
				}
				else if (RaceLenght < (IR_CAR_MENU_RACE_LENGHT_MIN + Inc))
  			{
    			RaceLenght = IR_CAR_MENU_RACE_LENGHT_MIN;
  			}
  			else
  			{
    			RaceLenght -= Inc;
  			}
				// Redraw
				ReDraw  = true;
			}
			break;

			// LapLimit
			case BUTTON_3:
			{
				U16 Inc = (HighSpeed++ > 10) ? IR_CAR_MENU_RACE_LENGHT_INC_HIH : IR_CAR_MENU_RACE_LENGHT_INC_LOW;
				if (RaceLenght < IR_CAR_MENU_RACE_LENGHT_MIN)
				{
  				RaceLenght = IR_CAR_MENU_RACE_LENGHT_MIN;
				}
				else if ((RaceLenght + Inc) > IR_CAR_MENU_RACE_LENGHT_MAX)
  			{
    			RaceLenght = IR_CAR_MENU_RACE_LENGHT_MAX;
  			}
  			else
  			{
    			RaceLenght += Inc;
  			}
				// Redraw
				ReDraw  = true;
			}
			break;

			// Valid
			case BUTTON_4:
				Break = true;
				break;

			default:
			  HighSpeed = 0;
				break;
		}

		// ReDraw
		if (ReDraw)
		{
			ReDraw = false;

			// Draw Limit
			Menu_LCD.setCursor(8,3);
			if (RaceLenght > 0)
			{
			  char Dec3[4];
			  PutDecNumber3(Dec3, RaceLenght/100); Dec3[3] = '\0'; Menu_LCD.print(Dec3);
			  Menu_LCD.print(F("."));	
			  PutDecNumber2(Dec3, RaceLenght%100); Dec3[2] = '\0'; Menu_LCD.print(Dec3);
			  Menu_LCD.print(F(" m"));
		  }
		  else
		  {
  		  Menu_LCD.print(F("--------"));
		  }
		}

		// Wait Tempo
		if (HighSpeed > 10)
		{
		  delay(100);
	  }
	  else
	  {
  	  delay(250);
	  }
	}

	// return config
	*pRaceLenght = RaceLenght;
	return true;
}

// Draw Menu
static void IRCar_Menu_Enter(Menu_t Menu)
{
  // Update Global Time
  IRCar_TimeMs = millis();
  
	// Avoid refresh unchanged Menu
	if (Menu == Menu_Current)
	{
		// Only refresh Power Level in IDLE Screen
		if (MENU_IDLE == Menu_Current)
		{
  		// Battery Power Level 
  		{
			  char PowerLevel[3];
			  PutDecNumber2(PowerLevel, IRCar_Battery_Voltage());	PowerLevel[2] = '\0';
			  Menu_LCD.setCursor(17,0);
			  Menu_LCD.print(PowerLevel); Menu_LCD.print(F("%"));
		  }
		  
		  // AutoTest Last ID
		  if (IRCar_AutoTest_LastID < TAG_ID_MAX)
  		{
    		char LastDetectedID[3];
			  PutDecNumber2(LastDetectedID, IRCar_AutoTest_LastID);	LastDetectedID[2] = '\0';
			  Menu_LCD.setCursor(17,3);
			  Menu_LCD.print(F("#")); Menu_LCD.print(LastDetectedID); 
		  }

			// Set TimeOut for Refresh
			Menu_TimeOut = IRCar_TimeMs;
			// no null TimeOut => reserved for stopped state
			if (!Menu_TimeOut) Menu_TimeOut = 1;
			// Serial.println(F("New Menu: ")); Serial.println(Menu);
		}
		return;
	}

	// New Menu
	Menu_Current = Menu;
	Menu_TimeOut = IRCar_TimeMs;
	// no null TimeOut => reserved for stopped state
	if (!Menu_TimeOut) Menu_TimeOut = 1;
	// Serial.println(F("New Menu: ")); Serial.println(Menu);

	// Re-arm Backlight
	IRCar_Menu_BackLight(true);

	// Draw
	switch(Menu)
	{
		case MENU_INIT:
		{
			// Show Version
			Menu_LCD.clear();
			Menu_LCD.setCursor(0,0);
			Menu_LCD.print(F(PROJET_NAME));
			//Menu_LCD.setCursor(0,1);
			//Menu_LCD.print(F(PROJET_AUTHOR));
			Menu_LCD.setCursor(0,2);
			Menu_LCD.print(F(PROJET_VERSION));
			Menu_LCD.setCursor(0,3);
			Menu_LCD.print(F(PROJET_AUTHOR));
			// Play Bip
			IRCar_Signal_PlayBip();
		}
		break;

		case MENU_IDLE:
		{
			Menu_LCD.clear();
			Menu_LCD.setCursor(0,0);
			Menu_LCD.print(F("IR-LAP COUNTER *"));
      Menu_LCD.setCursor(0,1);
			Menu_LCD.print(F(">1: Set Race Lenght"));
      Menu_LCD.setCursor(0,2);
			Menu_LCD.print(F(">2/3: Quick/New RACE"));
      Menu_LCD.setCursor(0,3);
			Menu_LCD.print(F(">4: Last Results"));

			// Power Level
			char PowerLevel[3];
			PutDecNumber2(PowerLevel, IRCar_Battery_Voltage());	PowerLevel[2] = '\0';
			Menu_LCD.setCursor(17,0);
			Menu_LCD.print(PowerLevel); Menu_LCD.print(F("%"));
		}
		break;

		case MENU_RACE_STARTING:
		{
			Menu_LCD.clear();
			Menu_LCD.print(F("<< NEW RACE START >>"));

      // Time Limit
			Menu_LCD.setCursor(0,2);
      Menu_LCD.print(F("> Tim Limit:"));
      Menu_LCD.setCursor(13,2);
			if (IRSave.MaxTim > 0)
			{
			  Menu_LCD.print(IRSave.MaxTim); Menu_LCD.print(F(" mn")); if (IRSave.MaxTim < 10) Menu_LCD.print(F(" "));
		  }
		  else
		  {
  		  Menu_LCD.print(F(" --- "));
		  }
  		
      // Lap limit       
      Menu_LCD.setCursor(0,3);
      Menu_LCD.print(F("> Lap Limit:"));
		  Menu_LCD.setCursor(13,3);
			if (IRSave.MaxLap > 0)
			{
  			Menu_LCD.print(IRSave.MaxLap);  Menu_LCD.print(F(" tr")); if (IRSave.MaxLap < 10)  Menu_LCD.print(F(" "));
			}
			else
			{
  			Menu_LCD.print(F(" --- "));
		  }
		}
		break;

		case MENU_RACE_CHECKING:
		{
			// Show NewRaw 2/2
			Menu_LCD.setCursor(0,1);
      Menu_LCD.print(F("  ** Race Check **  "));
		}
		break;

		case MENU_RACE_RUNNING:
		{
			// Show NewRaw 2/2
			Menu_LCD.setCursor(0,1);
      Menu_LCD.print(F("                    "));
		}
		break;

		case MENU_RACE_FINISHED:
		{
			// Show NewRaw 2/2
			Menu_LCD.setCursor(0,1);
      Menu_LCD.print(F("  ** Race  Over **  "));
		}
		break;

		case MENU_RACE_END:
		{
			Menu_LCD.clear();
			Menu_LCD.print(F("<<    RACE END    >>"));
			Menu_LCD.setCursor(0,2);
			Menu_LCD.print(F("Winner Car : ")); Menu_LCD.print(IRRace.leaded_car);
			Menu_LCD.setCursor(0,3);
			Menu_LCD.print(F("Winner Name: ")); Print_Name7(IRRace.leaded_car);
		}
		break;

		default:
		{
			Menu_LCD.clear();
			Menu_Current = MENU_NONE;
			Menu_TimeOut = 0;
		}
		break;
	}
	
	// synchro
	IRCar_Menu_WaitNoButtons();
}

// Reset
void IRCar_Menu_Reset(Reset_t Reset)
{
	if (RESET_INIT == Reset)
	{
		// Initialize the lcd for 20 chars 4 lines and turn on backlight
		Menu_LCD.begin(20,4);

		// Test backlight
		Menu_LCD.backlight();
		delay(1000);
		Menu_LCD.noBacklight();
		delay(1000);

		// Set backlight
		Menu_BackLightTimeON = 0;
		IRCar_Menu_BackLight(true);
		
		// Init
		Menu_NoMoreInit = false;
	}

	if (RESET_NONE != Reset)
	{
		// Enter First Menu
		IRCar_Menu_Enter(MENU_INIT);
	}
}

// Update
void IRCar_Menu_Update(void)
{
	// Update on Status
	switch(IRRace.lap_status)
	{
		case IR_CAR_LAP_STATUS_START_ENGINE:
		{
			IRCar_Menu_Enter(MENU_RACE_STARTING);
		}
		break;

		case IR_CAR_LAP_STATUS_START_CHECK:
		{
			IRCar_Menu_Enter(MENU_RACE_CHECKING);
		}
		break;

		case IR_CAR_LAP_STATUS_START_1:
		{
			IRCar_Menu_Enter(MENU_RACE_RUNNING);
		}
		break;

		case IR_CAR_LAP_STATUS_FINISHED:
		{
			IRCar_Menu_Enter(MENU_RACE_FINISHED);
		}
		break;

		case IR_CAR_LAP_STATUS_NONE:
		{

		}
		break;

		default:
			break;
	}
}

// Process
void IRCar_Menu_Process(bool AllowNewCommand /* = false */)
{
	// Power Issue ?
	if (!IRCar_Power_OK)
	{
		IRCar_Menu_BackLight(false);
	}

	// BackLight Time Out ?
	if (Menu_BackLightTimeON && ((IRCar_TimeMs - Menu_BackLightTimeON) >= IR_CAR_MENU_BACKLIGHT_ON))
	{
		IRCar_Menu_BackLight(false);
	}

	// All Menu ?
	if (IR_CAR_LAP_STATUS_NONE == IRRace.lap_status)
	{
		// TimeOut ?
		if (Menu_TimeOut && ((IRCar_TimeMs - Menu_TimeOut) >= IR_CAR_MENU_TIME_OUT))
		{
			IRCar_Menu_Enter(MENU_IDLE);
		}

		// Get Button ?
		switch(Menu_Current)
		{
			case MENU_INIT:
			{
				if (Menu_NoMoreInit)
				{
  				IRCar_Menu_Enter(MENU_IDLE);
		    }
		    // else: wait timeout to go IDLE
			}
			break;

			case MENU_IDLE:
			{
  			Menu_NoMoreInit = true;  			
				if (AllowNewCommand)
				{
					switch(IRCar_Menu_GetButtons())
					{
						case BUTTON_1:
						{
  						// Set Race Lenght
							IRCar_Menu_Race_Lenght(&IRSave.RaceLenght);

							// Return Idle Menu
							IRCar_Menu_Enter(MENU_INIT);
						}
						break;

						case BUTTON_2:
						{
  						// Start a Quick Race
              IRSave.MaxLap = 5;
              IRSave.MaxTim = 0;
							IRCar_Race_Start(IRSave.MaxLap, (U32)IRSave.MaxTim*60UL*1000UL, 0/*DebugMode*/);
						}
						break;

						case BUTTON_3:
						{
							// Start a New Race
							if (IRCar_Menu_Race_Conf(&IRSave.MaxLap, &IRSave.MaxTim))
							{
								IRCar_Race_Start(IRSave.MaxLap, (U32)IRSave.MaxTim*60UL*1000UL, 0/*DebugMode*/);
							}
							else
							{
								// Return Init Menu
								IRCar_Menu_Enter(MENU_INIT);
							}
						}
						break;

						case BUTTON_4:
						{
							// Result Print
							IRCar_Menu_Result();
							// Return Init Menu
							IRCar_Menu_Enter(MENU_INIT);
						}
						break;

						default:
							break;
					}
				}
			}
			break;

			default:
			{
				IRCar_Menu_Enter(MENU_INIT);
			}
			break;
		}
	}
	else if (IR_CAR_LAP_STATUS_FINISHED == IRRace.lap_status)
	{
		// TimeOut ?
		if (Menu_TimeOut && ((IRCar_TimeMs - Menu_TimeOut) >= IR_CAR_MENU_TIME_OUT))
		{
			IRCar_Menu_Enter(MENU_RACE_END);
		}
		// Detect reset race
		if (BUTTON_1 == IRCar_Menu_GetButtons())
		{
			// Reset Race
			IRCar_Race_Reset();
		}
	}
	else
	{
		// Detect aborted race
		if (BUTTON_1 == IRCar_Menu_ReadButtons(false /* direct reading */) &&
				BUTTON_1 == IRCar_Menu_ReadButtons(false /* direct reading */) &&
				BUTTON_1 == IRCar_Menu_ReadButtons(false /* direct reading */)  )
		{
			// Stop Race
			IRCar_Race_Reset();
		}
	}
}