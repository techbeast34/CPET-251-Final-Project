#include <LiquidCrystal.h>

enum walletState_t {WAITING, INIT, SELECT, TRANSMIT, DOWNLOAD};
walletState_t walletState = WAITING;
walletState_t prevWalletState = -1;
String inputString = "";
String track = "";

#define EEPROM_C1 0
#define EEPROM_C2 256
#define EEPROM_C3 512
#define EEPROM_C4 768
//There's enough EEPROM storage to store 4 cards
//could store more if all three tracks were not
//stored, or if tracks were encoded and then stored.


#define T1_LEN 81
#define T2_LEN 42
#define T3_LEN 109
//Total length of 232 bytes

#define PIN_A 5 //Connect to base on transistor, PD5
#define ENABLE_PIN 13 //Built-in LED, PB5
#define BUTTON_PIN 11 //PB3
#define SEL_B_PIN 3 //PD3


#define TRACKS 3

unsigned int cardLoc = 0;
char cardNum = '1';
boolean isNewState = true;
unsigned int curTrack = 0;
String cardTracks[3];

char revTrack[41];

const int sublen[] = {
  32, 48, 48 };
const int bitlen[] = {
  7, 5, 5 };
  
int dir;

//LiquidCrystal lcd(A5, A4, A3, A2, A1, A0); //Character LCD goes on A0-A5
//LCD was borrowed for the project. For portability, reprogram to use
//LEDs. Card 1 is also the default card so this can run without any
//output if you only care about one card.

void setup() {
  Serial.begin(115200);
  Serial.println(F("Setup complete"));
  
//  replaced with ddr commands, kept in case stuff breaks or something
//  pinMode(PIN_A, OUTPUT);
//  pinMode(ENABLE_PIN, OUTPUT);
//  pinMode(BUTTON_PIN, INPUT_PULLUP);
  

  DDRB &= 0xF7;
  DDRB |= 0x20;
  PORTB |= 0x08;
  //PB5 as output, PB3 as input w/ pullup

  DDRD &= 0xF7;
  DDRD |= 0x20;
  PORTD |= 0x08; 
  //PD5 as output, PD3 as input w/ pullup
  

  // blink to show we started up
  blink(ENABLE_PIN, 200, 3);

  //lcd.begin(16, 2);
  //lcd.clear();
}

