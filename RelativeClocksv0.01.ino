
/*
  Relative Clocks Arduino Code
  Jonathan Chomko Sept 2016
*/

#include <EEPROM.h>
#include <DS3231.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include "U8glib.h"

#include <Adafruit_GPS.h>
#include <SoftwareSerial.h>




//Display Variables
U8GLIB_SSD1306_128X64 u8g(U8G_I2C_OPT_NONE | U8G_I2C_OPT_DEV_0); // I2C / TWI


//RTC variables
DS3231 clock;
RTCDateTime dt;

//SD card file reading
File data;
int index;
char buf[40];
long filePosition;

//Hand values
unsigned long currentOffset = 0;
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
int maxAttempts = 1;

String sCurrOffset;
int currSecond;

bool isOffsetNeg;
int secondPad = 0;

//GPS Stuff
//TODO: Move to hardware serial

boolean usingInterrupt = false;


//SoftwareSerial mySerial(5, 4);
//Adafruit_GPS GPS(&mySerial);

Adafruit_GPS GPS(&Serial3);

const byte interruptPin = 2;
volatile byte state = LOW;
long blinkTimer;
bool blinkOff;

#define GPSECHO  false

bool setTimeFromGps = false;


void setup()
{

  Serial.begin(9600);
  Serial.println("Initialize DS3231");;

  Serial.setTimeout(150);

  // Disarm alarms and clear alarms for this example, because alarms is battery backed.
  // Under normal conditions, the settings should be reset after power and restart microcontroller.
  clock.begin();

  Serial.println("after clock begin");

  clock.armAlarm1(false);
  clock.armAlarm2(false);
  clock.clearAlarm1();
  clock.clearAlarm2();

  // Set Alarm - Every second.
  //Trigger every second
  clock.setAlarm1(0, 0, 0, 0, DS3231_EVERY_SECOND);

  //Trigger every day
  clock.setAlarm2(0, 0, 10, DS3231_MATCH_H_M, true);
  Serial.println("after clock setup");

  pinMode(11, OUTPUT);
  pinMode(12, OUTPUT);
  pinMode(7, OUTPUT);

  digitalWrite(7, HIGH);

  if (!SD.begin()) {
    Serial.println("initialization failed!");
    // return;
  } else {
    Serial.println("initialization done.");
  }

  if (data) {
    data = SD.open("DATA.TXT");
  }

  dt = clock.getDateTime();
  char * cd = clock.dateFormat("Y-m-d", dt);
  currDate = String(cd);
  Serial.println(currDate);


  //Display
  u8g.setFont(u8g_font_5x7);
  u8g.setFontPosTop();

  // assign default color value
  if ( u8g.getMode() == U8G_MODE_R3G3B2 ) {
    u8g.setColorIndex(255);     // white
  }
  else if ( u8g.getMode() == U8G_MODE_GRAY2BIT ) {
    u8g.setColorIndex(3);         // max intensity
  }
  else if ( u8g.getMode() == U8G_MODE_BW ) {
    u8g.setColorIndex(1);         // pixel on
  }
  else if ( u8g.getMode() == U8G_MODE_HICOLOR ) {
    u8g.setHiColorByRGB(255, 255, 255);
  }

  sCurrOffset = "";
  earthHandAngle = 0;
  spaceHandAngle = 0;

   startDataCheck();

  //GPS
  GPS.begin(9600);

  GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCONLY);
  GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ);   // 1 Hz update rate
  GPS.sendCommand(PGCMD_ANTENNA);

  useInterrupt(true);

  delay(1000);

  //GPS Interrupt
  pinMode(interruptPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(interruptPin), tick, RISING);
  pinMode(13, OUTPUT);

  state = false;


}

void useInterrupt(boolean v) {
  if (v) {
    // Timer0 is already used for millis() - we'll just interrupt somewhere
    // in the middle and call the "Compare A" function above
    OCR0A = 0xAF;
    TIMSK0 |= _BV(OCIE0A);
    usingInterrupt = true;
  } else {
    // do not call the interrupt function COMPA anymore
    TIMSK0 &= ~_BV(OCIE0A);
    usingInterrupt = false;
  }
}

