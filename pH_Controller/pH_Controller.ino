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
#include <EEPROM.h>

// Define hardware pins
//#define rssipin ?                                   // Read RSSI signal from pin 6 on Xbee module, and display bars on LCD. See chapter 6 in datasheet
//#define associatepin ?                              // Read associate pin 15 on Xbee module, and display associate status on LCD

#define TIMER_INTERVAL_ROTATE_DEGREE 100              // msec interval for rotation of degree char
#define TIMER_INTERVAL_PH_POLL 500                    // msec interval for reading the pH probe. Cannot be lower than about 400 ms
#define TIMER_INTERVAL_READ_RTC 10

#define PH_SET_POINT 6.50

// Signalstyrke-tegn


// RX-tegn (roterende så lenge radio er aktiv


char pHSetpoint[5];  
int pHSet = 650;                                      // Legg inn settpunkt 100 ghanger større enn ønsket pH.
// Legges i EPROM og i defaultScreen


// Bli kvitt String
String inputstring = "";                             //a string to hold incoming data from the PC
String sensorstring = "";                            //a string to hold the data from the Atlas Scientific product
boolean input_stringcomplete = false;                //have we received all the data from the PC
boolean sensor_stringcomplete = false;               //have we received all the data from the Atlas Scientific product

float ph = 0;                                       //used to hold a floating point number that is the pH. 
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

static int run = 0; // used in Serial3 read to throw away the first readings due to garbage

static float lastPH = 0;

unsigned long startMillis = 0;

static unsigned long lastMillis = 0;

// For rotateDegree
static long timer_rotateDegree = 0;                            // Must be a signed number
int rotateChar = 0;
// For pHPoll
static long timer_pHPoll = 0;
int pHPoll = 0;
// For readRTC
static long timer_readRTC = 0;
//int readRTC = 0;

//////////////////////////////////////////////////////
void setup(){
  lcd.begin(16, 4);                                           // set up the LCD's number of rows and columns
  Serial.begin(57600);
  Serial3.begin(38400);
  Wire.begin();
  RTC.begin();
  if (! RTC.isrunning()) {
    Serial.println("RTC is NOT running!");
    // following line sets the RTC to the date & time this sketch was compiled
    //RTC.adjust(DateTime(__DATE__, __TIME__));
  }
  
  inputstring.reserve(5);                                                   //set aside some bytes for receiving data from the PC
  sensorstring.reserve(30);                                                 //set aside some bytes for receiving data from Atlas Scientific product
                                                        // Draw default setup screen
  timer_rotateDegree = TIMER_INTERVAL_ROTATE_DEGREE;
  timer_pHPoll = TIMER_INTERVAL_PH_POLL;
  lastMillis = millis();                                                    // The very last thing to do in setup
  startMillis = millis();
  //pHSetpoint = (char)PH_SET_POINT;
  Serial.println("Starting");
  lcd.setCursor(4,0);
  lcd.print("Starting");
  delay(750);
  defaultScreen();    
  
  
} //End Setup

///////////////////////////////////////////////////
void loop() {
///////////////////////////////////////////////////

unsigned long millisBefore = millis();

  // General timer stuff
  unsigned long deltaMillis = 0;        // Clear last result
  unsigned long thisMillis = millis();  // do this just once to prevent getting different answers from multiple calls to millis()
  if (thisMillis != lastMillis) {       // we have ticked over
  deltaMillis = thisMillis-lastMillis;  // Calculate how many millis have gone past. Note this works even if millis() has rolled over back to 0
  lastMillis = thisMillis;
  } // End general timer stuff



 if (input_stringcomplete){            // If we receive incoming from thr PC; handle it...
    Serial3.print(inputstring);
    inputstring = "";
    input_stringcomplete = false;
}

   
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
 
 // 2. Se om det er gått lang nok tid til at ting skal oppdateres. Hvis så, oppdater det som skal oppdateres
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
  
 

   if (sensor_stringcomplete){                                                 //if a string from the Atlas Scientific product has been received in its entirety
        
           char pHArray[30];
           sensorstring.toCharArray(pHArray, sizeof(pHArray));
           float pHLatestF = atof(pHArray);
           //Serial.print("pHLatestF: ");
           Serial.println(pHLatestF, 2);
           //float pHLatestHF = pHLatestF * 100;
           //int pHLatestH = (int) pHLatestHF;
           //Serial.print("pHLatestH: ");
           //Serial.println(pHLatestH, DEC);
           
           // Gjør det som skal gjøres med pH verdi her:
           
        
           
           if (pHLatestF == 0.00) {
             Serial.println(sensorstring);
             // Alarm bør trigges her

           }
           
           if (pHLatestF < lastPH - 0.4 || pHLatestF > lastPH + 0.4) {
             alarmScreen("Probe Error");
             Serial.println("pH Reading Alarm!");
             Serial.println(sensorstring);
           
           
           lcd.setCursor(4,0);
           lcd.print(pHLatestF);
         }
           sensorstring = "";
           lastPH = pHLatestF;    
       sensor_stringcomplete = false;                                          //reset the flag used to tell if we have received a completed string from the Atlas Scientific product
      }





timer_readRTC -= deltaMillis;
  if (timer_readRTC <= 0) {
    // reset timer since this is a repeating timer
    timer_readRTC += TIMER_INTERVAL_READ_RTC;
    readRTC();
 } // End read RTC



// Rotate degree char
timer_rotateDegree -= deltaMillis;
//Serial.print("timer_rotateDegree ");
//Serial.println(timer_rotateDegree);
  if (timer_rotateDegree <= 0) {
    // reset timer since this is a repeating timer
    timer_rotateDegree += TIMER_INTERVAL_ROTATE_DEGREE; // note this prevents the delay accumulating if we miss a mS or two 
    // if we want exactly 1000 delay to next time even if this one was late then just use timeOut = 1000;
    // do time out stuff here
    rotateDegree(rotateChar);
    rotateChar++;
    if (rotateChar == 7) rotateChar = 0;
  //  Serial.print("Rotated ");
 } // End rotate char




} // End loop////////////////////////////////////////////

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
void serialEvent() { // Incoming from USB
  char inchar = (char)Serial.read();
  inputstring += inchar;
  if(inchar == '\r') {input_stringcomplete = true;}
}

