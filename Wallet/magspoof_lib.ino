/*
 * MagSpoof - "wireless" magnetic stripe/credit card emulator
*
 * by Samy Kamkar
 *
 * http://samy.pl/magspoof/
 *
 * - Allows you to store all of your credit cards and magstripes in one device
 * - Works on traditional magstripe readers wirelessly (no NFC/RFID required)
 * - Can disable Chip-and-PIN (code not included)
 * - Correctly predicts Amex credit card numbers + expirations from previous card number (code not included)
 * - Supports all three magnetic stripe tracks, and even supports Track 1+2 simultaneously
 * - Easy to build using Arduino or ATtiny
 *
 */

 /**
  * Code modified by Satyendra Emani for use with ATmega328 and 1
  * transistor
  */

#include <avr/sleep.h>
#include <avr/interrupt.h>

#define CLOCK_US 200

#define BETWEEN_ZERO 53 // 53 zeros between track1 & 2

void blink(int pin, int msdelay, int times)
{
  for (int i = 0; i < times; i++)
  {
    digitalWrite(pin, HIGH);
    delay(msdelay);
    digitalWrite(pin, LOW);
    delay(msdelay);
  }
}

// send a single bit out
void playBit(int sendBit)
{
  dir ^= 1;
  digitalWrite(PIN_A, dir);
  delayMicroseconds(CLOCK_US);

  if (sendBit)
  {
    dir ^= 1;
    digitalWrite(PIN_A, dir);
  }
  delayMicroseconds(CLOCK_US);

}

// when reversing
void reverseTrack(int track)
{
  int i = 0;
  track--; // index 0
  dir = 0;

  while (revTrack[i++] != '\0');
  i--;
  while (i--)
    for (int j = bitlen[track]-1; j >= 0; j--)
      playBit((revTrack[i] >> j) & 1);
}

// plays out a full track, calculating CRCs and LRC
void playTrack(int track)
{
  int tmp, crc, lrc = 0;
  track--; // index 0
  dir = 0;

  // enable H-bridge and LED
  digitalWrite(ENABLE_PIN, HIGH);
  

  // First put out a bunch of leading zeros.
  for (int i = 0; i < 25; i++)
    playBit(0);

  //
  for (int i = 0; cardTracks[track][i] != '\0'; i++)
  {
    crc = 1;
    tmp = cardTracks[track][i] - sublen[track];

    for (int j = 0; j < bitlen[track]-1; j++)
    {
      crc ^= tmp & 1;
      lrc ^= (tmp & 1) << j;
      playBit(tmp & 1);
      tmp >>= 1;
    }
    playBit(crc);
  }

  // finish calculating and send last "byte" (LRC)
  tmp = lrc;
  crc = 1;
  for (int j = 0; j < bitlen[track]-1; j++)
  {
    crc ^= tmp & 1;
    playBit(tmp & 1);
    tmp >>= 1;
  }
  playBit(crc);

//  // if track 1, play 2nd track in reverse (like swiping back?)
//  if (track == 0)
//  {
//    // if track 1, also play track 2 in reverse
//    // zeros in between
//    for (int i = 0; i < BETWEEN_ZERO; i++)
//      playBit(0);
//
//    // send second track in reverse
//    reverseTrack(2);
//  }

  // finish with 0's
  for (int i = 0; i < 25; i++)
    playBit(0);

  digitalWrite(PIN_A, LOW);
  digitalWrite(ENABLE_PIN, LOW);

}



// stores track for reverse usage later
void storeRevTrack(int track)
{
  int i, tmp, crc, lrc = 0;
  track--; // index 0
  dir = 0;

  for (i = 0; cardTracks[track][i] != '\0'; i++)
  {
    crc = 1;
    tmp = cardTracks[track][i] - sublen[track];

    for (int j = 0; j < bitlen[track]-1; j++)
    {
      crc ^= tmp & 1;
      lrc ^= (tmp & 1) << j;
      tmp & 1 ?
        (revTrack[i] |= 1 << j) :
        (revTrack[i] &= ~(1 << j));
      tmp >>= 1;
    }
    crc ?
      (revTrack[i] |= 1 << 4) :
      (revTrack[i] &= ~(1 << 4));
  }

  // finish calculating and send last "byte" (LRC)
  tmp = lrc;
  crc = 1;
  for (int j = 0; j < bitlen[track]-1; j++)
  {
    crc ^= tmp & 1;
    tmp & 1 ?
      (revTrack[i] |= 1 << j) :
      (revTrack[i] &= ~(1 << j));
    tmp >>= 1;
  }
  crc ?
    (revTrack[i] |= 1 << 4) :
    (revTrack[i] &= ~(1 << 4));

  i++;
  revTrack[i] = '\0';
}

void sleep()
{
  PCICR |= _BV(PCIE0);                     // Enable Pin Change Interrupts
  PCMSK0 |= _BV(PCINT3);                   // Use PB3 as interrupt pin
  
  ADCSRA &= ~_BV(ADEN);                   // ADC off
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);    // replaces above statement


  sleep_enable();                         // Sets the Sleep Enable bit in the MCUCR Register (SE BIT)
  sei();                                  // Enable interrupts
  sleep_cpu();                            // sleep

  cli();                                  // Disable interrupts
  PCMSK0 &= ~_BV(PCINT3);                  // Turn off PB3 as interrupt pin
  sleep_disable();                        // Clear SE bit

  sei();                                  // Enable interrupts
}

ISR(PCINT0_vect) {
  switch(walletState){
    case WAITING:
      walletState = SELECT;
      break;
    case SELECT:
      walletState = INIT;
  }
}

//void loop()
//{
//
//
//  sleep();
//
//  noInterrupts();
//  while (digitalRead(BUTTON_PIN) == LOW);
//  
//  delay(50);
//  while (digitalRead(BUTTON_PIN) == LOW);
//  playTrack(1 + (curTrack++ % 2));
//  delay(400);
//  
//  interrupts();
//}
