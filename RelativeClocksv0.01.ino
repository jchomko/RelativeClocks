/*
  DS3231: Real-Time Clock. Alarm simple
  Read more: www.jarzebski.pl/arduino/komponenty/zegar-czasu-rzeczywistego-rtc-DS3231.html
  GIT: https://github.com/jarzebski/Arduino-DS3231
  Web: http://www.jarzebski.pl
  (c) 2014 by Korneliusz Jarzebski
*/

//#include <Wire.h>
#include <DS3231.h>
//#include <TimeLib.h>
#include <SPI.h>
#include <SD.h>


DS3231 clock;
RTCDateTime dt;
long  startTimeUnix;
long  currentUnixTime;

//File root;
File data;
int index;
char buf[40];
boolean getCurrTime;
long filePosition;

float currentOffset;
long offsetTimer;

bool hand1;
bool hand2;



void setup()
{
  Serial.begin(57600);

  // Initialize DS3231
  Serial.println("Initialize DS3231");;

  clock.begin();

  Serial.setTimeout(50);

  startTimeUnix = 1472688000;

  getCurrTime = true;

  // Disarm alarms and clear alarms for this example, because alarms is battery backed.
  // Under normal conditions, the settings should be reset after power and restart microcontroller.
  //  clock.armAlarm1(false);
  //  clock.armAlarm2(false);
  //  clock.clearAlarm1();
  //  clock.clearAlarm2();



  // Set Alarm - Every second.
  // DS3231_EVERY_SECOND is available only on Alarm1.
  // setAlarm1(Date or Day, Hour, Minute, Second, Mode, Armed = true)
  clock.setAlarm1(0, 0, 0, 0, DS3231_EVERY_SECOND);

  dt = clock.getDateTime();
  char * cd = clock.dateFormat("Y-m-d", dt);
  String currDate = String(cd);
  Serial.println(currDate);


  pinMode(5, OUTPUT);
  pinMode(6, OUTPUT);

  pinMode(8, OUTPUT);
  pinMode(9, OUTPUT);

  if (!SD.begin(4)) {
    Serial.println("initialization failed!");
    return;
  }
  Serial.println("initialization done.");

  //root = SD.open("/");


  data = SD.open("DATA.TXT");

  if (data) {
    Serial.println("data.txt:");

    // read from the file until there's nothing else in it:
    while (data.available()) {

      char c = data.read();
      Serial.write(c);
      if (c == '\n') {
        break;
      }
    }

  } else {
    // if the file didn't open, print an error:
    Serial.println("error opening data.txt");
  }

  Serial.println("done!");
  String dataDate = "";

  if (data) {

    while (dataDate != currDate) {

      while (data.available()) {

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

          //Serial.print(buf);
          Serial.println(dataDate);
          Serial.println(offset);
          index = 0;
          break;

        }

      }

    }

    Serial.println("match found");
    Serial.println(dataDate);
    Serial.println(currDate);
    Serial.println(currentOffset);
    filePosition = data.position();
    Serial.println(filePosition);
  }
}



void loop()
{


  //Set time via Serial
  if (Serial.available() > 0) {
    //This works well if timeout is set properly
    String s =  Serial.readStringUntil('\n');

    if (s.charAt(0) == 'R') {

      dt = clock.getDateTime();
      Serial.println(clock.dateFormat("d-m-Y H:i:s - l", dt));

    } else if (s.charAt(0) == 'S') {


      String y = s.substring(1, 5);
      String m = s.substring(5, 7);
      String d = s.substring(7, 9);
      String hr = s.substring(9, 11);
      String mn = s.substring(11, 13);
      String ss = s.substring(13, 15);
      String out = y + m + d + hr + mn + ss;

      Serial.print("read: ");
      Serial.println(out);

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
      

      //set clock alarm to go off every morning at 3 am
      //clock.setDateTime(DateTime(iy,1,1,ihr,imn,iss));

    }
  }


  //Check alarms
  if (clock.isAlarm1())
  {
    //Serial.println("ALARM 1 TRIGGERED!");

    dt = clock.getDateTime();
    //Something is wrong here -
    char * cut = clock.dateFormat("U", dt);
    currentUnixTime = cut[0] | (cut[1] << 8) | (cut[2] << 16) | (cut[3] << 24);

    char * cd = clock.dateFormat("Y-m-d", dt);
    String currDate = String(cd);
    Serial.println(currDate);

    //    if(getCurrTime == true){
    //        startTimeUnix = currentUnixTime;
    //        getCurrTime = false;
    //     }
    //     currentUnixTime = abs(currentUnixTime);

    Serial.println(currentUnixTime);
    Serial.print("timeDiff: ");
    
    long diff =  startTimeUnix - currentUnixTime;
    
    Serial.println(diff);
    
    //Serial.println(clock.dateFormat("d-m-Y H:i:s - l", dt));
    
    offsetTimer = millis();

   
    digitalWrite(5, HIGH);
    digitalWrite(8, LOW);
    
    hand1 = true;
    hand2 = true;
  }


  if(millis()-offsetTimer > 50 && hand1 == true){
    
    
    hand1 = false;
    digitalWrite(5, LOW);  
    digitalWrite(8, HIGH);
    
    
  }

  if(millis()-offsetTimer > currentOffset && hand2 == true ){
      hand2 = false;
      digitalWrite(6, HIGH);
      digitalWrite(9, LOW);
    
  }

  if(millis()-offsetTimer > currentOffset +50 && hand2 == false){

    
     digitalWrite(6, LOW);
     digitalWrite(9, HIGH);
    
  }
  
}
