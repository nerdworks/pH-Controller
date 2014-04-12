/*
 
  The circuit:
 * 5V to Arduino 5V pin
 * GND to Arduino GND pin
 * CLK to Analog #5
 * DAT to Analog #4
*/

// include the libraries:
#include <Wire.h>
#include <LiquidTWI.h>
#include "RTClib.h"
#include<stdlib.h>
#include <SoftwareSerial.h>
#include <EEPROM.h>

// Define hardware pins
#define rxpin 2                                       //set the RX pin to pin 2 for softwareserial
#define txpin 3                                       //set the TX pin to pin 3 for softwareserial
//#define rssipin ?                                   // Read RSSI signal from pin 6 on Xbee module, and display bars on LCD. See chapter 6 in datasheet
//#define associatepin ?                              // Read associate pin 15 on Xbee module, and display associate status on LCD

#define TIMER_INTERVAL_ROTATE_DEGREE 100              // msec interval for rotation of degree char
#define TIMER_INTERVAL_PH_POLL 400                    // msec interval for reading the pH probe. Cannot be lower than about 400 ms

// Signalstyrke-tegn


// RX-tegn (roterende så lenge radio er aktiv
  
int pHSet = 650;                                      // Legg inn settpunkt 100 ghanger større enn ønsket pH.

SoftwareSerial myserial(rxpin, txpin);                //enable the soft serial port

// 4 x obsolete lines
//String inputstring = "";                             //a string to hold incoming data from the PC
//String sensorstring = "";                            //a string to hold the data from the Atlas Scientific product
//boolean input_stringcomplete = false;                //have we received all the data from the PC
//boolean sensor_stringcomplete = false;               //have we received all the data from the Atlas Scientific product

// pH probe stuff
char ph_data[20];                  //we make a 20 byte character array to hold incoming data from the pH. 
char computerdata[20];             //we make a 20 byte character array to hold incoming data from a pc/mac/other. 
byte received_from_computer=0;     //we need to know how many characters have been received.                                 
byte received_from_sensor=0;       //we need to know how many characters have been received.
byte arduino_only=1;               //if you would like to operate the pH Circuit with the Arduino only and not use the serial monitor to send it commands set this to 1. The data will still come out on the serial monitor, so you can see it working.  
byte startup=0;                    //used to make sure the Arduino takes over control of the pH Circuit properly.
float ph=0;                        //used to hold a floating point number that is the pH. 
byte string_received=0;            //used to identify when we have received a string from the pH circuit.
// End pH probe stuff

RTC_DS1307 RTC;

// Connect via i2c, default address #0 (A0-A2 not jumpered)
LiquidTWI lcd(0);

byte second;
String checkProbeString = "check probe";
String alarm = "";

static int nowYear;
static int nowMonth;
static int nowDay;
static int nowHour;
static int nowMinute;
static int nowSecond;




static unsigned long lastMillis = 0;

// For rotateDegree
static long timer_rotateDegree = 0;                            // Must be a signed number
int rotateChar = 0;
// For pHPoll
static long timer_pHPoll = 0;
int pHPoll = 0;

//////////////////////////////////////////////////////
void setup(){
  
  
  lcd.begin(16, 4);                                           // set up the LCD's number of rows and columns
  
  Serial.begin(57600);
  Wire.begin();
  RTC.begin();
  if (! RTC.isrunning()) {
    Serial.println("RTC is NOT running!");
    // following line sets the RTC to the date & time this sketch was compiled
    //RTC.adjust(DateTime(__DATE__, __TIME__));
  }
  
  myserial.begin(38400);                                                    //set baud rate for software serial port to 38400
//  inputstring.reserve(5);                                                   //set aside some bytes for receiving data from the PC
//  sensorstring.reserve(30);                                                 //set aside some bytes for receiving data from Atlas Scientific product
  defaultScreen();                                                          // Draw default setup screen
  timer_rotateDegree = TIMER_INTERVAL_ROTATE_DEGREE;
  timer_pHPoll = TIMER_INTERVAL_PH_POLL;
  lastMillis = millis();                                                    // The very last thing to do in etup
  
} //End Setup

