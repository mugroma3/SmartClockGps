/*
 * GPS-SmartClock ™
 * 
 * SmartClock using GPS to synchronize time with an atomic UTC timestamp
 * This project uses a bluetooth GPS module together with an HC-05 bluetooth module
 * to stream the GPS data over serial to the Arduino.
 * 
 * Licensed under GNU General Public License
 * 
 * by Vincenzo d'Orso e John D'Orazio
 * 
 * A project of the Microcontrollers Users Group - Roma Tre University
 * http://muglab.uniroma3.it/
*/

/*
 *
 * RMC - NMEA has its own version of essential gps pvt (position, velocity, time) data. It is called RMC, The Recommended Minimum, which will look similar to:
 * $GPRMC,123519,A,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W*6A
 * Where:
     RMC          Recommended Minimum sentence C
     123519       Fix taken at 12:35:19 UTC
     A            Status A=active or V=Void.
     4807.038,N   Latitude 48 deg 07.038' N
     01131.000,E  Longitude 11 deg 31.000' E
     022.4        Speed over the ground in knots
     084.4        Track angle in degrees True
     230394       Date - 23rd of March 1994
     003.1,W      Magnetic Variation
     *6A          The checksum data, always begins with *
 * Note that, as of the 2.3 release of NMEA, there is a new field in the RMC sentence at the end just prior to the checksum.
 * This field is a mode indicator to several sentences, which is used to indicate the kind of fix the receiver currently has. 
 * This indication is part of the signal integrity information needed by the FAA. 
 * The value can be A=autonomous, D=differential, E=Estimated, N=not valid, S=Simulator. 
 * Sometimes there can be a null value as well. 
 * Only the A and D values will correspond to an Active and reliable Sentence. 
 * This mode character has been added to the RMC, RMB, VTG, and GLL, sentences and optionally some others including the BWC and XTE sentences. 
 * INFORMATION FROM http://www.gpsinformation.org/dale/nmea.htm
*/


#include <SoftwareSerial.h>
#include <LiquidCrystal.h>

/********************************************INIZIALIZATION*******************************************/

#define ENG 0
#define ITA 1
#define ESP 2
#define FRA 3
#define DEU 4

//initialize buttons
int buttonUtcAugment = 6;
int buttonUtcReduce = 7;
int buttonAugmentState = 0;
int buttonReduceState = 0;
boolean BUTTON_AUGMENT_PRESSED = false;
boolean BUTTON_REDUCE_PRESSED = false;
int oldtime; //useful for filtering button press
int newtime; //useful for filtering button press

// initialize the display library with the numbers of the interface pins
int rs = 11;
int en = 10;
LiquidCrystal lcd(rs, en, 5, 4, 3, 2);

// initialize the bluetooth serial data from HC-05
SoftwareSerial BTSerial(13, 12); // RX | TX

// define some global variables
String inputString = "";                // a string to hold incoming data
String GPSCommandString = "$GPRMC";     // the GPS command string that we are looking for
int offsetUTC = 0;                      // until we can implement an automatic timezone correction based on coordinates, we will assume UTC+2 timezone (Europe/Rome)

//String languages[5] = {"English","Italiano","Español","Français","Deutch"}; //perhaps implement button for changing languages

int currentLocale = ITA; // we will be displaying our strings in Italian for our own test phase, can be changed to another european locale (EN, IT, ES, FR, DE)


/********************************************************SETUP***************************************/

void setup() {
  // put your setup code here, to run once:
   BTSerial.begin(38400);
   Serial.begin(38400);
   lcd.begin(16, 2);
   inputString.reserve(300);
   oldtime = millis(); //useful for filtering button presses
   pinMode(buttonUtcAugment, INPUT);
   pinMode(buttonUtcReduce, INPUT);
}

/********************************************************LOOP***************************************/

void loop() {
 newtime = millis(); //useful for filtering button press
 if(BTSerial.available()>0){
    inputString = BTSerial.readStringUntil('\n');
    if(inputString.startsWith(GPSCommandString)){
      elaborateValues(inputString);
    }
 }
 pressButton();
}

/***********************************************METHODS********************************************/


//TODO: double check whether time / date data is correct even when Fix Status = V (void)
//TODO: double check also whether time / date data is valid when signal integrity != A and !=D