/////////////////////////////////////////////////////////  
void serialEvent3(){ // Incoming from pH sensor 
  char commandLetter;  // the delineator / command chooser
  int iS;
  char numStr[15];      // the number characters and null
  long speed;          // the number stored as a long integer
  char throwed;


  if(Serial3.available() ) { // 3 is the shortest response from the pH sensor
    commandLetter = Serial3.peek();
    if ( ! isalpha( commandLetter )) { // dump until a letter is found or nothing remains
      while( ! isprint(Serial3.peek()) && Serial3.available()){
        throwed = Serial3.read(); // throw out the letter
        Serial.print("Trowed a letter: ");
        Serial.println(throwed, DEC);
      }
      
      if ( Serial3.available() < 2 ) { //not enough letters left, quit
        Serial.println("Not enough letters left");
        return;
      }
    }
    
    // What remains now is something we want. Read the characters from the buffer into a character array
    for( iS = 0; iS < 14; ++iS ) {
      char n = Serial3.read();
      if(n == '\r') {sensor_stringcomplete = true; numStr[iS] = '\0'; return;}
      numStr[iS] = n;
      Serial.print("Read: ");
      Serial.println(numStr[iS]);
    }
    
    
    //terminate the string with a null prevents atol reading too far
    numStr[iS] = '\0';
    
    //read character array until non-number, convert to long int
    //speed = atol(numStr);
    Serial.print("speed: ");
    Serial.println(numStr);
    
  
  }
  
    
 // char inchar = (char)Serial3.read();
 // if(inchar == '\r') {sensor_stringcomplete = true; return;}
 // sensorstring += inchar;
}  
  
/////////////////////////////////////////////////////////
 void defaultScreen(){
    // Draw default screen
    // pH_??.??__Temp_??.?o    Eks: pH  6.45  Temp 26.7grader    pH legges inn på 4,0;      Temp legges inn på 14,0;       Grader legges inn på 18,0
    // Set_?.??_NxtCal_????    Eks: Set 6.50 NxtCal 0914         Set pH legges inn på 4,1;  NxtCal legges inn på 16,1
    // Min_?.??_@??:??_St_^    Eks: Min 6.36 @ 02:34 St ^        Min legges inn på 4,2;     Min Klokke legges inn på 10,2; St tegn legges inn på 19,2 
    // Max_?.??_@??:??_RX_^    Eks: Max 6.53 @ 17:21 RX ^        Max legges inn på 4,3;     Max klokke legges inn på 10,3; RX tegn legges inn på 19,3
     
    // Ruller en manglende pixel i gradtegn. Se rotateDegree funksjon
       
  byte degree[8] = {B11100,B10100,B11100,B0,B0,B0,B0,B0};
  lcd.createChar(0, degree);
  lcd.setBacklight(HIGH); // Only HIGH and LOW available :-(
  lcd.setCursor(0,0);  lcd.print("pH");   lcd.setCursor(9,0); lcd.print("Temp");    lcd.setCursor(18,0); lcd.write(byte(0)); lcd.setCursor(19,0); lcd.print("C"); 
  lcd.setCursor(0,1);  lcd.print("Set");  lcd.setCursor(4,1); lcd.print(pHSet);     lcd.setCursor(9,1); lcd.print("NxtCal");  
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

     

