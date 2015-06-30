/*
 * Infrared Lap Counter for Radio-Controlled Cars
 * Copyright 2014-2015 Team RCFree44 
 */

#include "IRTagRx.h"

#ifdef RX_TAG_CHANNEL2_PIN
	void RxIrChannel_2_resume(void)
	{
		RxIrChannel_2.rawlen = 0;
		RxIrChannel_2.timer  = 0;
		RxIrChannel_2.rcvstate = STATE_IDLE;
	}

	bool RxIrChannel_2_decode(unsigned char * p_data)
	{
		unsigned char offset = 0;
		unsigned char parity = 1;
		unsigned char data = 0;
		if (RX_TAG_RAW_SIZE != RxIrChannel_2.rawlen)
		{
	#ifdef RX_TAG_CHANNEL2_PRINT_ERR
		Serial.print("?1: "); Serial.println(RxIrChannel_2.rawlen, DEC);
	#endif
			return false;
		}
		// Dont skip first space, check its size
		if (RxIrChannel_2.rawbuf[offset] < RX_TAG_DEMI_GUARD_TICKS)
		{
	#ifdef RX_TAG_CHANNEL2_PRINT_ERR
		Serial.print("?2: "); Serial.println(RxIrChannel_2.rawbuf[offset], DEC);
	#endif
			return false;
		}

		// Initial mark
		++offset;
		if (!MATCH_MARK(RxIrChannel_2.rawbuf[offset], TAG_HDR_MARK))
		{
	#ifdef RX_TAG_CHANNEL2_PRINT_ERR
		Serial.print("?3: "); Serial.println(RxIrChannel_2.rawbuf[offset], DEC);
	#endif
			return false;
		}

		// Data Extraction
		while (++offset + 1 < RxIrChannel_2.rawlen)
		{
			if (!MATCH_SPACE(RxIrChannel_2.rawbuf[offset], TAG_HDR_SPACE))
			{
				break;
			}

			++offset;
			if (MATCH_MARK(RxIrChannel_2.rawbuf[offset], TAG_ONE_MARK))
			{
				data    = (data << 1) | 1;
				parity ^= 1;
			}
			else if (MATCH_MARK(RxIrChannel_2.rawbuf[offset], TAG_ZERO_MARK))
			{
				data <<= 1;
			}
			else
			{
			#ifdef RX_TAG_CHANNEL2_PRINT_ERR
				Serial.print("?4: "); Serial.println(RxIrChannel_2.rawbuf[offset], DEC);
			#endif
				return false;
			}
		}

		// Final Check
		if (RX_TAG_RAW_SIZE != offset)
		{
		#ifdef RX_TAG_CHANNEL2_PRINT_ERR
			Serial.print("?5: "); Serial.println(offset, DEC);
		#endif
			return false;
		}
		if (parity)
		{
		#ifdef RX_TAG_CHANNEL2_PRINT_ERR
			Serial.print("?6: 0x"); Serial.println(data >> 1, HEX);
		#endif
			return false;
		}

		// Success
		*p_data = data >> 1;
		return true;
	}

	void RxIrChannel_2_process(uint8_t irdata)
	{
		// Work on the active channel
		if (STATE_STOP != RxIrChannel_2.rcvstate)
		{
			// Update Timer
			++RxIrChannel_2.timer;

			// Process State
			switch(RxIrChannel_2.rcvstate)
			{
				case STATE_IDLE: // In the middle of a gap
				{
					if (RxIrChannel_2.timer >= RX_TAG_DEMI_GUARD_TICKS)
					{
						RxIrChannel_2.timer = RX_TAG_DEMI_GUARD_TICKS;
						// wait start of frame
						if (irdata == MARK)
						{
							// start recording transmission
							RxIrChannel_2.rawbuf[RxIrChannel_2.rawlen++] = RxIrChannel_2.timer;
							RxIrChannel_2.timer    = 0;
							RxIrChannel_2.rcvstate = STATE_MARK;
						}
					}
					// check start of frame
					else if (irdata == MARK)
					{
						// Not big enough to be a gap.
						RxIrChannel_2.timer = 0;
					}
				}
				break;

				case STATE_MARK: // timing MARK
				{
					if (irdata != MARK /* == SPACE*/)
					{
						// MARK ended, record time
						RxIrChannel_2.rawbuf[RxIrChannel_2.rawlen++] = RxIrChannel_2.timer;
						RxIrChannel_2.timer    = 0;
						RxIrChannel_2.rcvstate = STATE_SPACE;
					}
				}
				break;

				case STATE_SPACE: // timing SPACE
				{
					if (irdata == MARK)
					{
						// SPACE just ended, record it
						RxIrChannel_2.rawbuf[RxIrChannel_2.rawlen++] = RxIrChannel_2.timer;
						RxIrChannel_2.timer    = 0;
						RxIrChannel_2.rcvstate = STATE_MARK;
					}
					else
					{
						// SPACE
						if (RxIrChannel_2.timer > RX_TAG_DEMI_GUARD_TICKS)
						{
							// big SPACE, indicates gap between codes
							// Mark current code as ready for processing
							RxIrChannel_2.rcvstate = STATE_STOP;
						}
					}
				}
				break;
			}

			// End of Buffer ?
			if (RxIrChannel_2.rawlen >= RX_TAG_RAW_BUF)
			{
				RxIrChannel_2.rcvstate = STATE_STOP;
			}
		}
	}

#endif // RX_TAG_CHANNEL2_PIN