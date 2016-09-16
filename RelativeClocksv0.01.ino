#include <EEPROM.h>

/*
  Relative Clocks Arduino Code
  Jonathan Chomko Sept 2016
*/
#include <DS3231.h>
#include <SPI.h>
#include <SD.h>

//RTC variables
DS3231 clock;
RTCDateTime dt;

//SD card file reading
File data;
int index;
char buf[40];
long filePosition;

//Hand values
unsigned long currentOffset;
unsigned long offsetTimer;
bool hand1;
bool hand2;
int earthHandAngle;
int spaceHandAngle;

bool getData;
String dataDate = "";
String currDate = "";
int beepPin = 10;
int attemptCount;
int maxAttempts = 10;


void setup()
{

  Serial.begin(57600);

  Serial.println("Initialize DS3231");;

  clock.begin();

  Serial.setTimeout(150);

  // Disarm alarms and clear alarms for this example, because alarms is battery backed.
  // Under normal conditions, the settings should be reset after power and restart microcontroller.
  clock.armAlarm1(false);
  clock.armAlarm2(false);
  clock.clearAlarm1();
  clock.clearAlarm2();

  // Set Alarm - Every second.
  //Trigger every second
  clock.setAlarm1(0, 0, 0, 0, DS3231_EVERY_SECOND);

  //Trigger every day
  clock.setAlarm2(0, 0, 10, DS3231_MATCH_H_M, true);

  pinMode(5, OUTPUT);
  pinMode(6, OUTPUT);

  if (!SD.begin()) {
    Serial.println("initialization failed!");
    // return;
  }

  Serial.println("initialization done.");

  if (data) {
    data = SD.open("DATA.TXT");
  }

  dt = clock.getDateTime();
  char * cd = clock.dateFormat("Y-m-d", dt);
  currDate = String(cd);
  Serial.println(currDate);

  //startDataCheck();

}



void loop()
{

  //Set time via Serial
  if (Serial.available() > 0) {

    String s =  Serial.readStringUntil('\n');

    Serial.println(s);

    //If READ
    if (s.charAt(0) == 'R') {

      dt = clock.getDateTime();
      Serial.println(clock.dateFormat("d-m-Y H:i:s - l", dt));

      //Set RTC
    } else if (s.charAt(0) == 'S') {


      String y = s.substring(1, 5);
      String m = s.substring(5, 7);
      String d = s.substring(7, 9);
      String hr = s.substring(9, 11);
      String mn = s.substring(11, 13);
      String ss = s.substring(13, 15);
      String out = y + m + d + hr + mn + ss;

      int iy  = y.toInt();
      int im  = m.toInt();
      int id = d.toInt();
      int ihr = hr.toInt();
      int imn = mn.toInt();
      int iss = ss.toInt();

      Serial.println(iy);
      Serial.println(im);
      Serial.println(id);
      Serial.println(ihr);
      Serial.println(imn);
      Serial.println(iss);

      clock.setDateTime(iy, im, id, ihr, imn, iss);

    } else if (s.charAt(0) == 'U') {

      Serial.println("updating");
      startDataCheck();

    }

    else if (s.charAt(0) == 'C') {

      char * cd = clock.dateFormat("Y-m-d", dt);
      currDate = String(cd);
      Serial.println(currDate);

    } else if (s.charAt(0) == 'B') {

      long lastOffest = EEPROMReadlong(0);
      Serial.println(lastOffest);

    }


  }


  unsigned long currentMillis = millis();

  //Trigger for Earth Hand
  if (clock.isAlarm1())
  {
    //Serial.write(eartHand);
    //eartHand += 6
    tone(beepPin, 700, 20);
    digitalWrite(11, HIGH);

    //Reset Hand catches
    hand1 = true;
    hand2 = true;

    //Start offset timer
    offsetTimer = currentMillis;

  }


  //Turn off earth LED
  if (currentMillis - offsetTimer > 30 && hand1 == true) {
    hand1 = false;
    //led
    digitalWrite(11, LOW);
  }

  //Trigger for Space Hand
  if (currentMillis - offsetTimer > currentOffset && hand2 == true ) {
    hand2 = false;
    //led
    digitalWrite(12, HIGH);
    tone(beepPin, 700, 20);
    //spaceHand += 6
    //Serial.write(spaceHand);
  }

  //Turn off space LED
  if (millis() - offsetTimer > currentOffset + 30 && hand2 == false) {
    digitalWrite(12, LOW);
  }

  //Trigger once a day to get new offset data
  if (clock.isAlarm2()) {
    startDataCheck();
  }

  checkData();

}

void startDataCheck() {

  dt = clock.getDateTime();
  char * cd = clock.dateFormat("Y-m-d", dt);
  currDate = String(cd);
  //Serial.println(currDate);

  
  if (data) {
    data = SD.open("DATA.TXT");
  } else {
    if (!SD.begin()) {
      Serial.println("initialization failed!");
    } else {
      Serial.println("initialization done.");
    }
  }

  getData = true;

}


void checkData() {

  if ( getData) {

    if (dataDate != currDate) {

      if (data.available()) {
        char c = data.read();

        if (index < 40) {
          buf[index] = c;
          index ++;
        }

        if (c == '\n') {

          String s = String(buf);
          dataDate = s.substring(0, 10);

          String offset = s.substring(22, 32);
          currentOffset = (unsigned long)offset.toFloat();

          Serial.println(dataDate);
          index = 0;

        }

      } else {

        Serial.println("data not available");
        data = SD.open("DATA.TXT");
        attemptCount ++;

        if (attemptCount > maxAttempts) {
          getData = false;
          currentOffset = (unsigned long)EEPROMReadlong(0);
            
        }
        
      }

    } else {

      Serial.println("match found");
      Serial.println(dataDate);
      Serial.println(currDate);
      Serial.println(currentOffset);

      //filePosition = data.position();
      Serial.println(filePosition);

      if (currentOffset != 0) {
        EEPROMWritelong(0, currentOffset);
      }

      getData = false;
    }
  }
}


//This function will write a 4 byte (32bit) long to the eeprom at
//the specified address to address + 3.
void EEPROMWritelong(int address, long value)
{
  //Decomposition from a long to 4 bytes by using bitshift.
  //One = Most significant -> Four = Least significant byte
  byte four = (value & 0xFF);
  byte three = ((value >> 8) & 0xFF);
  byte two = ((value >> 16) & 0xFF);
  byte one = ((value >> 24) & 0xFF);

  //Write the 4 bytes into the eeprom memory.
  EEPROM.write(address, four);
  EEPROM.write(address + 1, three);
  EEPROM.write(address + 2, two);
  EEPROM.write(address + 3, one);
}

long EEPROMReadlong(long address)
{
  //Read the 4 bytes from the eeprom memory.
  long four = EEPROM.read(address);
  long three = EEPROM.read(address + 1);
  long two = EEPROM.read(address + 2);
  long one = EEPROM.read(address + 3);

  //Return the recomposed long by using bitshift.
  return ((four << 0) & 0xFF) + ((three << 8) & 0xFFFF) + ((two << 16) & 0xFFFFFF) + ((one << 24) & 0xFFFFFFFF);
}

