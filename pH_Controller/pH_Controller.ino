

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
#include <stdlib.h>
#include <EEPROMex.h>
#include <EEPROMVar.h>
//#include "Arduino.h"

// Define hardware pins
//#define rssipin ?                                   // Read RSSI signal from pin 6 on Xbee module, and display bars on LCD. See chapter 6 in datasheet
//#define associatepin ?                              // Read associate pin 15 on Xbee module, and display associate status on LCD


// Robust timer. See: http://www.forward.com.au/pfod/ArduinoProgramming/TimingDelaysInArduino.html
#define TIMER_INTERVAL_ROTATE_DEGREE 100              // msec interval for rotation of degree char

#define TIMER_INTERVAL_READ_RTC 10

#define PH_SET_POINT 6.50

// Signalstyrke-tegn


// RX-tegn (roterende så lenge radio er aktiv



float pHSet = PH_SET_POINT;                          // Bruk Eprom til dette

String inputstringUSB = "";                             //a string to hold incoming data from the PC
String inputstringRadio = "";
String sensorstringPH = "";                            //a string to hold the data from the Atlas Scientific product
boolean inputUSB_stringcomplete = false;                //have we received all the data from the PC
boolean inputRadio_stringcomplete = false;
boolean sensor_stringcomplete = false;               //have we received all the data from the Atlas Scientific product
static float lastPH = 0;

String valueString = "";                             // String from USB or radio without the commad char

static float pHMin = 16.0;
static int pHMinHour = 0;
static int pHMinMin = 0;
static float pHMax = 0;
static int pHMaxHour = 0;
static int pHMaxMin = 0;
static boolean skipNextPH = false;
static boolean probeCalibOngoing = false;


// End pH probe stuff

// RTC and timer stuff
RTC_DS1307 RTC;

static int nowYear;
static int nowMonth;
static int nowDay;
static int nowHour;
static int nowMinute;
static int nowSecond;
static String dateTimeNow = "";

unsigned long startMillis = 0;
static unsigned long lastMillis = 0;

static long timer_rotateDegree = 0;                            // Must be a signed number
int rotateChar = 0;

static long timer_readRTC = 0;

LiquidTWI lcd(0);        // Connect via i2c, default address #0 (A0-A2 not jumpered)


////////////////////////////////////////////////////////////////////////////////////////
void setup(){      /////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////
  lcd.begin(16, 4);                                           // set up the LCD's number of rows and columns
  Serial.begin(57600);
  Serial2.begin(38400);
  Wire.begin();
  RTC.begin();
  if (! RTC.isrunning()) {
    Serial.println("RTC is NOT running!");
    // following line sets the RTC to the date & time this sketch was compiled
    RTC.adjust(DateTime(__DATE__, __TIME__));
  }
  
//  Serial.println(__DATE__);
//  Serial.println(__TIME__);
  
  inputstringUSB.reserve(20);                                                   //set aside some bytes for receiving data from the PC
  sensorstringPH.reserve(30);                                                 //set aside some bytes for receiving data from Atlas Scientific product
  timer_rotateDegree = TIMER_INTERVAL_ROTATE_DEGREE;

  Serial.println("Starting");
  lcd.setCursor(4,0);
  lcd.print("Starting");
  delay(750);
  lcd.clear();
  
  

  // EEPROM check
  while (!EEPROM.isReady()) { delay(1); }
  if(EEPROM.readByte(511) != 1) {                // Chek if values have ever been set in EEPROM. If not, set defaults.
    cli();                                       // Always disable intserrupts before writing to EEPROM
    EEPROM.writeFloat(0, PH_SET_POINT);          // If never set, set to default value
    
    EEPROM.writeByte(511, 1);                    // EEPROM in use address = 511. If value is not 1, write defaults for all variables, and 1 at address 511
    sei();                                       // Enable interrupts again
  }

  
  // Read latest values from EEPROM
  pHSet = EEPROM.readFloat(0);                  // pH setpoint. float = 4 bytes. Address = 0-3
 
 
 
  defaultScreen();                              // Draw default screen
  lastMillis = millis();                        // The very last thing to do in setup
  startMillis = millis();

} //End Setup