///////////////////////////////////////////////////
void loop() {
///////////////////////////////////////////////////

  // General timer stuff
  unsigned long deltaMillis = 0;        // Clear last result
  unsigned long thisMillis = millis();  // do this just once to prevent getting different answers from multiple calls to millis()
  if (thisMillis != lastMillis) {       // we have ticked over
  deltaMillis = thisMillis-lastMillis;  // Calculate how many millis have gone past. Note this works even if millis() has rolled over back to 0
  lastMillis = thisMillis;
 } // End timer stuff



  // pH probe stuff
  if(myserial.available() > 0){Arduino_Control(); Serial.println("test");} //if we see that the pH Circuit has sent a character.
  
  
  // Poll pH
  timer_pHPoll -= deltaMillis;
  Serial.print("timer_pHPoll ");
  Serial.println(timer_pHPoll);
    if (timer_pHPoll <= 0) {
    // reset timer since this is a repeating timer
      timer_pHPoll += TIMER_INTERVAL_PH_POLL; // note this prevents the delay accumulating if we miss a mS or two 
     myserial.print("R\r");             //send it the command to take a single reading.
     if(string_received==1){            //did we get data back from the ph Circuit?
       ph=atof(ph_data);                
       if(ph>=7.5){Serial.println("high\r");} 
       if(ph<7.5){Serial.println("low\r");}   
       string_received=0;}             //reset the string received flag.
    } // End pH poll timer
  
  
  /*TODO
  
  Når check probe eller annen alarm, f.eks temp probe, clear display og blink display  
  
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
 
 // 2. Se om det er gått lang nok tid til at ting skal oppdateres (1000 ms). Hvis så, oppdater det som skal oppdateres på 1000 ms
 //      2.1 Avles klokke
 //      2.2 Avles temp og send denne til pH dings hvis endringen fra forrige gang er stor nok f.eks 1 grad. Hvis probefeil, gå til alarmfunksjon med alarmtype, og bli der inntil reset.
 //      2.3 Avles pH verdi. Hvis probefeil, gå til alarmfunksjon og bli der inntil reset. Limp home modus for pH fra lagrede verdier.
 //      2.4 Finn ut om pH endrer seg med f.eks mer enn 1 på 10 sekunder = Alarm.
 //      2.5 Osv
 
 
 
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
  
 /* 
    if (input_stringcomplete){                                                 //if a string from the PC has been recived in its entierty 
      myserial.print(inputstring);                                             //send that string to the Atlas Scientific product
      inputstring = "";                                                        //clear the string:
      input_stringcomplete = false;                                            //reset the flage used to tell if we have recived a completed string from the PC
      }
 

  while (myserial.available()) {                                               //while a char is holding in the serial buffer
         char inchar = (char)myserial.read();                                  //get the new char and add it to the sensorString
         if (inchar == '\r') {sensor_stringcomplete = true;return;}            //if the incoming character is a <CR>, set the flag
         sensorstring += inchar;
         if (sensorstring == checkProbeString) {
           // Husk at check probe kanskje kommer kun en gang. Gi også alarm når pH endrer seg med mer enn f.eks 2 på kort tid. Sørg for at alarm ikke kommer bort...
           alarm = "ALARM check probe";
          // Legg inn en else her for å lagre som float 
          // gange denne med hundre og lagre som int
           }
         } // End while myserial.available


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
       sensor_stringcomplete = false;                                          //reset the flag used to tell if we have received a completed string from the Atlas Scientific product
      }
*/
DateTime now = RTC.now();
Serial.print(now.year(), DEC); Serial.print('/');
if (now.month() < 10){
   Serial.print("0");
  }
Serial.print(now.month(), DEC); Serial.print('/');
if (now.day() < 10){
   Serial.print("0");
  }
Serial.print(now.day(), DEC); Serial.print(' ');
if (now.hour() < 10){
   Serial.print("0");
  }
Serial.print(now.hour(), DEC); Serial.print(':');
if (now.minute() < 10){
   Serial.print("0");
  }
Serial.print(now.minute(), DEC); Serial.print(':');
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


lcd.setCursor(4,0);
lcd.print(ph_data);
//sensorstring = ""; 

// Rotate degree char
timer_rotateDegree -= deltaMillis;
Serial.print("timer_rotateDegree ");
Serial.println(timer_rotateDegree);
  if (timer_rotateDegree <= 0) {
    // reset timer since this is a repeating timer
    timer_rotateDegree += TIMER_INTERVAL_ROTATE_DEGREE; // note this prevents the delay accumulating if we miss a mS or two 
    // if we want exactly 1000 delay to next time even if this one was late then just use timeOut = 1000;
    // do time out stuff here
    rotateDegree(rotateChar);
    rotateChar++;
    if (rotateChar == 7) rotateChar = 0;
 } // End rotate char



delay(10);

} // End loop

/////////////////////////////////////////////////////////
//     Functions
/////////////////////////////////////////////////////////

void readRTC(){
  DateTime now = RTC.now();                                                   // Finn og sett tiden i diverse statiske variabler
  nowYear = now.year();
  nowMonth = now.month();
  nowDay = now.day();
  nowHour = now.hour();
  nowMinute = now.minute();
  nowSecond = now.second(); 
  } // End readRTC
  
/////////////////////////////////////////////////////////
 void serialEvent(){               //this interrupt will trigger when the data coming from the serial monitor(pc/mac/other) is received.    
        //if(arduino_only!=1){       //if Arduino_only does not equal 1 this function will be bypassed.  
           received_from_computer=Serial.readBytesUntil(13,computerdata,20); //we read the data sent from the serial monitor(pc/mac/other) until we see a <CR>. We also count how many characters have been received.      
           computerdata[received_from_computer]=0; //we add a 0 to the spot in the array just after the last character we received.. This will stop us from transmitting incorrect data that may have been left in the buffer. 
           myserial.print(computerdata);           //we transmit the data received from the serial monitor(pc/mac/other) through the soft serial port to the pH Circuit. 
           myserial.print('\r');                   //all data sent to the pH Circuit must end with a <CR>.  
       //   }    
        }
  
