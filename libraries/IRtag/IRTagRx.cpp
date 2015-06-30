/*
 * Infrared Lap Counter for Radio-Controlled Cars
 * Copyright 2014-2015 Team RCFree44 
 */

#include "IRTagRx.h"

// Rx Instance
volatile RxIrChannel_t RxIrChannel_1;
volatile RxIrChannel_t RxIrChannel_2;
volatile RxIrChannel_t RxIrChannel_3;
volatile RxIrChannel_t RxIrChannel_4;

// initialization
void IRTagRecv::enableIRIn()
{
	// Serial.println("enableIRIn");

#ifdef RX_TAG_IT_LOAD_PIN
	pinMode(RX_TAG_IT_LOAD_PIN, OUTPUT);
#endif
#ifdef RX_TAG_CHANNEL1_DATABACK_PIN
	pinMode(RX_TAG_CHANNEL1_DATABACK_PIN, OUTPUT);
#endif
#ifdef RX_TAG_CHANNEL2_DATABACK_PIN
	pinMode(RX_TAG_CHANNEL2_DATABACK_PIN, OUTPUT);
#endif
#ifdef RX_TAG_CHANNEL3_DATABACK_PIN
	pinMode(RX_TAG_CHANNEL3_DATABACK_PIN, OUTPUT);
#endif
#ifdef RX_TAG_CHANNEL4_DATABACK_PIN
	pinMode(RX_TAG_CHANNEL4_DATABACK_PIN, OUTPUT);
#endif


	// disable interrupts
	cli();

	// setup pulse clock timer interrupt
	// Prescale /8 (16M/8 = 0.5 microseconds per tick)
	// Therefore, the timer interval can range from 0.5 to 128 microseconds
	// depending on the reset value (255 to 0)
	TIMER_CONFIG_NORMAL();

	//Timer2 Overflow Interrupt Enable
	TIMER_ENABLE_INTR;

	TIMER_RESET;

	// set pin modes
	// & initialize the reception
#ifdef RX_TAG_CHANNEL1_PIN
	pinMode(RX_TAG_CHANNEL1_PIN, INPUT_PULLUP);
	RxIrChannel_1_resume();
#endif
#ifdef RX_TAG_CHANNEL2_PIN
	pinMode(RX_TAG_CHANNEL2_PIN, INPUT_PULLUP);
	RxIrChannel_2_resume();
#endif
#ifdef RX_TAG_CHANNEL3_PIN
	pinMode(RX_TAG_CHANNEL3_PIN, INPUT_PULLUP);
	RxIrChannel_3_resume();
#endif
#ifdef RX_TAG_CHANNEL4_PIN
	pinMode(RX_TAG_CHANNEL4_PIN, INPUT_PULLUP);
	RxIrChannel_4_resume();
#endif

	// enable interrupts
	sei();
}

// TIMER2 interrupt code to collect raw data.
// Widths of alternating SPACE, MARK are recorded in rawbuf.
// Recorded in ticks of 50 microseconds.
// rawlen counts the number of entries recorded so far.
// First entry is the SPACE between transmissions.
// As soon as a SPACE gets long, ready is set, state switches to IDLE, timing of SPACE continues.
// As soon as first MARK arrives, gap width is recorded, ready is cleared, and new logging starts
ISR(TIMER_INTR_NAME)
{
	TIMER_RESET;

#ifdef RX_TAG_IT_LOAD_PIN
	digitalWrite(RX_TAG_IT_LOAD_PIN, HIGH);
#endif

 uint8_t irdata;

#ifdef RX_TAG_CHANNEL1_PIN
	irdata = (uint8_t)digitalRead(RX_TAG_CHANNEL1_PIN);
	#ifdef RX_TAG_CHANNEL1_DATABACK_PIN
		digitalWrite(RX_TAG_CHANNEL1_DATABACK_PIN, irdata);
	#endif
	RxIrChannel_1_process(irdata);
#endif
#ifdef RX_TAG_CHANNEL2_PIN
	irdata = (uint8_t)digitalRead(RX_TAG_CHANNEL2_PIN);
	#ifdef RX_TAG_CHANNEL2_DATABACK_PIN
		digitalWrite(RX_TAG_CHANNEL2_DATABACK_PIN, irdata);
	#endif
	RxIrChannel_2_process(irdata);
#endif
#ifdef RX_TAG_CHANNEL3_PIN
	irdata = (uint8_t)digitalRead(RX_TAG_CHANNEL3_PIN);
	#ifdef RX_TAG_CHANNEL3_DATABACK_PIN
		digitalWrite(RX_TAG_CHANNEL3_DATABACK_PIN, irdata);
	#endif
	RxIrChannel_3_process(irdata);
#endif
#ifdef RX_TAG_CHANNEL4_PIN
	irdata = (uint8_t)digitalRead(RX_TAG_CHANNEL4_PIN);
	#ifdef RX_TAG_CHANNEL4_DATABACK_PIN
		digitalWrite(RX_TAG_CHANNEL4_DATABACK_PIN, irdata);
	#endif
	RxIrChannel_4_process(irdata);
#endif

#ifdef RX_TAG_IT_LOAD_PIN
	digitalWrite(RX_TAG_IT_LOAD_PIN, LOW);
#endif
}

// Decodes the received IR message
// Returns 0 if no data ready, 1 if data ready.
// Results of decoding are stored in tag
unsigned char IRTagRecv::decodeTAG(IRTagDecod_t *tag)
{
	// initilization
	unsigned char decoded_count = 0;
	memset(tag, 0xFF, sizeof(IRTagDecod_t));

#ifdef RX_TAG_CHANNEL1_PIN
	if (STATE_STOP == RxIrChannel_1.rcvstate)
	{
		if (RxIrChannel_1_decode(&(tag->channel_1)))
		{
			++decoded_count;
		}
		RxIrChannel_1_resume();
	}
#endif
#ifdef RX_TAG_CHANNEL2_PIN
	if (STATE_STOP == RxIrChannel_2.rcvstate)
	{
		if (RxIrChannel_2_decode(&(tag->channel_2)))
		{
			++decoded_count;
		}
		RxIrChannel_2_resume();
	}
#endif
#ifdef RX_TAG_CHANNEL3_PIN
	if (STATE_STOP == RxIrChannel_3.rcvstate)
	{
		if (RxIrChannel_3_decode(&(tag->channel_3)))
		{
			++decoded_count;
		}
		RxIrChannel_3_resume();
	}
#endif
#ifdef RX_TAG_CHANNEL4_PIN
	if (STATE_STOP == RxIrChannel_4.rcvstate)
	{
		if (RxIrChannel_4_decode(&(tag->channel_4)))
		{
			++decoded_count;
		}
		RxIrChannel_4_resume();
	}
#endif

	return decoded_count;
}
