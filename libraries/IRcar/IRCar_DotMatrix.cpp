/*
 * Infrared Lap Counter for Radio-Controlled Cars
 * Copyright 2014-2015 Team RCFree44 
 */

#include "IRCarInt.h"
#include <avr/pgmspace.h>

// Timer
static U16 DotMatrix_TimerON;
static U16 DotMatrix_Timer_Duration;
static U16 DotMatrix_Flash_Timer;
static U8  DotMatrix_Flash_Digit;
static bool DotMatrix_Flash_State;

// Bitmap
#define FONT_DIGIT_NUMBER 11
#define FONT_DIGIT_RESET  0xFF
static const uint8_t Font_Digit[FONT_DIGIT_NUMBER][8] PROGMEM  =
{
	// 0
	{
		B00111100,
		B01100110,
		B01100110,
		B01100110,
		B01100110,
		B01100110,
		B01100110,
		B00111100,
	},
	// 1
	{
		B00001100,
		B00011100,
		B00111100,
		B01101100,
		B00001100,
		B00001100,
		B00001100,
		B00001100,
	},
	// 2
	{
		B00111000,
		B01101100,
		B00000110,
		B00000110,
		B00001100,
		B00110000,
		B01100000,
		B01111110,
	},
	// 3
	{
		B00111000,
		B01101100,
		B00000110,
		B00001100,
		B00000110,
		B00000110,
		B01100110,
		B00111100,
	},
	// 4
	{
		B00001100,
		B00011100,
		B00101100,
		B00101100,
		B01001100,
		B01111110,
		B00001100,
		B00001100,
	},
	// 5
	{
		B00111110,
		B00110000,
		B01100000,
		B01111100,
		B00000110,
		B00000110,
		B01100110,
		B00111100,
	},
	// 6
	{
		B00111100,
		B01100110,
		B01100000,
		B01111100,
		B01100110,
		B01100110,
		B01100110,
		B00111100,
	},
	// 7
	{
		B01111110,
		B00000110,
		B00001100,
		B00001100,
		B00011000,
		B00011000,
		B00011000,
		B00011000,
	},
	// 8
	{
		B00111100,
		B01100110,
		B01100110,
		B00111100,
		B01100110,
		B01100110,
		B01100110,
		B00111100,
	},
	// 9
	{
		B00111100,
		B01100110,
		B01100110,
		B01100110,
		B00111110,
		B00000110,
		B01100110,
		B00111100,
	},
	// A
	{
		B01111110,
		B11111111,
		B11111111,
		B11111111,
		B11111111,
		B11111111,
		B11111111,
		B01111110,
	},
};

// define max7219 registers
#define max7219_reg_noop        0x00
#define max7219_reg_digit0      0x01
#define max7219_reg_digit1      0x02
#define max7219_reg_digit2      0x03
#define max7219_reg_digit3      0x04
#define max7219_reg_digit4      0x05
#define max7219_reg_digit5      0x06
#define max7219_reg_digit6      0x07
#define max7219_reg_digit7      0x08
#define max7219_reg_decodeMode  0x09
#define max7219_reg_intensity   0x0a
#define max7219_reg_scanLimit   0x0b
#define max7219_reg_shutdown    0x0c
#define max7219_reg_displayTest 0x0f

// define max in use
#define maxInUse 1    //change this variable to set how many MAX7219's you'll use

// funtions
static void putByte(byte data)
{
	byte i = 8;
	byte mask;
	while(i > 0)
	{
		mask = 0x01 << (i - 1);      // get bitmask
		digitalWrite(IO_OUT_SPI_CLK, LOW);   // tick
		// choose bit
		if (data & mask)
		{
			digitalWrite(IO_OUT_SPI_DATA, HIGH);// send 1
		}
		else
		{
			digitalWrite(IO_OUT_SPI_DATA, LOW); // send 0
		}
		digitalWrite(IO_OUT_SPI_CLK, HIGH);   // tock
		--i;                         // move to lesser bit
	}
}

