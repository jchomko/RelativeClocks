/*
  Relative Clocks Arduino Code
  Jonathan Chomko Sept 2016
*/
//issue around serial - seems to hang if you send multiple commands. might have somethign to do with low memory, probably does. order a bigger arduino


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
float currentOffset;
long offsetTimer;
bool hand1;
bool hand2;
int earthHandAngle;
int spaceHandAngle;

bool getData;
String dataDate = "";
String currDate = "";

int beepPin = 10;

void setup()
{

  Serial.begin(57600);

  Serial.println("Initialize DS3231");;

  clock.begin();

  //Serial.setTimeout(150);

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

  //if(!data){

  data = SD.open("DATA.TXT");

  //}

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

      //IF Set
    } else if (s.charAt(0) == 'S') {


      String y = s.substring(1, 5);
      String m = s.substring(5, 7);
      String d = s.substring(7, 9);
      String hr = s.substring(9, 11);
      String mn = s.substring(11, 13);
      String ss = s.substring(13, 15);
      String out = y + m + d + hr + mn + ss;

      //Serial.print("read: ");
      // Serial.println(out);

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

      Serial.println("started checking data");

    }

    else if (s.charAt(0) == 'C') {
      
      clock.begin();
      //dt = clock.getDateTime();
      char * cd = clock.dateFormat("Y-m-d", dt);
      currDate = String(cd);
      Serial.println(currDate);
      
    }
  }


  //Trigger for Earth Hand
  if (clock.isAlarm1())
  {
    //Serial.write(eartHand);
    //eartHand += 6
    tone(beepPin, 700, 20);
    digitalWrite(5, HIGH);

    //Reset Hand catches
    hand1 = true;
    hand2 = true;

    //Start offset timer
    offsetTimer = millis();

  }


  //Turn off earth LED
  if (millis() - offsetTimer > 50 && hand1 == true) {
    hand1 = false;
    //led
    digitalWrite(5, LOW);
  }

  //Trigger for Space Hand
  if (millis() - offsetTimer > currentOffset && hand2 == true ) {
    hand2 = false;
    //led
    digitalWrite(6, HIGH);
    tone(beepPin, 700, 20);
    //spaceHand += 6
    //Serial.write(spaceHand);
  }

  //Turn off space LED
  if (millis() - offsetTimer > currentOffset + 50 && hand2 == false) {
    digitalWrite(6, LOW);
  }

  //Trigger to get new offset data
  if (clock.isAlarm2()) {

    startDataCheck();

  }

  //delay(10);
  checkData();

}

void startDataCheck() {

//      data.close();
//      
//      clock.begin();
//      dt = clock.getDateTime();
//      char * cd = clock.dateFormat("Y-m-d", dt);
//      currDate = String(cd);
//      Serial.println(currDate);

  //if (!SD.begin(4)) {
  //  Serial.println("initialization failed!");
  // return;
  //  }

  //  getData = true;


  //  dt = clock.getDateTime();
  //  char * cd = clock.dateFormat("Y-m-d", dt);
  //  currDate = String(cd);
  //  Serial.println(currDate);
  //
  //  delay(50);
  //
  //  data = SD.open("DATA.TXT");


 // data.close();

 if (!SD.begin()) {
    Serial.println("initialization failed!");
    // return;
  }

  Serial.println("initialization done.");

  //if(!data){

  data = SD.open("DATA.TXT");
  
  getData = true;

//  data = SD.open("DATA.TXT");
  // currDate = "2016-09-09";



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
          currentOffset = offset.toFloat();

          // Serial.print(buf);
          Serial.println(dataDate);
          //Serial.println(offset);
          index = 0;

        } else {
          // Serial.print(buf);

        }

      } else {
        
        Serial.println("data not available");
        data = SD.open("DATA.TXT");
      }

    } else {

      Serial.println("match found");
      Serial.println(dataDate);
      Serial.println(currDate);
      Serial.println(currentOffset);
      filePosition = data.position();
      getData = false;
      
//      data.close();
    }
    // Serial.println(filePosition);
  } else {
    //Serial.println("getData = false");
  }



}


//void checkData1() {
//
//
//  //  if (!SD.begin(4)) {
//  //    Serial.println("initialization failed!");
//  //    return;
//  //  }
//  //  Serial.println("initialization done.");
//  //
//  //  data = SD.open("DATA.TXT");
//
//  //  if (data) {
//  //    // Serial.println("data.txt:");
//  //
//  //    // read from the file until there's nothing else in it:
//  //    if (data.available()) {
//  //
//  //      char c = data.read();
//  //      //Serial.write(c);
//  //      if (c == '\n') {
//  //        break;
//  //      }
//  //    }
//  //
//  //  } else {
//  //    // if the file didn't open, print an error:
//  //    //Serial.println("error opening data.txt");
//  //  }
//
//  //Serial.println("done!");
//  String dataDate = "";
//
//  if (data) {
//
//    while (dataDate != currDate) {
//
//      while (data.available()) {
//
//        char c = data.read();
//        if (index < 40) {
//          buf[index] = c;
//          index ++;
//        }
//
//        if (c == '\n') {
//          String s = String(buf);
//          dataDate = s.substring(0, 10);
//
//          String offset = s.substring(22, 32);
//          currentOffset = offset.toFloat();
//
//          // Serial.print(buf);
//          Serial.println(dataDate);
//          //Serial.println(offset);
//          index = 0;
//          break;
//
//        }
//
//      }
//
//    }
//
//    Serial.println("match found");
//    Serial.println(dataDate);
//    Serial.println(currDate);
//    Serial.println(currentOffset);
//    filePosition = data.position();
//    // Serial.println(filePosition);
//  }
//
//  data.close();
//
//}
//
//
//
//
//
//
//