void loop() {
  isNewState = (walletState != prevWalletState);
  prevWalletState = walletState;

  switch (walletState) {
    case WAITING:
      if(isNewState){
        //lcd.clear();
        Serial.println(F("Entered WAIT"));
        //lcd.print("Sleeping ^_^");
        delay(500);
        sleep();
      }
      break;
    case INIT:
      //lcd.clear();
      Serial.println(F("Entered INIT"));
      for (int i = cardLoc; i < (cardLoc + 81); i++) {
        if (EEPROM_read(i) != 0x00) {
          track += (char) EEPROM_read(i);
        }
      }
      cardTracks[0] = track;
      Serial.println(cardTracks[0]);
      Serial.println(F("T1 retreived"));
      track = "";
      
      for (int i = (cardLoc + 81); i < (cardLoc + 124); i++) {
        if (EEPROM_read(i) != 0x00) {
          track += (char) EEPROM_read(i);
        }
      }
      cardTracks[1] = track;
      Serial.println(cardTracks[1]);
      Serial.println(F("T2 retreived"));
      track = "";
      
      for (int i = (cardLoc + 124); i < (cardLoc + 234); i++) {
        if (EEPROM_read(i) != 0x00) {
          track += (char) EEPROM_read(i);
        }
      }

      cardTracks[2] = track;
      Serial.println(cardTracks[2]);
      Serial.println(F("T3 retreived"));
      track = "";
      
      storeRevTrack(2); //Store track 2 for reverse play

      walletState = TRANSMIT;
      break;
    case SELECT:
      if(isNewState){
        //lcd.clear();
        Serial.println(F("Entered SELECT"));
        ////         0000000000000000
        //lcd.print("Select a card");
      }

      //lcd.setCursor(0,1);
      if(!(PIND & 0x08)){
        cardNum += 1; //Increment character
        if(cardNum > 52) cardNum = 49; //Reset to '1' char
      }

      if(!(PINB & 0x08)){
        walletState = INIT;
      }
      
      switch (cardNum) {
        case '1':
          cardLoc = EEPROM_C1;
          //lcd.print("(1)  2   3   4  ");
          break;
        case '2':
          cardLoc = EEPROM_C2;
          //lcd.print(" 1  (2)  3   4  ");
          break;
        case '3':
          cardLoc = EEPROM_C3;
          //lcd.print(" 1   2  (3)  4  ");
          break;
        case '4':
          cardLoc = EEPROM_C4;
          //lcd.print(" 1   2   3  (4) ");
          break;
        default:
          cardLoc = EEPROM_C1;
          break;
      }
      break;
    case TRANSMIT:
      //lcd.print("Transmitting");
      cli();
      playTrack(1);
      delayMicroseconds(500);
      playTrack(2);
      delayMicroseconds(000);
      //playTrack(3);
      /*
       * If this is used with credit cards, track 3 should
       * not be transmitted. Most cards do not physically
       * hold track 3.
       * 
       * Due to physics, we can't ensure that the read heads for different
       * tracks won't pickup the data from other tracks transmitting. So
       * if you see data from track 1 on track 3 when using a card reader,
       * don't freak.
       * 
       */
      sei();
      Serial.println(F("Exited TRANSMIT"));
      walletState = WAITING;
      //lcd.clear();
      break;
      
    case DOWNLOAD:
      if (isNewState) {
        //Entry
        download();
      }
      //State Business

      //Exit
      walletState = WAITING;
      Serial.println(F("Exiting download mode"));
      break;
  }

  
  inputString = Serial.readString();
  if (inputString == "DOWNLOAD") walletState = DOWNLOAD;
  
}//loop()
//==========================================================
void download() {
  //lcd.clear();
//  lcd.print("DOWNLOAD MODE");
  Serial.println(F("Entered Download Mode"));
  Serial.println(F("Which card would you like to write?"));
  Serial.println(F("(1)    (2)    (3)    (4)"));

  while (!Serial.available()) {}

  cardNum = Serial.read(); //Card selection
  switch (cardNum) {
    case '1':
      cardLoc = EEPROM_C1;
      break;
    case '2':
      cardLoc = EEPROM_C2;
      break;
    case '3':
      cardLoc = EEPROM_C3;
      break;
    case '4':
      cardLoc = EEPROM_C4;
      break;
    default:
      Serial.println(F("Invalid card selection! Exiting."));
      return 0;
      break;
  }

  Serial.println(F("Enter in track 1 (NO CONTROL CHARACTERS EXCEPT FS)"));
  while (!Serial.available()) {}
  inputString = Serial.readString();
  inputString = "%B" + inputString + "?\0"; //Add in control characters
  while (inputString.length() > 81) { //check to see if length is valid (compliant to ISO-7813 standards)
    Serial.println(F("Track too long! Enter in again"));
    while (!Serial.available()) {}
    inputString = Serial.readString();
    inputString = "%B" + inputString + "?\0";
  }
  cardTracks[0] = inputString;

  Serial.println(F("Enter in track 2 (NO CONTROL CHARACTERS EXCEPT FS)"));
  while (!Serial.available()) {}
  inputString = Serial.readString();
  inputString = ";" + inputString + "?\0";
  while (inputString.length() > 42) {
    Serial.println(F("Track too long! Enter in again"));
    while (!Serial.available()) {}
    inputString = Serial.readString();
    inputString = ";" + inputString + "?\0";
  }
  cardTracks[1] = inputString;

  Serial.println(F("Enter in track 3 (NO CONTROL CHARACTERS EXCEPT FS)"));
  while (!Serial.available()) {}
  inputString = Serial.readString();
  inputString = ";" + inputString + "?\0";
  while (inputString.length() > 109) {
    Serial.println(F("Track too long! Enter in again"));
    while (!Serial.available()) {}
    inputString = Serial.readString();
    inputString = ";" + inputString + "?\0";
  }
  cardTracks[2] = inputString;

  Serial.println(F("Storing this card:"));

  //Store & print track 1
  for (int i = cardLoc; i < (cardLoc + 81); i++) {
    if ((i - cardLoc) < cardTracks[0].length()) {
      EEPROM_write(i, cardTracks[0][i - cardLoc]);
    }
    else {
      EEPROM_write(i, 0x00);
    }
  }
  //Do this to verify that the card was written correctly to EEPROM, if it fails then
  //the EEPROM is starting to get corrupted
  for (int i = cardLoc; i < (cardLoc + 81); i++) {
    if (EEPROM_read(i) != 0x00) {
      Serial.print( (char) EEPROM_read(i));
    }
  }
  Serial.println();
  //=====
  //Store & print track 2
  for (int i = (cardLoc + 81); i < (cardLoc + 124); i++) {
    if ((i - (cardLoc + 81)) < cardTracks[1].length()) {
      EEPROM_write(i, cardTracks[1][i - (cardLoc + 81)]);
    }
    else {
      EEPROM_write(i, 0x00);
    }
  }

  for (int i = (cardLoc + 81); i < (cardLoc + 124); i++) {
    if (EEPROM_read(i) != 0x00) {
      Serial.print( (char) EEPROM_read(i));
    }
  }
  Serial.println();
  //=====
  //Store & print track 3
  for (int i = (cardLoc + 124); i < (cardLoc + 234); i++) {
    if ((i - (cardLoc + 124)) < cardTracks[2].length()) {
      EEPROM_write(i, cardTracks[2][i - (cardLoc + 124)]);
    }
    else {
      EEPROM_write(i, 0x00);
    }
  }

  for (int i = (cardLoc + 124); i < (cardLoc + 234); i++) {
    if (EEPROM_read(i) != 0x00) {
      Serial.print( (char) EEPROM_read(i));
    }
  }
  Serial.println();
  cardTracks[0] = "";
  cardTracks[1] = "";
  cardTracks[2] = ""; //Reset tracks after running

}//download()
//===========================================================
void EEPROM_write(unsigned int Address, byte Data)
{
  /* Wait for completion of previous write. EEPE clears when write completes.*/
  while (EECR & 0b00000010) { }
  /* Set up address and Data Registers */
  EEAR = Address;
  EEDR = Data;
  cli(); //to ensure no interrupts occur between next two steps
  /* Write logical one to EEMPE */
  EECR |= 0b00000100;
  /* Start EEPROM write by setting EEPE */
  EECR |= 0b00000010;
  sei();
}//EEPROM_write
//===========================================================
byte EEPROM_read(unsigned int Address)
{
  /* Wait for completion of previous write. EEPE clears on completion */
  while (EECR & 0b00000010) { }
  /* Set up address register */
  EEAR = Address;
  /* Start EEPROM read by writing EERE */
  EECR |= 0b00000001;
  /* Return data from Data Register */
  return EEDR;
}//EEPROM_read
//===========================================================
