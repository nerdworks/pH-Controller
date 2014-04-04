/*
 
  The circuit:
 * 5V to Arduino 5V pin
 * GND to Arduino GND pin
 * CLK to Analog #5
 * DAT to Analog #4
*/

// include the library code:
#include <Wire.h>
#include <LiquidTWI.h>
#include "RTClib.h"

#include <SoftwareSerial.h>                           //add the soft serial libray
#define rxpin 2                                       //set the RX pin to pin 2
#define txpin 3                                       //set the TX pin to pin 3

SoftwareSerial myserial(rxpin, txpin);                //enable the soft serial port


String inputstring = "";                             //a string to hold incoming data from the PC
String sensorstring = "";                            //a string to hold the data from the Atlas Scientific product
boolean input_stringcomplete = false;                //have we received all the data from the PC
boolean sensor_stringcomplete = false;               //have we received all the data from the Atlas Scientific product


RTC_DS1307 RTC;

// Connect via i2c, default address #0 (A0-A2 not jumpered)
LiquidTWI lcd(0);

byte second;

void setup() {
  // set up the LCD's number of rows and columns: 
  lcd.begin(16, 4);
  // Print a message to the LCD.
 // lcd.print("hello, world!");
  
Serial.begin(57600);
Wire.begin();
RTC.begin();
if (! RTC.isrunning()) {
Serial.println("RTC is NOT running!");
// following line sets the RTC to the date & time this sketch was compiled
//RTC.adjust(DateTime(__DATE__, __TIME__));
}
  
     myserial.begin(38400);                                                    //set baud rate for software serial port to 38400
     inputstring.reserve(5);                                                   //set aside some bytes for receiving data from the PC
     sensorstring.reserve(30);                                                 //set aside some bytes for receiving data from Atlas Scientific product

  
} //End Setup

   void serialEvent() {                                                         //if the hardware serial port receives a char
               char inchar = (char)Serial.read();                               //get the char we just received
               inputstring += inchar;                                           //add it to the inputString
               if(inchar == '\r') {input_stringcomplete = true;}                //if the incoming character is a <CR>, set the flag
     } 

void loop() {
  
 
  
    if (input_stringcomplete){                                                 //if a string from the PC has been recived in its entierty 
      myserial.print(inputstring);                                             //send that string to the Atlas Scientific product
      inputstring = "";                                                        //clear the string:
      input_stringcomplete = false;                                            //reset the flage used to tell if we have recived a completed string from the PC
      }
 

  while (myserial.available()) {                                               //while a char is holding in the serial buffer
         char inchar = (char)myserial.read();                                  //get the new char
         sensorstring += inchar;                                               //add it to the sensorString
         if (inchar == '\r') {sensor_stringcomplete = true;}                   //if the incoming character is a <CR>, set the flag
         }


   if (sensor_stringcomplete){                                                 //if a string from the Atlas Scientific product has been received in its entirety
       Serial.println(sensorstring);                                           //use the hardware serial port to send that data to the PC
                                                            //clear the string:
       sensor_stringcomplete = false;                                          //reset the flag used to tell if we have received a completed string from the Atlas Scientific product
      }
  
  
 

DateTime now = RTC.now();
Serial.print(now.year(), DEC);
Serial.print('/');
if (now.month() < 10){
   Serial.print("0");
  }
Serial.print(now.month(), DEC);
Serial.print('/');
if (now.day() < 10){
   Serial.print("0");
  }
Serial.print(now.day(), DEC);
Serial.print(' ');
if (now.hour() < 10){
   Serial.print("0");
  }
Serial.print(now.hour(), DEC);
Serial.print(':');
if (now.minute() < 10){
   Serial.print("0");
  }
Serial.print(now.minute(), DEC);
Serial.print(':');
if (now.second() < 10){
   Serial.print("0");
  }
Serial.print(now.second(), DEC);
Serial.println();
//Serial.println(__DATE__);
//Serial.println(__TIME__);
Serial.println();

 // set the cursor to column 0, line 1
  // (note: line 1 is the second row, since counting begins with 0):
 // lcd.setCursor(0, 1);
  // print the number of seconds since reset:
//  lcd.print(millis()/1000);

  lcd.setBacklight(HIGH);
//  delay(500);
//  lcd.setBacklight(LOW);
//  delay(500);
lcd.setCursor(0,0);
lcd.print("pH");

lcd.setCursor(4,0);
lcd.print(sensorstring);
sensorstring = ""; 

lcd.setCursor(0, 1);
if (now.second() != second){
  if (now.second() < 10){
   lcd.print("0");
  }
  lcd.print(now.second());
  }
  second = now.second();
  
delay(1000);


}

