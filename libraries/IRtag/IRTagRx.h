/*
 * Infrared Lap Counter for Radio-Controlled Cars
 * Copyright 2014-2015 Team RCFree44 
 */

#ifndef IRTagRx_h
#define IRTagRx_h

#include "IRTag.h"
#include "IRTagInt.h"
#include <avr/interrupt.h>

// Debug
// #define RX_TAG_IT_LOAD_PIN             IO_OUT_DEBUG
// #define RX_TAG_CHANNEL1_DATABACK_PIN   IO_OUT_DEBUG
// #define RX_TAG_CHANNEL2_DATABACK_PIN   IO_OUT_DEBUG
// #define RX_TAG_CHANNEL3_DATABACK_PIN   IO_OUT_DEBUG
// #define RX_TAG_CHANNEL4_DATABACK_PIN   IO_OUT_DEBUG

// Trace
//#define RX_TAG_CHANNEL1_PRINT_ERR
//#define RX_TAG_CHANNEL2_PRINT_ERR
//#define RX_TAG_CHANNEL3_PRINT_ERR
//#define RX_TAG_CHANNEL3_PRINT_ERR

// Channel Config
#define RX_TAG_CHANNEL1_PIN IO_IN_CHANNEL1
#define RX_TAG_CHANNEL2_PIN IO_IN_CHANNEL2
#define RX_TAG_CHANNEL3_PIN IO_IN_CHANNEL3
#define RX_TAG_CHANNEL4_PIN IO_IN_CHANNEL4

// receiver states
#define STATE_STOP     0
#define STATE_IDLE     1
#define STATE_MARK     2
#define STATE_SPACE    3

// IR detector output is active low
#define MARK           0
#define SPACE          1

// Length of raw duration buffer
#define RX_TAG_RAW_SIZE          (2*TAG_DATA_BITS  + 2) // +2 headers bits
#define RX_TAG_RAW_BUF           (RX_TAG_RAW_SIZE  + 1) // +1 for overflow detection
#define RX_TAG_DEMI_GUARD_TICKS  (TAG_GUARD_TIME/(2*IR_TAG_USECPERTICK))

// information for the interrupt handler
typedef struct {
	uint8_t rcvstate;  // state machine
	uint8_t rawlen;    // counter of entries in rawbuf
	uint8_t timer;     // state timer, counts 50uS ticks.
	uint8_t rawbuf[RX_TAG_RAW_BUF]; // raw data
}
RxIrChannel_t;

// Mesurements contants
#define MATCH_MARK(measured_ticks, desired_us)                \
	((measured_ticks >= ((desired_us/TAG_HDR_TIME_BASE)-2)) &&  \
	 (measured_ticks <= ((desired_us/TAG_HDR_TIME_BASE)+2))  )

#define MATCH_SPACE(measured_ticks, desired_us)               \
	((measured_ticks >= ((desired_us/TAG_HDR_TIME_BASE)-2)) &&  \
	 (measured_ticks <= ((desired_us/TAG_HDR_TIME_BASE)+2))  )

// Rx Instance
extern volatile RxIrChannel_t RxIrChannel_1;
extern volatile RxIrChannel_t RxIrChannel_2;
extern volatile RxIrChannel_t RxIrChannel_3;
extern volatile RxIrChannel_t RxIrChannel_4;

// Rx Functions
	// Channel 1
	extern void RxIrChannel_1_resume();
	extern void RxIrChannel_1_process(uint8_t irdata);
	extern bool RxIrChannel_1_decode(unsigned char * p_data);
	// Channel 2
	extern void RxIrChannel_2_resume();
	extern void RxIrChannel_2_process(uint8_t irdata);
	extern bool RxIrChannel_2_decode(unsigned char * p_data);
	// Channel 3
	extern void RxIrChannel_3_resume();
	extern void RxIrChannel_3_process(uint8_t irdata);
	extern bool RxIrChannel_3_decode(unsigned char * p_data);
	// Channel 4
	extern void RxIrChannel_4_resume();
	extern void RxIrChannel_4_process(uint8_t irdata);
	extern bool RxIrChannel_4_decode(unsigned char * p_data);

#endif // IRTagRx_h