//maxSingle is the "easy"  function to use for a single max7219
static void maxSingle( byte reg, byte col)
{
	digitalWrite(IO_OUT_SPI_LOAD, LOW);       // begin
	putByte(reg);                  // specify register
	putByte(col);//((data & 0x01) * 256) + data >> 1); // put data
	digitalWrite(IO_OUT_SPI_LOAD, LOW);       // and load da shit
	digitalWrite(IO_OUT_SPI_LOAD, HIGH);
}

// initialize  all  MAX7219's in the system
static void maxAll (byte reg, byte col)
{
	digitalWrite(IO_OUT_SPI_LOAD, LOW);  // begin
	for (U8 c=0; c< maxInUse; ++c)
	{
		putByte(reg);  // specify register
		putByte(col);//((data & 0x01) * 256) + data >> 1); // put data
	}
	digitalWrite(IO_OUT_SPI_LOAD, LOW);
	digitalWrite(IO_OUT_SPI_LOAD, HIGH);
}

// maxOne is for adressing different MAX7219's,
// whilele having a couple of them cascaded
#if maxInUse > 1
static void maxOne(byte maxNr, byte reg, byte col)
{
	digitalWrite(IO_OUT_SPI_LOAD, LOW);  // begin

	for (U8 c = maxInUse; c > maxNr; c--)
	{
		putByte(0);    // means no operation
		putByte(0);    // means no operation
	}

	putByte(reg);  // specify register
	putByte(col);//((data & 0x01) * 256) + data >> 1); // put data

	for (U8 c = maxNr-1; c >= 1; c--)
	{
		putByte(0);    // means no operation
		putByte(0);    // means no operation
	}

	digitalWrite(IO_OUT_SPI_LOAD, LOW); // and load da shit
	digitalWrite(IO_OUT_SPI_LOAD, HIGH);
}
#endif

//
static void maxSingle(const uint8_t *p)
{
	if (p != NULL)
	{
		//Serial.println("DotMatrix ON");
		for(U8 i=1; i<=8; ++i)
		{
			maxSingle(i, pgm_read_byte(p++));
		}
	}
	else
	{
		//Serial.println("DotMatrix OFF");
		for(U8 i=1; i<=8; ++i)
		{
			maxSingle(i, 0);
		}
	}
}

// Cmd
static void DotMatrix_Cmd(U8 Digit = FONT_DIGIT_RESET, U16 SetTimer = IR_CAR_DOTMATRIX_AUTO_STOP, bool SetFlash = false)
{
	// Timer Reset
	DotMatrix_TimerON     = 0;
	DotMatrix_Flash_Timer = 0;

	// Set ?
	if ((Digit < FONT_DIGIT_NUMBER) && (IRCar_Power_OK) && (IRCar_Battery_LipoDetected))
	{
		maxSingle(Font_Digit[Digit]);

		// Update Global Time
    IRCar_TimeMs = millis();
		
		// Set Timer (if asked)
		DotMatrix_Timer_Duration = SetTimer;
		if (SetTimer > 0)
		{
			DotMatrix_TimerON = IRCar_TimeMs;
			// no null Timer => reserved for stopped state
			if (!DotMatrix_TimerON) DotMatrix_TimerON = 1;
		}
		// Set Flash (is asker)
		if (SetFlash)
		{
			DotMatrix_Flash_Timer = IRCar_TimeMs;
			// no null Timer => reserved for stopped state
			if (!DotMatrix_Flash_Timer) DotMatrix_Flash_Timer = 1;
			// save current digit
			DotMatrix_Flash_Digit = Digit;
			DotMatrix_Flash_State = true;
		}
	}
	// Reset ?
	else
	{
		maxSingle(NULL);
	}
}