/////////////////////////////////////////////////////////////////////////////////////
void loop() {      //////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////

  unsigned long millisBefore = millis();

  // General timer stuff
  unsigned long deltaMillis = 0;        // Clear last result
  unsigned long thisMillis = millis();  // do this just once to prevent getting different answers from multiple calls to millis()
  if (thisMillis != lastMillis) {       // we have ticked over
    deltaMillis = thisMillis-lastMillis;  // Calculate how many millis have gone past. Note this works even if millis() has rolled over back to 0
    lastMillis = thisMillis;
  } // End general timer stuff

 if (inputUSB_stringcomplete){ handlePCInput(); }         // If we receive incoming from the PC; handle it...
 if (inputRadio_stringcomplete){ handlePCInput(); }       // If we receive incoming from the PC; handle it...
 if (probeCalibOngoing) { calibratePHSensor; }

   
 // TODO
 
 // 1. Se om det noe innkommende å håndtere fra knapper eller serial, og håndter det som er kommet inn
 //        1.1 Still klokka
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
 //      2.2 Avles temp og send denne til pH dings hvis endringen fra forrige gang er stor nok f.eks 1 grad. Hvis probefeil, gå til alarmfunksjon med alarmtype, og bli der inntil reset.
 //      2.3 Avles pH verdi. Hvis probefeil, gå til alarmfunksjon og bli der inntil reset. Limp home modus for pH fra lagrede verdier.
 
 // 3. Sett verdier
 //      3.1 Sett pH output v.h.a. PID
 //      3.4 RSSI
 //      3.5 Lagre verdi for hvordan pH skal håndteres i tilfellet probefeil. F.eks % on i løpet av siste 24 timer...? Lagres i eprom og oppdateres kun hvis endringen er vesentlig.
 
 // 4. Send data
 //     4.1 Send til server/logger over hardware serial
 //     4.2 Oppdater lokalt display
 //         4.2.2 RSSI
 //         4.2.3 PID output, on eller off
 //         4.2.8 Neste kalibreringsdato
 //         4.2.10 Temp
  
 





  // Handle new reading from pH sensor
  if (sensor_stringcomplete){                                       //if a string from the pH probe has been received in its entirety
    newPH(); 
  } // End handling reading from pH sensor

  // Time to read RTC
  timer_readRTC -= deltaMillis;
  if (timer_readRTC <= 0) {
    timer_readRTC += TIMER_INTERVAL_READ_RTC; // reset timer since this is a repeating timer
    readRTC();
  } // End read RTC

  // Rotate degree char
  timer_rotateDegree -= deltaMillis;
  if (timer_rotateDegree <= 0) {
    // reset timer since this is a repeating timer
    timer_rotateDegree += TIMER_INTERVAL_ROTATE_DEGREE; // note this prevents the delay accumulating if we miss a mS or two 
    // if we want exactly 1000 delay to next time even if this one was late then just use timeOut = 1000;
    // do time out stuff here
    rotateDegree(rotateChar);
    rotateChar++;
    if (rotateChar == 7) rotateChar = 0;
  } // End rotate char


} // End loop//////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////
//     Functions        ///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////

void readRTC(){
  DateTime now = RTC.now();                                                   // Finn og sett tiden i diverse statiske variabler
  nowYear = now.year();
  nowMonth = now.month();
  nowDay = now.day();
  nowHour = now.hour();
  nowMinute = now.minute();
  nowSecond = now.second();
  dateTimeNow = "";
  dateTimeNow += nowYear; dateTimeNow += "/"; if(nowMonth < 10) {dateTimeNow += "0";} dateTimeNow += nowMonth; dateTimeNow += "/";
  if(nowDay < 10) {dateTimeNow += "0";} dateTimeNow += nowDay; dateTimeNow += " "; if(nowHour < 10) {dateTimeNow += "0";} 
  dateTimeNow += nowHour;   dateTimeNow += ":"; if(nowMinute < 10) {dateTimeNow += "0";} dateTimeNow += nowMinute;
  } // End readRTC
  
