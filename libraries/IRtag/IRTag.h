/*
 * Infrared Lap Counter for Radio-Controlled Cars
 * Copyright 2014-2015 Team RCFree44 
 */


#ifndef IRTag_h
#define IRTag_h

#include "LIBcommon.h"

// Data settings
#define TAG_BITS        5
#define TAG_PARITY_BIT  1
#define TAG_DATA_BITS   (TAG_BITS+TAG_PARITY_BIT)
#define TAG_ID_MAX      (1 << TAG_BITS) // 32

// Freq setting
#define TAG_FREQ        36
#define TAG_PERIOD      ((1000+TAG_FREQ/2)/TAG_FREQ)

// Frame settings
#define TAG_HDR_TIME_BASE 50
#define TAG_HDR_TIME_MULT 6 // 6 is the minimum for rx w/o data corruption
#define TAG_HDR_MARK    (4*TAG_HDR_TIME_MULT*TAG_HDR_TIME_BASE) // 2400
#define TAG_HDR_SPACE   (1*TAG_HDR_TIME_MULT*TAG_HDR_TIME_BASE) // 600
#define TAG_ONE_MARK    (2*TAG_HDR_TIME_MULT*TAG_HDR_TIME_BASE) // 1200
#define TAG_ZERO_MARK   (1*TAG_HDR_TIME_MULT*TAG_HDR_TIME_BASE) // 600
#define TAG_GUARD_TIME  (4*TAG_HDR_TIME_MULT*TAG_HDR_TIME_BASE) // 2400

#define TAG_MAX_TX_TIME (TAG_HDR_MARK+TAG_HDR_SPACE+TAG_BITS*(TAG_ONE_MARK+TAG_HDR_SPACE)+TAG_PARITY_BIT*(TAG_ZERO_MARK+TAG_HDR_SPACE))

// TX Constants
// 6.5ms gives a better rx range (105 -> 125cm on worst cases)
// 6ms is the min for wide rx
// 5ms got some "grab" issue
// 4ms got some "lost" issue
// 2/3ms only works in direct rx
#define TAG_SEND_MIN_IDLE_TIME_US (6500)
#define TAG_SEND_MAX_IDLE_TIME_US (6500)

// Rx Channel
typedef struct
{
	unsigned char channel_1;
	unsigned char channel_2;
	unsigned char channel_3;
	unsigned char channel_4;
} IRTagDecod_t;

// main class for receiving IR
class IRTagRecv
{
public:
	IRTagRecv() {} ;
	unsigned char decodeTAG(IRTagDecod_t *tag);
	void enableIRIn();
	void configPrint();
private:
};

// main class for seding IR
class IRTagSend
{
public:
	IRTagSend() {}
	int GetTXPin(void);
	void sendTAG(unsigned char data);
private:
	void enableIROut(int khz);
	void mark(int usec);
	void space(int usec);
};


#endif // IRTag_h
