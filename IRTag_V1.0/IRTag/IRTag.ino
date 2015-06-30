/*
 * Infrared Lap Counter for Radio-Controlled Cars
 * Copyright 2014-2015 Team RCFree44 
 */

#include <LIBcommon.h>
#include <IRTag.h>
#include <EEPROM.h>

// Projet Name & Version
#define PROJET_NAME    "IR/RC Lap Counter"
#define PROJET_AUTHOR  "Team RCFree44"
#define PROJET_VERSION "Tag Version V1.0"

#define PROJET_DEBUG         0
#define IR_TAG_FLASHING_LED  0

// Led pin
#define STATUS_LED       13 // Pin 13 has an LED connected on most Arduino boards.

// E2P Config
#define TAG_ID_E2P_ADDR  0

// Instances
U8     TagId;
U16    FlashTimeRef; 
bool   FlashState;
IRTagSend TagSend;

// Random
U32 Random;

// Read the Tag ID from E2P & do boundary check
void ReadID(void)
{
  TagId = EEPROM.read(TAG_ID_E2P_ADDR);
}

// Write the Tag ID & do boundary check
void WriteID(void)
{
  Serial.println("");
  Serial.println("Enter two decimal digits to update or wait 30sec to exit w/o change");
  int NewID = SerialReadNumber(30*1000UL);
  if (NewID >= 0 && NewID < TAG_ID_MAX)
  {
    TagId = NewID;
    EEPROM.write(TAG_ID_E2P_ADDR, TagId);
    Serial.println("TagId Saved.");
  }
  else
  {
    Serial.println("TagId Unchanged."); 
  }
}

void setup()
{
  // Init & Test
  Serial.begin(9600);
  randomSeed(analogRead(0));
  pinMode(STATUS_LED, OUTPUT); 

  // Program show up
  Serial.println("");
  Serial.println("------------------------------------------------");
  Serial.println(PROJET_NAME " by " PROJET_AUTHOR);
  Serial.println(PROJET_VERSION);
#if PROJET_DEBUG == 1
  Serial.println("");
  Serial.println("DEBUG");
#endif

  // Wait Valid ID configuration
#if PROJET_DEBUG == 0  
  do
  {
    digitalWrite(STATUS_LED, HIGH);

    // Read the ID
    ReadID();

    // Config change ?
    Serial.println("");
    Serial.print("Current TagId is: "); Serial.print(TagId, DEC);  Serial.println("");
    Serial.println("Press a key within 5sec to change the TagId.");

    // Update the ID in case of incomming data    
    if (SerialWaitInput(5000))
    {
      WriteID();
    }

    digitalWrite(STATUS_LED, LOW);
    delay(500);

  } while (TagId > TAG_ID_MAX); 
  delay(1500);
#endif

  // Protection Condition
  if (TagId > TAG_ID_MAX)
  {
    TagId = 0;
  }
  
  // Print Config
  Serial.println("");
  Serial.print("TagId: "); Serial.println(TagId, DEC);
  Serial.print("TX Pin: "); Serial.println(TagSend.GetTXPin(), DEC);
  Serial.print("TX Max Time: "); Serial.print(TAG_MAX_TX_TIME, DEC); Serial.println(" us");
  Serial.print("TX Idle time: ["); Serial.print(TAG_SEND_MIN_IDLE_TIME_US, DEC); Serial.print(" ; "); Serial.print(TAG_SEND_MAX_IDLE_TIME_US, DEC);  Serial.println("] us");

  // Make Flash the Status LED depends of the TagId
#if IR_TAG_FLASHING_LED == 1
  Serial.println("");
  Serial.print("Flashing Led to indicate TagId ");
  for(int i=0; i<TagId; ++i)
  {
    Serial.print(".");
    digitalWrite(STATUS_LED, HIGH);
    delay(50);
    digitalWrite(STATUS_LED, LOW);
    delay(500);
  }
  Serial.println("");
  delay(2000);
#endif

  // Print Go message !
  Serial.println("");
  Serial.println("Running !");

  // Flash Reset
  FlashTimeRef = millis();
  FlashState   = false;
}

void loop() 
{
  // Flash Status Led every 240 ms
  U16 FlashTime = (U16)millis() - FlashTimeRef;
  if (FlashTime > 240)
  {
    FlashTimeRef = (U16)millis();
    digitalWrite(STATUS_LED, HIGH);
    FlashState   = true;
  }
  // During at least 10 ms (depends of "random delay tx time")
  else if (FlashState && FlashTime > 10)
  {
    FlashState = false;
    digitalWrite(STATUS_LED, LOW);
  }
 
  // Debug - Wait Flash
#if PROJET_DEBUG == 1
  TagSend.sendTAG(TagId);
  delay(20); 
#else
  // Relase - Wait Idle Time
  #if TAG_SEND_MIN_IDLE_TIME_US < TAG_SEND_MAX_IDLE_TIME_US
    delayMicroseconds(random(TAG_SEND_MIN_IDLE_TIME_US, TAG_SEND_MAX_IDLE_TIME_US));
  #else
    delayMicroseconds(TAG_SEND_MIN_IDLE_TIME_US);
  #endif
  TagSend.sendTAG(TagId);
#endif

  // Roll ID
#if PROJET_DEBUG > 0
  if (++TagId == TAG_ID_MAX) TagId = 0;
#endif 
}
