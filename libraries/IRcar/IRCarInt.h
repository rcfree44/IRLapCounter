/*
 * Infrared Lap Counter for Radio-Controlled Cars
 * Copyright 2014-2015 Team RCFree44 
 */


#ifndef IRCarInt_h
#define IRCarInt_h

#include "LIBcommon.h"
#include "IRtag.h"

// System
extern U16  IRCar_TimeMs;
extern bool IRCar_Power_OK;
extern U8   IRCar_AutoTest_LastID;
extern void IRCar_AllUpdates(bool AllowNewCommand = false);

#define IR_CAR_FAST_REFRESH_PERIOD  100  // ms
#define IR_CAR_NORM_REFRESH_PERIOD  250  // ms
#define IR_CAR_SLOW_REFRESH_PERIOD  500  // ms

// Options
#define IR_CAR_RACE__COUNT_SINCE_ZERO    0
#define IR_CAR_RACE__FIRST_LAP_MIN_TIME  1
#define IR_CAR_RACE__CHECK_IDLE_AT_START 0
#define IR_CAR_RACE__LEADER_SEND_STATUS  1

// Reset
typedef enum
{
	RESET_NONE,
	RESET_INIT,
	RESET_RACE,
} Reset_t;

// IR
extern IRTagDecod_t TagDecod;
extern IRTagRecv    TagRecv;

// CPU Load
//#define IR_CAR_LOOP_COUNTER         1
//#define IR_CAR_LOOP_LOAD_PIN        IO_OUT_DEBUG

// Race
#define IR_CAR_MIN_LAP_TIME         8*1000UL // 8 sec
#define IR_CAR_CHECK_IDLE_TIME      2*1000UL // 2 sec
#define IR_CAR_MIN_CONFIRMED_TIME   500
#define IR_CAR_RACE_STATUS_PERIOD   5000

#define IR_CAR_LAP_STATUS_NONE         0
#define IR_CAR_LAP_STATUS_START_ENGINE 1
#define IR_CAR_LAP_STATUS_START_CHECK  2
#define IR_CAR_LAP_STATUS_START_3      3
#define IR_CAR_LAP_STATUS_START_2      4
#define IR_CAR_LAP_STATUS_START_1      5
#define IR_CAR_LAP_STATUS_RUN          6
#define IR_CAR_LAP_STATUS_LAST_LAP     7
#define IR_CAR_LAP_STATUS_FINISHED     8
#define IR_CAR_LAP_STATUS_NB           9

// Car
#define IR_CAR_CAR_WAITING    0
#define IR_CAR_CAR_CROSSING   1
#define IR_CAR_CAR_CONFIRMED  2

// Signal
#define IR_CAR_SIGNAL_LED     IO_OUT_STATUS
#define IR_CAR_SIGNAL_BIP_ON  100
#define IR_CAR_SIGNAL_BIP_OFF 100
extern void IRCar_Signal_Reset(Reset_t Reset = RESET_NONE);
extern void IRCar_Signal_Start(U16 ON_Duration, U16 OFF_Duration, U8 Repeat, bool WaitEnd = false);
extern void IRCar_Signal_AddBip(U8 Count);
extern void IRCar_Signal_TryBip(void);
extern void IRCar_Signal_PlayBip(void);
extern void IRCar_Signal_Process(void);

// Light
#define IR_CAR_LIGHT_PWM_RED1     IO_OUT_LIGHT1
#define IR_CAR_LIGHT_PWM_RED2     IO_OUT_LIGHT2
#define IR_CAR_LIGHT_PWM_RED3     IO_OUT_LIGHT3
#define IR_CAR_LIGHT_PWM_GREEN    IO_OUT_LIGHT4
#define IR_CAR_LIGHT_PWM_MAXON    255
#define IR_CAR_LIGHT_AUTO_STOP    5000
extern void IRCar_Light_Reset(Reset_t Reset = RESET_NONE);
extern void IRCar_Light_Update(void);
extern void IRCar_Light_Process(void);

// DotMatrix
#define IR_CAR_DOTMATRIX_AUTO_STOP    4000
#define IR_CAR_DOTMATRIX_FLASH_PERIOD  400
extern void IRCar_DotMatrix_Reset(Reset_t Reset = RESET_NONE);
extern void IRCar_DotMatrix_Update(void);
extern void IRCar_DotMatrix_RaceUpdate(U8 Lap_Downcount);
extern void IRCar_DotMatrix_Process(void);

