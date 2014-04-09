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
#include<stdlib.h>
#include <SoftwareSerial.h>                           //add the soft serial libray

#define rxpin 2                                       //set the RX pin to pin 2
#define txpin 3                                       //set the TX pin to pin 3
//#define rssipin ?                                   // Read RSSI signal, and display bars on LCD

int pHSet = 650;                                      // Legg inn settpunkt 100 ghanger større enn ønsket pH.

SoftwareSerial myserial(rxpin, txpin);                //enable the soft serial port

String inputstring = "";                             //a string to hold incoming data from the PC
String sensorstring = "";                            //a string to hold the data from the Atlas Scientific product
boolean input_stringcomplete = false;                //have we received all the data from the PC
boolean sensor_stringcomplete = false;               //have we received all the data from the Atlas Scientific product


RTC_DS1307 RTC;

// Connect via i2c, default address #0 (A0-A2 not jumpered)
LiquidTWI lcd(0);

byte second;
String checkProbeString = "check probe";
static String alarm = "";

static int nowYear;
static int nowMonth;
static int nowDay;
static int nowHour;
static int nowMinute;
static int nowSecond;

static unsigned long currentMillis;
static unsigned long lastCurrentMillis;

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
     currentMillis = millis();                                                 //
     lastCurrentMillis = currentMillis;

  
} //End Setup

   void serialEvent() {                                                         //if the hardware serial port receives a char
               char inchar = (char)Serial.read();                               //get the char we just received
               inputstring += inchar;                                           //add it to the inputString
               if(inchar == '\r') {input_stringcomplete = true;}                //if the incoming character is a <CR>, set the flag
               
     } 

void loop() {
  
  /*TODO
  
  Når check probe eller annen alarm, f.eks temp probe, clear display og blink display
  
  fjern delay()
  
  
  Bruk millis for å bestemme når pH skal leses, display oppdateres, konsoll oppdateres osv
  millis og %-funksjon...
  
  bruk heltall for behandling av pH inntil verdien skal ut til f.eks display osv
  
  */
 
 
 // 1. Se om det noe innkommende å håndtere fra knapper eller serial, og håndter det som er kommet inn
 //        1.1 Still klokka
 //        1.2 Endre pH settpunkt
 //        1.3 Kontrast og lysstyrke
 //        1.4 pH kalibrering (aktiveres via LCD-meny)
 //            1.4.1 Les av pH 4 kalibreringsløsning
 //            1.4.2 Les av pH 10 kalibreringsløsning
 //            1.4.3 Les av pH 7 kalibreringsløsning
 //            1.4.4 Finn avvik i span ved hjelp av avleste verdier, og bruk dette til å avgjøre om probe skal skiftes 
 //            1.4.5 Kalibrer til pH 4
 //            1.4.6 Kalibrer til pH 10
 //            1.4.7 Kalibrer til pH 7
 //        1.5 Reset check pH probe
 //        1.6 reset check temp probe
 //        1.7 Annet
 
 // 2. Se om det er gått lang nok tid til at ting skal oppdateres (1000 ms). Hvis så, oppdater alt
 currentMillis = millis();
 if(currentMillis >= (lastCurrentMillis + 1000)) {
 //      2.1 Avles klokke
 //      2.2 Avles temp og send denne til pH dings hvis endringen fra forrige gang er stor nok f.eks 1 grad. Hvis probefeil, gå til alarmfunksjon med alarmtype, og bli der inntil reset.
 //      2.3 Avles pH verdi. Hvis probefeil, gå til alarmfunksjon og bli der inntil reset. Limp home modus for pH fra lagrede verdier.
 //      2.4 Osv
 
 
 
 // 3. Sett verdier
 //      3.1 Sett pH output v.h.a. PID
 //      3.2 pH Max
 //      3.3 pH Min
 //      3.4 RSSI
 //      3.5 Lagre verdi for hvordan pH skal håndteres i tilfellet probefeil. F.eks % on i løpet av siste 24 timer...? Lagres i eprom og oppdateres kun hvis endrinen er vesentlig.
 
 // 4. Send data
 //     4.1 Send til server/logger over hardware serial
 //     4.2 Oppdater lokalt display
 //         4.2.1 pH verdi
 //         4.2.2 RSSI
 //         4.2.3 PID output, on eller off
 //         4.2.4 Max pH
 //         4.2.5 Min pH
 //         4.2.6 Tidspunkt for max pH
 //         4.2.7 Tidspunkt for min pH
 //         4.2.8 Neste kalibreringsdato
 //         4.2.9 pH settpunkt
 //         4.2.10 Temp
 
 
 lastCurrentMillis = currentMillis;
 } // End 1000 ms loop
 
 
  
    if (input_stringcomplete){                                                 //if a string from the PC has been recived in its entierty 
      myserial.print(inputstring);                                             //send that string to the Atlas Scientific product
      inputstring = "";                                                        //clear the string:
      input_stringcomplete = false;                                            //reset the flage used to tell if we have recived a completed string from the PC
      }
 

  while (myserial.available()) {                                               //while a char is holding in the serial buffer
         char inchar = (char)myserial.read();                                  //get the new char
                                                                               //add it to the sensorString
         if (inchar == '\r') {sensor_stringcomplete = true;return;}            //if the incoming character is a <CR>, set the flag
         sensorstring += inchar;
         
         if (sensorstring == checkProbeString) {
           // Husk at check probe kanskje kommer kun en gang. Gi også alarm når pH endrer seg med mer enn f.eks 2 på kort tid. Sørg for at alarm ikke kommer bort...
           alarm = "ALARM check probe";
        // Legg inn en else her for å lagre som float 
        // gange denne med hundre og lagre som int
        
           
           }
           
         }

//Serial.println(sensorstring);
//Serial.println(checkProbeString);
//Serial.println(alarm);
// Få dette ut på LCD



   if (sensor_stringcomplete){                                                 //if a string from the Atlas Scientific product has been received in its entirety
       Serial.println(sensorstring);                                           //use the hardware serial port to send that data to the PC
           char pHArray[30];
           sensorstring.toCharArray(pHArray, sizeof(pHArray));
           Serial.print("sensorstring: ");
           Serial.println(sensorstring);
           Serial.print("pHLatest Char: ");
           Serial.println(pHArray);
           float pHLatestF = atof(pHArray);
           Serial.print("pHLatestF: ");
           Serial.println(pHLatestF);
           float pHLatestHF = pHLatestF * 100;
           int pHLatestH = (int) pHLatestHF;
           Serial.print("pHLatestH: ");
           Serial.println(pHLatestH, DEC);
       //sensorstring = "";                                                      //clear the string:
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

readRTC();
Serial.println(nowSecond, DEC);

 // set the cursor to column 0, line 1. (note: line 1 is the second row, since counting begins with 0):
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
  
delay(10
);


} // End loop

/////////////////////////////////////////////////////////
//     Functions
/////////////////////////////////////////////////////////

void readRTC(){
  // Finn og sett tiden i diverse statiske variabler
  DateTime now = RTC.now();
  nowYear = now.year();
  nowMonth = now.month();
  nowDay = now.day();
  nowHour = now.hour();
  nowMinute = now.minute();
  nowSecond = now.second(); 
  } // End readRTC