///////////////////////////////////////////////////////////////////////////////////////////
void serialEvent() { // Incoming from USB
  char inchar = (char)Serial.read();
  inputstringUSB += inchar;
  if(inchar == '\r') {inputUSB_stringcomplete = true;}
}

///////////////////////////////////////////////////////////////////////////////////////////  
void serialEvent2(){ // Incoming from pH sensor 
  char pChar; 
  if(Serial2.available() ) {
    pChar = Serial2.peek();
    if ( ! isalpha( pChar )) { // dump until a letter is found or nothing remains
      while( (! isprint(Serial2.peek())) && Serial2.available()){
        if (pChar == '\r'){
          sensor_stringcomplete = true;
          Serial2.read(); // throw out the letter
          sensorstringPH += '\0';
          return;
        }
        Serial2.read(); // throw out the letter
      }
    }
    char inchar = (char)Serial2.read();    // What remains now is something we want.
    sensorstringPH += inchar;
  } 
}  
  
///////////////////////////////////////////////////////////////////////////////////////////
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
  lcd.setCursor(0,0);  lcd.print("pH");   lcd.setCursor(9,0); lcd.print("Temp");    lcd.setCursor(18,0); lcd.write(byte(0));    lcd.setCursor(19,0); lcd.print("C"); 
  lcd.setCursor(0,1);  lcd.print("Set");  lcd.setCursor(4,1); lcd.print(pHSet);     lcd.setCursor(9,1);  lcd.print("NxtCal");  
  lcd.setCursor(0,2);  lcd.print("Min");  lcd.setCursor(9,2); lcd.print("@");       lcd.setCursor(12,2); lcd.print(":");        lcd.setCursor(16,2); lcd.print("St");
  lcd.setCursor(0,3);  lcd.print("Max");  lcd.setCursor(9,3); lcd.print("@");       lcd.setCursor(12,3); lcd.print(":");        lcd.setCursor(16,3); lcd.print("RX");
  } // End defaultScreen
     
//////////////////////////////////////////////////////////////////////////////////////////
void alarmScreen(String alarm){
    lcd.clear();   
    lcd.setBacklight(HIGH); // Only HIGH and LOW available :-(
    boolean hl = true;
    lcd.setCursor(7,0);  lcd.print("ALARM");   
    lcd.setCursor(2,1);  lcd.print(dateTimeNow);
    lcd.setCursor(2,2);  lcd.print(alarm);
    // Få inn klokkeslett først
    Serial.print("ALARM: ");
    Serial.println(alarm);
    while (true) {
      lcd.setBacklight(LOW);
      delay(500);
      lcd.setBacklight(HIGH);
      delay(500);
    }
  } // End alarmScreen
  