// Reset
void IRCar_DotMatrix_Reset(Reset_t Reset)
{
	if (RESET_INIT == Reset)
	{
		pinMode(IO_OUT_SPI_DATA, OUTPUT);
		pinMode(IO_OUT_SPI_LOAD, OUTPUT);
		pinMode(IO_OUT_SPI_CLK,  OUTPUT);

		//initiation of the max 7219
		maxAll(max7219_reg_scanLimit,   0x07);
		maxAll(max7219_reg_decodeMode,  0x00);  // using an led matrix (not digits)
		maxAll(max7219_reg_shutdown,    0x01);    // not in shutdown mode
		maxAll(max7219_reg_displayTest, 0x00); // no display test
		maxAll(max7219_reg_intensity,   0x0f & 0x0f); // the first 0x0f is the value you can set range: 0x00 to 0x0f

		// empty registers, turn all LEDs off
		for (U8 e=1; e<=8; e++)
		{
			maxAll(e, 0);
		}
	}

	if (RESET_NONE != Reset)
	{
		DotMatrix_Cmd();
	}
	else
	{
  	// Update Global Time
	  IRCar_TimeMs = millis();
  	
		// Restart TimeOut for 5 sec
		DotMatrix_Timer_Duration = 5*1000UL;
		DotMatrix_TimerON = IRCar_TimeMs;
		// no null Timer => reserved for stopped state
		if (!DotMatrix_TimerON) DotMatrix_TimerON = 1;
	}
}

// Update
void IRCar_DotMatrix_Update(void)
{
	// Power Issue ?
	if (!IRCar_Power_OK)
	{
		DotMatrix_Cmd();
		return;
	}

	// Update on Status
	switch(IRRace.lap_status)
	{
		case IR_CAR_LAP_STATUS_START_ENGINE:
		{
			DotMatrix_Cmd(10);
		}
		break;

		case IR_CAR_LAP_STATUS_START_CHECK:
		{
			DotMatrix_Cmd();
		}
		break;

		case IR_CAR_LAP_STATUS_START_3:
		{
			DotMatrix_Cmd(3);
		}
		break;

		case IR_CAR_LAP_STATUS_START_2:
		{
			DotMatrix_Cmd(2);
		}
		break;

		case IR_CAR_LAP_STATUS_START_1:
		{
			DotMatrix_Cmd(1);
		}
		break;

		case IR_CAR_LAP_STATUS_RUN:
		{
			DotMatrix_Cmd(0);
		}
		break;

		case IR_CAR_LAP_STATUS_LAST_LAP:
		{
			DotMatrix_Cmd(1, 0 /* no timer */);
		}
		break;

		case IR_CAR_LAP_STATUS_FINISHED:
		{
			DotMatrix_Cmd(10, 8*1000UL /* 8 sec */, true /* flash */);
		}
		break;

		default:
		{
			DotMatrix_Cmd();
		}
		break;
	}
}

// Race Update
void IRCar_DotMatrix_RaceUpdate(U8 Lap_Downcount)
{
	if (Lap_Downcount < 10)
	{
		DotMatrix_Cmd(Lap_Downcount, 0 /* no timer */);
	}
}

// Process
void IRCar_DotMatrix_Process(void)
{
	// Power Issue ?
	if (!IRCar_Power_OK)
	{
		DotMatrix_Cmd();
		return;
	}

	// Execute
	if (DotMatrix_TimerON && ((IRCar_TimeMs - DotMatrix_TimerON) >= DotMatrix_Timer_Duration))
	{
		DotMatrix_Cmd();
	}
	else if (DotMatrix_Flash_Timer && ((IRCar_TimeMs - DotMatrix_Flash_Timer) >= IR_CAR_DOTMATRIX_FLASH_PERIOD))
	{
		// re-arm flash timer
		DotMatrix_Flash_Timer = IRCar_TimeMs;
		if (!DotMatrix_Flash_Timer) DotMatrix_Flash_Timer = 1;

		// re-draw
		if (DotMatrix_Flash_State)
		{
			DotMatrix_Flash_State = false;
			maxSingle(NULL);
		}
		else
		{
			DotMatrix_Flash_State = true;
			if ((DotMatrix_Flash_Digit < FONT_DIGIT_NUMBER) && (IRCar_Power_OK) && (IRCar_Battery_LipoDetected))
			{
				maxSingle(Font_Digit[DotMatrix_Flash_Digit]);
			}
		}
	}
}