// Menu
#define IR_CAR_MENU_TIME_OUT           5000
#define IR_CAR_MENU_BACKLIGHT_ON      10000
#define IR_CAR_MENU_RACE_MAX_TIME        30 // minutes
#define IR_CAR_MENU_RACE_MAX_LAP         30 // count
#define IR_CAR_MENU_RACE_LENGHT_MIN  (25*100UL) // in centimeters
#define IR_CAR_MENU_RACE_LENGHT_MAX  (500*100UL) // in centimeters
#define IR_CAR_MENU_RACE_LENGHT_INC_LOW   (25)
#define IR_CAR_MENU_RACE_LENGHT_INC_HIH  (200)
extern void IRCar_Menu_Reset(Reset_t Reset = RESET_NONE);
extern void IRCar_Menu_Update(void);
extern void IRCar_Menu_Process(bool AllowNewCommand = false);

// Status
extern void IRCar_Status_Process(void);
extern void IRCar_Status_Race_Send();
extern void IRCar_Status_Car_Send(U8 ID);

// Command
#define IR_CAR_CMD_MAX_SIZE      20
#define IR_CAR_CMD_TIMEOUT       100
extern void IRCar_Command_Reset(Reset_t Reset = RESET_NONE);
extern void IRCar_Command_Process(bool AllowNewCommand = false);

#define IR_CAR_COMMAND_RESET         0x00
#define IR_CAR_COMMAND_SEND_STATUS   0x01
#define IR_CAR_COMMAND_START_STATUS  0x02
#define IR_CAR_COMMAND_STOP_STATUS   0x03

#define IR_CAR_COMMAND_START_RACE    0x10

#define IR_CAR_COMMAND_LAST_LAP      0x20
#define IR_CAR_COMMAND_FINISH        0x21

#define IR_CAR_COMMAND_TEST_MODE     0x30
#define IR_CAR_COMMAND_SIMULATION    0x31

// Battery
#define IR_CAR_ALARM__2SLIPO_MV_MIN   6600 // 3.3V x2
#define IR_CAR_ALARM__2SLIPO_MV_MAX   8400 // 4.2V x2
#define IR_CAR_ALARM__COUNT_DETECTION 5
#define IR_CAR_ALARM__COUNT_RESETHOLD 10
extern bool IRCar_Battery_LipoDetected; // does a external "lipo" battery detected ?
extern void IRCar_Battery_Init(void);
extern bool IRCar_Battery_Check(void);
extern U8   IRCar_Battery_Voltage(void);

// Test & Simulation
#define IR_CAR_TEST_MODE_TIME        1*60*1000UL  // 1 mn
#define IR_CAR_TEST_SIMU_TIME        5*60*1000UL  // 5 mn

// Car
typedef struct
{
	U32 race_reference_time; // used to send status & score the car position (stop counted when the race is over)
	U32 best_lap_time;       // best lap time updated when a new lap is counter
	U32 last_reference_time; // last internal time used to count lap time
	U32 reference_time;      // internal time used as reference
	U32 total_crossing_time; // total crossing time 
	U16 best_crossing_time;  // best crossing time
	U16 crossing_time;       // internal crossing time
	U8  lap_count;           // lap counting
	U8  car_status;          // detection status
} IRCar_t;
extern IRCar_t  IRCars[TAG_ID_MAX];
extern bool IRCar_Car_Process(U8 ID, U32 Time);
extern void IRCar_Car_Status_Update_Send(U32 Time);

// Race
typedef struct
{
	U32 time_ref;
	U32 last_time_status;
	U32 lap_time_max;
	U8  lap_mode;
	U8  lap_status;
	U8  lap_max;
	U8  leaded_car;
	bool debug_mode;
} IRRace_t;
extern IRRace_t IRRace;

// Race
extern void IRCar_Race_SetTime();
extern U32  IRCar_Race_GetTime();
extern void IRCar_Race_NewStatus(U8 State = 0xFF);
extern void IRCar_Race_RaceUpdate(U8 Lap_Downcount);
extern void IRCar_Race_Reset(Reset_t Reset = RESET_NONE);
extern void IRCar_Race_Start(U8 MaxLap, U32 MaxTime, U8 DebugMode);
extern bool IRCar_Race_CountNewLap(U8 ID);
extern U8   IRCar_Race_GetTopPos(U8 ID);

// Save
typedef struct
{
  U16 RaceLenght; // in centimeter
  U8  MaxLap;
  U8  MaxTim;
} IRSave_t;
extern IRSave_t IRSave;

// Test & Simu
// #define IR_CAR_TESTMODE__FIXED_ID 1
extern void IRCar_Race_TestMode(void);
extern void IRCar_Race_Simulation(U8 MaxLap, U32 MaxTime, U8 DebugMode);

#endif // IRCarInt_h