//////////////////////////////////////////////////////////////////////////////////////////
void rotateDegree(int i){
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

/////////////////////////////////////////////////////////////////////////////////////////
void newPH(){
  
  char pHArray[7];
  sensorstringPH.toCharArray(pHArray, sizeof(pHArray));
  float pHLatestF = atof(pHArray);
  //pHLatestF += 8.0;
  Serial.print("pH: ");
  Serial.println(pHLatestF, 2);
  // Gjør det som skal gjøres med pH verdi her:
  if (pHLatestF == 0.00) {                                // this happens if the sensorstringPH cannot be converted to float (garbage on serial comms).
     Serial.print("pH probe Comm Error: ");
     Serial.println(sensorstringPH);
     // Alarm trigges her
     alarmScreen("pH Probe Comm Error");
  }
           
  if (pHLatestF < lastPH - 0.8 || pHLatestF > lastPH + 0.8) {
    if (lastPH < 1) {lastPH = pHLatestF; return;}                                 // Skip first reading in case of garbage and to set lastPh
    if (skipNextPH = true) {lastPH = pHLatestF; skipNextPH = false; return;}    // Skip next reading after sending command to probe!
    alarmScreen("pH Probe Error");
    Serial.print("pH Reading Alarm!: ");
    Serial.println(pHLatestF);
  }
           
  lcd.setCursor(4,0);
  if(pHLatestF > 9.99) {lcd.setCursor(3,0);}
  lcd.print(pHLatestF);
  
  if(nowHour == 0 && nowMinute == 0 && nowSecond == 0) {pHMin = 16.0; pHMax = 0;} // Reset pH max and min at midnight

  if(pHLatestF < pHMin) {
    if(pHLatestF > 9.99) {lcd.setCursor(3,2);} else {lcd.setCursor(4,2);}
    lcd.print(pHLatestF);
    pHMin = pHLatestF; pHMinHour = nowHour; pHMinHour = nowMinute; lcd.setCursor(10,2);
    if(nowHour < 10){lcd.print("0"); lcd.setCursor(11,2); lcd.print(nowHour);} else {lcd.print(nowHour);}
    lcd.setCursor(13,2); 
    if(nowMinute < 10) {lcd.print("0"); lcd.setCursor(14,2); lcd.print(nowMinute);} else {lcd.print(nowMinute);}
  }
           
  if(pHLatestF > pHMax) {
    if(pHLatestF > 9.99) {lcd.setCursor(3,3);} else {lcd.setCursor(4,3);}
    lcd.print(pHLatestF);
    pHMax = pHLatestF; pHMaxHour = nowHour; pHMaxMin = nowMinute; lcd.setCursor(10,3); 
    if(nowHour < 10) {lcd.print("0"); lcd.setCursor(11,3);lcd.print(nowHour);} else {lcd.print(nowHour);}
    lcd.setCursor(13,3);
    if(nowMinute < 10) {lcd.print("0"); lcd.setCursor(14,3); lcd.print(nowMinute);} else {lcd.print(nowMinute);}
  }
           
  sensorstringPH = "";
  lastPH = pHLatestF;    
  sensor_stringcomplete = false;       //reset the flag used to tell if we have received a completed string from the pH probe
} // End newPH

//////////////////////////////////////////////////////////////////////////////////////////
void handlePCInput(){
     
   char commandByte;
   commandByte = inputstringUSB.charAt(0);
   valueString = inputstringUSB.substring(1);
   
   
   Serial.println(commandByte);
   
   switch (commandByte) {
      case 'A' :                          // Set the time and date
       Serial.println(inputstringUSB); break;
     case 'c' :
       Serial.println("Calibrate pH probe"); probeCalibOngoing = true; calibratePHSensor(); skipNextPH = true; break;
     case 'C' :                          // Continous mode fro pH probe
       Serial.println(inputstringUSB); Serial2.print(inputstringUSB); skipNextPH = true; break;
     case 'F' :                          // Calibration at 4
       Serial.println(inputstringUSB); Serial2.print(inputstringUSB); skipNextPH = true; break;
     case 'L' :                          // L0 or L1, switches off or on the debud LED´s on the probe controller
       Serial.println(inputstringUSB); Serial2.print(inputstringUSB); skipNextPH = true; break;
     case 'P' :                          // Angi nytt pH settpunkt. oppdater variabel og update EEPROM   
       Serial.println(inputstringUSB); updatePHSetpoint(valueString); break;  
     case 'R' :                          // Single pH reading
       Serial.println(inputstringUSB); Serial2.print(inputstringUSB); skipNextPH = true; break;
     case 'S' :                          // Calibration at 7
       Serial.println(inputstringUSB); Serial2.print(inputstringUSB); skipNextPH = true; break;
     case 't' :                          // Send new temp to pH probe.      Can be removed after debug period
       Serial.println(valueString); Serial2.print(valueString); skipNextPH = true; break;
     case 'T' :                          // Calibration at 10
       Serial.println(inputstringUSB); Serial2.print(inputstringUSB); skipNextPH = true; break;

       
     default:
       // alarmScreen("USB Input Error");  
       Serial.println("Case -- Default");  
   }
   
   inputstringUSB = "";
   valueString = "";
   inputUSB_stringcomplete = false;     
} // End handlePCInput

//////////////////////////////////////////////////////////////////////////////////////////////////////
void updatePHSetpoint(String newPH) {
  float pHF;
  char pH[7];
  newPH.toCharArray(pH, sizeof(pH));
  pHF = atof(pH);
  pHSet = pHF;
  lcd.setCursor(4,1);
  lcd.print(pHSet);
  while (!EEPROM.isReady()) { delay(1); }
  cli();
  EEPROM.updateFloat(0, pHF);
  sei();
} // End updatePHSetpoint

/////////////////////////////////////////////////////////////////////////////////////////////////////
void calibratePHSensor() {
  Serial.println("pH probe calibration");
  int s = 10;
  int i;
  boolean healthDone = false;
  // Notes:
  // Calibration solution and probe should be at the same temp @ 25 degrees celcius.
  // A deviation of more than 0.2 equals replacement of electrode. Deviation of 0.4 means that end of life is reached, and the elctrode becomes unstable.
  // Deviation is slow in the beginning, and increases with time.
  // Buffer solution has a shelf life of less than 3 years.
  // Recommended probe is "Hamilton Polylite Lab" Hamilton partNo 238403. This probe is for low conductivity fluids.
  // Remember that a S7 to BNC cable is needed. I.e. Hamilton partNo 355176.
  // See: http://www.hamiltoncompany.com/downloads/610277_05_pH_Measurement_Guide_LR_EN.pdf
  // and: http://www.hamiltoncompany.com/downloads/E_610277_04%20with%20Pathfinder.pdf

  // Health cehck info...      
  lcd.clear();
  lcd.setCursor(0,0); lcd.print("Step 1:"); lcd.setCursor(0,1); lcd.print("Probe health check"); lcd.setCursor(0,3); lcd.print("Press enter or esc.");
  delay(5000);      
  lcd.clear();
  lcd.setCursor(0,0); lcd.print("Please have buffers"); lcd.setCursor(0,1); lcd.print("ready (4, 7, 10)."); lcd.setCursor(0,2); lcd.print("Bufferlife = 2.5 yrs"); lcd.setCursor(0,3); lcd.print("Press enter or esc.");
  delay(5000);      
  lcd.clear();
  lcd.setCursor(0,0); lcd.print("Probe and buffers"); lcd.setCursor(0,1); lcd.print("should be 25 degrees"); lcd.setCursor(0,2); lcd.print("Celcius."); lcd.setCursor(0,3); lcd.print("Press enter or esc.");
  delay(5000);
  // Health cehck info... Done


 // Bruk for-sløyfe som går 2 rundewr...
  while (probeCalibOngoing) {


      
      
    lcd.clear();
    lcd.setCursor(0,0); lcd.print("Stir probe slowly in"); lcd.setCursor(0,1); lcd.print("pH 7 buffer,"); lcd.setCursor(0,2); lcd.print("and press enter.");
    delay(5000);
      
      
      
    //Serial2.print("C"); NEI, dette er for kalib
    lcd.setCursor(0,3);
    i= s;
    while ( i >= 0 ) {lcd.setCursor(0,3); lcd.print(i); delay(1000); i -= 1;}
    //Serial2.print("S"); NEI, dette er for kalib
    // Finn deviation
      
    lcd.clear();
    lcd.setCursor(0,0); lcd.print("Done."); lcd.setCursor(0,1); lcd.print("Dry pH probe with"); lcd.setCursor(0,2); lcd.print("paper towel."); lcd.setCursor(0,3); lcd.print("Press enter or esc.");
    delay(5000);
      
    lcd.clear();
    lcd.setCursor(0,0); lcd.print("Stir probe slowly in"); lcd.setCursor(0,1); lcd.print("pH 4 buffer,"); lcd.setCursor(0,2); lcd.print("and press enter.");
    delay(5000);
      
    i = s;
    lcd.setCursor(0,3);
    while ( i >= 0 ) {lcd.setCursor(0,3); lcd.print(i); delay(1000); i -= 1;}
    //Serial2.print("F"); NEI, dette er for kalib
    // Finn deviation
      
      
    lcd.clear();
    lcd.setCursor(0,0); lcd.print("Done."); lcd.setCursor(0,1); lcd.print("Dry pH probe with"); lcd.setCursor(0,2); lcd.print("paper towel."); lcd.setCursor(0,3); lcd.print("Press enter or esc.");
    delay(5000);
      
    lcd.clear();
    lcd.setCursor(0,0); lcd.print("Stir probe slowly in"); lcd.setCursor(0,1); lcd.print("pH 10 buffer,"); lcd.setCursor(0,2); lcd.print("and press enter.");
    delay(5000);
      
    i = s;
    lcd.setCursor(0,3);
    while ( i >= 0 ) {lcd.setCursor(0,3); lcd.print(i); delay(1000); i -= 1;}
    //Serial2.print("T"); NEI, dette er for kalib
    // Finn deviation
    //Serial2.print("E"); NEI, dette er for kalib
    // Delay(100);
    //Serial2.print("C"); NEI, dette er for kalib
    lcd.clear();
    lcd.setCursor(0,0); lcd.print("Probe health check"); lcd.setCursor(0,1); lcd.print("is done."); lcd.setCursor(0,3); lcd.print("Press enter.");
    delay(5000);
      
    // Calibration
      
    lcd.clear();
    lcd.setCursor(0,0); lcd.print("Step 2:");
    lcd.setCursor(0,1); lcd.print("Probe calibration");
    lcd.setCursor(0,3); lcd.print("Press enter.");
    delay(5000);
      
      


    // Check offset and span deviation
    // Check what we measure in a pH 7 solution
    // Check what we measure in a pH 4 solution
    // Check what we measure in a pH 10 solution
    // Give user info about deviation
    // Give user info about time to next calibration
    // Give user advise to order new probe (close to end of life).

    // Calibration:
      
    lcd.clear();
    lcd.setCursor(0,0); lcd.print("Step 2:");
    lcd.setCursor(0,1); lcd.print("Probe calibration");
    lcd.setCursor(0,3); lcd.print("Press enter.");
    delay(5000);

 
    lcd.clear();
    lcd.setCursor(0,0); lcd.print("pH probe calibration");
    lcd.setCursor(0,1); lcd.print("Place probe in pH 7");
    lcd.setCursor(0,2); lcd.print("and press enter.");
    lcd.setCursor(0,3); lcd.print("and press enter.");

/* 
1. Place pH Sensor in pH 7 solution.
2. Instruct the circuit to go into continues mode. (C)
3. Wait 1 to 2 minutes.
4. TX the S command. Your pH Circuit is now calibrated for pH 7.
5. Dry sensor with paper towel.
6. Place pH sensor in pH 4 solution.
7. Wait 1 to 2 minutes.
8. TX the F command. Your pH Circuit is now calibrated for pH4.
9. Dry sensor with paper towel.
10. Place pH sensor in pH 10 solution.
11. Wait 1 to 2 minutes.
12. TX the T command. Your pH Circuit is now calibrated for pH10.
13. Transmit the E command.
Transmit the C command.
14. The pH Circuit is now calibrated.
*/

    // Skip next pH reading
    // Set new calibration date
    // Update display with new calibration date. Store new calibration date in EEPROM
  
    healthDone = true;
    probeCalibOngoing = false;

  } // End while probeCalibOngoing
  
      lcd.clear();
    defaultScreen();                    // Draw default screen
  
  
} // End calibratePHSensor