SIGNAL(TIMER0_COMPA_vect) {
  char c = GPS.read();
  // if you want to debug, this is a good time to do it!
#ifdef UDR0
  if (GPSECHO)
    if (c) UDR0 = c;
  // writing direct to UDR0 is much much faster than Serial.print
  // but only one character can be written at a time.
#endif
}



void tick() {

  state = true;

}





void loop()
{
  //Set time via Serial
  if (Serial.available() > 0) {

    String s =  Serial.readStringUntil('/n');

    //Serial.println(s);

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

      clock.begin();

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

    } else if (s.charAt(0) == 'G') {

      setTimeFromGps = true;

    }

  }

  //Blink LED
  if ( state == true ) { //state == true &&

    blinkTimer = millis() + 30;
    digitalWrite(13, HIGH);

    state = false;
    blinkOff = true;
    
    GPS.parse(GPS.lastNMEA());

    if (GPS.milliseconds == 0 && setTimeFromGps) {

      Serial.print(GPS.hour, DEC); Serial.print(':');
      Serial.print(GPS.minute, DEC); Serial.print(':');
      Serial.print(GPS.seconds + 1, DEC); Serial.print('.');
      Serial.println(GPS.milliseconds);

      Serial.print("Date: ");
      Serial.print(GPS.day, DEC); Serial.print('/');
      Serial.print(GPS.month, DEC); Serial.print("/20");
      Serial.println(GPS.year, DEC);

      int y = 2000 + GPS.year;
      clock.setDateTime(y, GPS.month, GPS.day, GPS.hour, GPS.minute, GPS.seconds);
      clock.setAlarm1(0, 0, 0, 0, DS3231_EVERY_SECOND);

      //Trigger every day
      clock.setAlarm2(0, 0, 10, DS3231_MATCH_H_M, true);
    
      setTimeFromGps = false;
    
    }
  }

  if ( millis() > blinkTimer && blinkOff ) {

    digitalWrite(13, LOW);
    blinkOff = false;
    //state = false;

  }

  //Check for new GPS coordinates
  if (GPS.newNMEAreceived()) {
    // a tricky thing here is if we print the NMEA sentence, or data
    // we end up not listening and catching other sentences!
    // so be very wary if using OUTPUT_ALLDATA and trytng to print out data
    //Serial.println(GPS.lastNMEA());   // this also sets the newNMEAreceived() flag to false

    if (!GPS.parse(GPS.lastNMEA()))   // this also sets the newNMEAreceived() flag to false
      return;  // we can fail to parse a sentence in which case we should just wait for another
  }

  //unsigned long currentMillis = millis();
  unsigned long currentMicros = micros();

  //Trigger for Earth Hand
  if (clock.isAlarm1())
  {

    //Serial.write(eartHand);
    //eartHand += 6

    dt = clock.getDateTime();
    char * cs = clock.dateFormat("s", dt);
    currSecond = String(cs).toInt();

    Serial.println(String(cs));
    //Serial.println(currSecond);

    earthHandAngle = currSecond * 6;

    writeAngles();


    tone(beepPin, 700, 20);
    digitalWrite(11, HIGH);
    digitalWrite(7, HIGH);

    //Reset Hand catches
    hand1 = true;
    hand2 = true;

    offsetTimer = currentMicros;
    digitalWrite(7, LOW);

  }


  //Turn off earth LED
  if (currentMicros - offsetTimer > 30000 && hand1 == true) {
    hand1 = false;
    //led
    digitalWrite(11, LOW);
  }

  //Trigger for Space Hand
  if (currentMicros - offsetTimer > currentOffset && hand2 == true ) {
    
    hand2 = false;
    int cs = currSecond + secondPad;
    
    if (cs > 59) {
      cs = 0;
    }
    
    spaceHandAngle = cs * 6;
    writeAngles();
    
    digitalWrite(12, HIGH);
    tone(beepPin, 700, 20);

  }

  //Turn off space LED
  if (currentMicros - offsetTimer > currentOffset + 30000 && hand2 == false) {
    digitalWrite(12, LOW);
  }

  //Trigger once a day to get new offset data
  if (clock.isAlarm2()) {
    startDataCheck();

    setTimeFromGps;

  }

  checkData();

}

