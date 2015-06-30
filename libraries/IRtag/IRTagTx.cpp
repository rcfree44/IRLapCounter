/*
 * IRremote
 * Version 0.11 August, 2009
 * Copyright 2009 Ken Shirriff
 * For details, see http://arcfn.com/2009/08/multi-protocol-infrared-remote-library.html
 *
 * Modified by Paul Stoffregen <paul@pjrc.com> to support other boards and timers
 * Modified  by Mitra Ardron <mitra@mitra.biz> 
 * Added Sanyo and Mitsubishi controllers
 * Modified Sony to spot the repeat codes that some Sony's send
 *
 * Interrupt code based on NECIRrcv by Joe Knapp
 * http://www.arduino.cc/cgi-bin/yabb2/YaBB.pl?num=1210243556
 * Also influenced by http://zovirl.com/2008/11/12/building-a-universal-remote-with-an-arduino/
 *
 * JVC and Panasonic protocol added by Kristian Lauszus (Thanks to zenwheel and other people at the original blog post)
 */

#include "IRTag.h"
#include "IRTagInt.h"
#include <avr/interrupt.h>

int IRTagSend::GetTXPin(void)
{
  return TIMER_PWM_PIN;
}

void IRTagSend::sendTAG(unsigned char data) 
{
#if TAG_BITS+TAG_PARITY_BIT >= 8
  #error Configuration Error
#endif
  // Add Parity Bit to data
  unsigned char parity_bit = (1 << (8-(TAG_BITS+TAG_PARITY_BIT))); 
  for (unsigned char i = 0; i < TAG_BITS; i++)
  {
    if (data & (1<<i))
    {
      parity_bit ^= (1 << (8-(TAG_BITS+TAG_PARITY_BIT)));
    }
  }
  data <<= (8-TAG_BITS);  
  data  |= parity_bit;

  // disable interrupts
  cli();

  // Send Data + Parity Bit
  enableIROut(TAG_FREQ);
  mark(TAG_HDR_MARK - TAG_PERIOD);
  space(TAG_HDR_SPACE + TAG_PERIOD);
  for (unsigned char i = 0; i < (TAG_BITS+TAG_PARITY_BIT); i++) 
  {
    if (data & 0x80)
    {
      mark(TAG_ONE_MARK - TAG_PERIOD);
      space(TAG_HDR_SPACE + TAG_PERIOD);
    }
    else
    {
      mark(TAG_ZERO_MARK - TAG_PERIOD);
      space(TAG_HDR_SPACE + TAG_PERIOD);
    }
    data <<= 1;
  }

  // enable interrupts
  sei();
}

void IRTagSend::mark(int time) 
{
  // Sends an IR mark for the specified number of microseconds.
  // The mark output is modulated at the PWM frequency.
  TIMER_ENABLE_PWM; // Enable pin 3 PWM output
  delayMicroseconds(time);
}

/* Leave pin off for time (given in microseconds) */
void IRTagSend::space(int time) 
{
  // Sends an IR space for the specified number of microseconds.
  // A space is no output, so the PWM output is disabled.
  TIMER_DISABLE_PWM; // Disable pin 3 PWM output
  delayMicroseconds(time);
}

void IRTagSend::enableIROut(int khz) 
{
  // Enables IR output.  The khz value controls the modulation frequency in kilohertz.
  // The IR output will be on pin 3 (OC2B).
  // This routine is designed for 36-40KHz; if you use it for other values, it's up to you
  // to make sure it gives reasonable tag.  (Watch out for overflow / underflow / rounding.)
  // TIMER2 is used in phase-correct PWM mode, with OCR2A controlling the frequency and OCR2B
  // controlling the duty cycle.
  // There is no prescaling, so the output frequency is 16MHz / (2 * OCR2A)
  // To turn the output on and off, we leave the PWM running, but connect and disconnect the output pin.
  // A few hours staring at the ATmega documentation and this will all make sense.
  // See my Secrets of Arduino PWM at http://arcfn.com/2009/07/secrets-of-arduino-pwm.html for details.

  
  // Disable the Timer2 Interrupt (which is used for receiving IR)
  TIMER_DISABLE_INTR; //Timer2 Overflow Interrupt
  
  pinMode(TIMER_PWM_PIN, OUTPUT);
  digitalWrite(TIMER_PWM_PIN, LOW); // When not sending PWM, we want it low
  
  // COM2A = 00: disconnect OC2A
  // COM2B = 00: disconnect OC2B; to send signal set to 10: OC2B non-inverted
  // WGM2 = 101: phase-correct PWM with OCRA as top
  // CS2 = 000: no prescaling
  // The top value for the timer.  The modulation frequency will be SYSCLOCK / 2 / OCR2A.
  TIMER_CONFIG_KHZ(khz);
}