boolean elaborateValues(String myString){

  int hours,minutes,seconds,day,month,year;  
  String hourString,minuteString,secondString,dayString,monthString,yearString,timeString,dateString;
  
  //The GPS Unit that we are using for our testing uses NMEA 2.3, so we have eleven commas instead of just ten
  //TODO: turn this into a for loop
  int idxFirstComma = myString.indexOf(',');
  int idxSecondComma = myString.indexOf(',', idxFirstComma+1);
  int idxThirdComma = myString.indexOf(',', idxSecondComma+1);
  int idxFourthComma = myString.indexOf(',', idxThirdComma+1);
  int idxFifthComma = myString.indexOf(',', idxFourthComma+1);
  int idxSixthComma = myString.indexOf(',', idxFifthComma+1);
  int idxSeventhComma = myString.indexOf(',', idxSixthComma+1);
  int idxEighthComma = myString.indexOf(',', idxSeventhComma+1);
  int idxNinthComma = myString.indexOf(',', idxEighthComma+1);
  int idxTenthComma = myString.indexOf(',', idxNinthComma+1);
  int idxEleventhComma = myString.indexOf(',', idxTenthComma+1);
  int idxTwelfthComma = myString.indexOf(',', idxEleventhComma+1);

  String valueArray[13];
//  valueArray[0] = myString.substring(0,idxFirstComma);                          //GPS Command (in this case, $GPRMC)
  valueArray[1] = myString.substring(idxFirstComma+1, idxSecondComma);          //Time of fix (this is an atomic, precise time!)
/*
  valueArray[2] = myString.substring(idxSecondComma+1, idxThirdComma);          //Status
  valueArray[3] = myString.substring(idxThirdComma+1, idxFourthComma);          //Latitude
  valueArray[4] = myString.substring(idxFourthComma+1, idxFifthComma);          //N
  valueArray[5] = myString.substring(idxFifthComma+1, idxSixthComma);           //Longitude
  valueArray[6] = myString.substring(idxSixthComma+1, idxSeventhComma);         //E
  valueArray[7] = myString.substring(idxSeventhComma+1, idxEighthComma);        //Speed
  valueArray[8] = myString.substring(idxEighthComma+1, idxNinthComma);          //Track angle
*/
  valueArray[9] = myString.substring(idxNinthComma+1, idxTenthComma);           //Date
/*
  valueArray[10] = myString.substring(idxTenthComma+1, idxEleventhComma);       //Magnetic variation
  valueArray[11] = myString.substring(idxEleventhComma+1, idxTwelfthComma);     //Signal integrity
  valueArray[12] = myString.substring(idxTwelfthComma+1);                       //Checksum
*/
  hours = (valueArray[1].substring(0,2).toInt()) + offsetUTC;
  //minutes=(valueArray[1].substring(2,4)).toInt();
  //seconds=(valueArray[1].substring(4,6)).toInt();
  //day = valueArray[9].substring(0,2).toInt();
  month = valueArray[9].substring(2,4).toInt();
  //year = valueArray[9].substring(4,6).toInt();
  
  hourString += hours;
  minuteString = valueArray[1].substring(2,4);
  secondString = valueArray[1].substring(4,6);  
  timeString = hourString+":"+minuteString+":"+secondString;

  dayString = valueArray[9].substring(0,2);
  monthString += month;
  yearString = valueArray[9].substring(4,6);
  
  if(currentLocale == ENG){
    dateString = monthString + "/" + dayString + "/20" + yearString;
  }
  else{
    dateString = dayString + "/" + monthString + "/20" + yearString;
  }
 
  //dateString = dayString+"/"+monthString+"/"+yearString; 
  Serial.print(dateString);
  lcd.setCursor(0,0);
  lcd.print(dateString);
  lcd.setCursor(0,1);
  lcd.print(timeString);
  return true; //perhaps if fix not valid, or signal integrity not A or D, return false?
}

/***********************BUTTONS**************/

void pressButton(){
  buttonAugmentState = digitalRead(buttonUtcAugment);
  buttonReduceState = digitalRead(buttonUtcReduce);

    // CHECK STATE OF COMMAND BUTTON
  if(buttonAugmentState == HIGH && BUTTON_AUGMENT_PRESSED == false && (newtime-oldtime) > 1000){
    BUTTON_AUGMENT_PRESSED = true;
    oldtime = newtime;
  }
  else{
    BUTTON_AUGMENT_PRESSED = false;
  }
 
  // if the pushbutton utc+ is pressed it will augment the utc  
  if(BUTTON_AUGMENT_PRESSED){
    offsetUTC=offsetUTC+1;
  }

    // CHECK STATE OF COMMAND BUTTON
  if(buttonReduceState == HIGH && BUTTON_REDUCE_PRESSED == false && (newtime-oldtime) > 1000){
    BUTTON_REDUCE_PRESSED = true;
    oldtime = newtime;
  }
  else{
    BUTTON_REDUCE_PRESSED = false;
  }
  
  // if the pushbutton utc- is pressed it will reduce the utc
  if (BUTTON_REDUCE_PRESSED) {
    offsetUTC=offsetUTC-1;
  }
}