void writeAngles() {

  //Serial2
  //  Serial.write("angles ");
  //  char buff[3];
  //  sprintf(buff, "%.3u", earthHandAngle);
  //  Serial.write(buff);
  //  Serial.write("000");
  //  sprintf(buff, "%.3u", spaceHandAngle);
  //  Serial.write(buff);
  //  Serial.write("\r");

  Serial.print(spaceHandAngle);
  Serial.print(",");
  Serial.println(earthHandAngle);

}

void startDataCheck() {

  dt = clock.getDateTime();
  char * cd = clock.dateFormat("Y-m-d", dt);
  currDate = String(cd);
  Serial.println(currDate);


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

        //add chars to buffer
        if (index < 40) {
          buf[index] = c;
          index ++;
        }

        if (c == '\n') {

          //data buffer into string
          String s = String(buf);

          //get current date
          dataDate = s.substring(0, 10);

          //get offset string for display
          sCurrOffset = s.substring(20, 32);

          //get offset string for processing
          String sCurrOffset = s.substring(20, 32);

          //get index of decimal
          int d = sCurrOffset.indexOf('.');

          //remove decimal point
          sCurrOffset.remove(d, 1);

          //add decmial point 3 places back, same as *1000
          sCurrOffset.setCharAt(d + 3, '.');

          Serial.println(sCurrOffset);
          
          //convert string to lon
          const char * csCurrOffset = sCurrOffset.c_str();

          //find minus sign
          d = sCurrOffset.indexOf('-');

          //if offset is negative
          if(d != -1){
            isOffsetNeg = true;
            //remove minus sign
            sCurrOffset.remove(d,1);
          }else{
            
            isOffsetNeg = false;  
          }

          Serial.println(csCurrOffset);
          
          currentOffset = atol( csCurrOffset );

          Serial.println(currentOffset);

          index = 0;

        }

      } else {

        Serial.println("data not available");
        
        data = SD.open("DATA.TXT");
        attemptCount ++;

        if (attemptCount >= maxAttempts) {

          getData = false;
          currentOffset = EEPROMReadlong(0);
          Serial.print("backup offset: ");
          Serial.println(currentOffset);
          attemptCount = 0;
          
        }

      }

    } else {

      //if current offset is positive
      if (isOffsetNeg == false) {

        //Calculate padding
        if (currentOffset < 1000000) {
          secondPad = 1;
        } else if (currentOffset > 1000000 && currentOffset < 2000000) {
          secondPad = 2;
        }
        //get inverse of offset value
        currentOffset = 1000000 - currentOffset;
      
      //if offset is negative
      } else {
        //turn offset to a positive value
//        currentOffset = currentOffset;
        //secondPad = -1;
      }

      Serial.println("match found");
      Serial.println(dataDate);
      Serial.println(currDate);
      Serial.println(currentOffset);

      if (currentOffset != 0) {
        EEPROMWritelong(0, currentOffset);
      }

      updateScreen();
      getData = false;

    }
  }
}


void updateScreen(void) {

  String offset_s = "";
  String date_s = " ";

  u8g.setFontPosTop();
  u8g.firstPage();

  do {
    // graphic commands to redraw the complete screen should be placed here
    //u8g.setFont(u8g_font_osb21);
    u8g.setFont(u8g_font_unifont);


    date_s = "Date: " + currDate;
    offset_s = "Offset: " + sCurrOffset;

    u8g.drawStr( 0, 16, date_s.c_str());
    u8g.drawStr( 0, 32, offset_s.c_str());

  } while ( u8g.nextPage() );

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