/////////////////////////////////////////////////////////
void defaultScreen(){
     /* Draw default screen
     pH_??.??__Temp_??.?o    Eks: pH  6.45  Temp 26.7grader    pH legges inn på 4,0;      Temp legges inn på 14,0;       Grader legges inn på 18,0
     Set_?.??_NxtCal_????    Eks: Set 6.50 NxtCal 0914         Set pH legges inn på 4,1;  NxtCal legges inn på 16,1
     Min_?.??_@??:??_St_^    Eks: Min 6.36 @ 02:34 St ^        Min legges inn på 4,2;     Min Klokke legges inn på 10,2; St tegn legges inn på 19,2 
     Max_?.??_@??:??_RX_^    Eks: Max 6.53 @ 17:21 RX ^        Max legges inn på 4,3;     Max klokke legges inn på 10,3; RX tegn legges inn på 19,3
     
     Ruller en manglende pixel i gradtegn. Se rotateDegree funksjon
     */   
  byte degree[8] = {B11100,B10100,B11100,B0,B0,B0,B0,B0};
  lcd.createChar(0, degree);
  lcd.setBacklight(HIGH); // Only HIGH and LOW available :-(
  lcd.setCursor(0,0);  lcd.print("pH");   lcd.setCursor(9,0); lcd.print("Temp");    lcd.setCursor(18,0); lcd.write(byte(0)); lcd.setCursor(19,0); lcd.print("C"); 
  lcd.setCursor(0,1);  lcd.print("Set");  lcd.setCursor(9,1); lcd.print("NxtCal");  
  lcd.setCursor(0,2);  lcd.print("Min");  lcd.setCursor(9,2); lcd.print("@");       lcd.setCursor(16,2); lcd.print("St");
  lcd.setCursor(0,3);  lcd.print("Max");  lcd.setCursor(9,3); lcd.print("@");       lcd.setCursor(16,3); lcd.print("RX");
  } // End defaultScreen
     
////////////////////////////////////////////////////////
void alarmScreen(String alarm){
     // Draw alarm screen
     /*
     Blink med baklys!
     Men ikke gå ut av denne funksjonen før reset!
     */
  lcd.setBacklight(HIGH); // Only HIGH and LOW available :-(
  lcd.setCursor(4,0);  lcd.print("ALARM");   
  lcd.setCursor(1,2);  lcd.print(alarm);  
  } // End alarmScreen
  
////////////////////////////////////////////////////////  
void rotateDegree(int i){
  // Define some special characters
  // Roterende temp grader-tegn. Bruke createChar(). det er nok. Behøver ikke å bruke write mer enn en gang ;-)
  byte degR[8][8] = {
      {B1100,B10100,B11100,B0,B0,B0,B0,B0},
      {B10100,B10100,B11100,B0,B0,B0,B0,B0},
      {B11000,B10100,B11100,B0,B0,B0,B0,B0},
      {B11100,B10000,B11100,B0,B0,B0,B0,B0},
      {B11100,B10100,B11000,B0,B0,B0,B0,B0},
      {B11100,B10100,B10100,B0,B0,B0,B0,B0},
      {B11100,B10100,B1100,B0,B0,B0,B0,B0},
      {B11100,B100,B11100,B0,B0,B0,B0,B0},
  };
  lcd.createChar(0, degR[i]);
} // End rotateDegree

///////////////////////////////////////////////////////
void Arduino_Control(){
  
     received_from_sensor=myserial.readBytesUntil(13,ph_data,20); //we read the data sent from pH Circuit until we see a <CR>. We also count how many character have been received.  
     ph_data[received_from_sensor]=0;  //we add a 0 to the spot in the array just after the last character we received. This will stop us from transmitting incorrect data that may have been left in the buffer. 
     string_received=1;                //a flag used when the Arduino is controlling the pH Circuit to let us know that a complete string has been received.
     Serial.println(ph_data);          //lets transmit that data received from the pH Circuit to the serial monitor.
  
      if(startup==0){                //if the Arduino just booted up, we need to set some things up first.   
          myserial.print("e\r");     //take the pH Circuit out of continues mode. 
          delay(50);                 //on start up sometimes the first command is missed. 
          myserial.print("e\r");     //so, let’s send it twice.
          delay(50);                 //a short delay after the pH Circuit was taken out of continues mode is used to make sure we don’t over load it with commands.
          startup=1;                 //startup is completed, let's not do this again during normal operation. 
      }
//  delay(500);                        //we will take a reading ever xxx ms. You can make this much longer or shorter if you like.

} // End Arduino_Control
     

