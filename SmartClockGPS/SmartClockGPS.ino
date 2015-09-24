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


#include <SoftwareSerial.h>
#include <LiquidCrystal.h>
#include "Timer.h"                     //http://github.com/JChristensen/Timer

#define ENG 0
#define ITA 1
#define ESP 2
#define FRA 3
#define DEU 4


// initialize the display library with the numbers of the interface pins
int RS = 11; //Register Select
int EN = 10; //Enable
LiquidCrystal lcd(RS, EN, 5, 4, 3, 2);

// initialize the bluetooth serial data from HC-05
SoftwareSerial BTSerial(13, 12); // RX | TX

//define user definable globals, can change value, held in RAM memory
int offsetUTC = 2;                      // until we can implement an automatic timezone correction based on coordinates, we will assume UTC+2 timezone (Europe/Rome)
int currentLocale = ENG; // we will be displaying our strings in Italian for our own test phase, can be changed to another european locale (EN, IT, ES, FR, DE)
boolean useLangStrings = false;

// define global variables that can change value, held in RAM memory
String inputString = "";                // a string to hold incoming data

// define static variables, will never change, will be held in Flash memory instead of RAM
static const String GPSCommandString = "$GPRMC";     // the GPS command string that we are looking for

static const String months[5][13] = {{"EN","Jan","Feb","Mar","Apr","May","June","July","Aug","Sept","Oct","Nov","Dec"},
{"IT","Gen", "Feb", "Mar", "Apr", "Mag", "Giu", "Lug", "Ago", "Sett", "Ott", "Nov", "Dic"},
{"ES","Ene", "Feb", "Mar", "Abr", "May", "Jun", "Jul", "Ago", "Sep", "Oct", "Nov", "Dic"},
{"FR","Jan", "Fev", "Mar", "Avr", "Mai", "Juin", "Juil", "Aout", "Sept", "Oct", "Nov", "Dec"},
{"DE","Jan", "Febr", "Marz", "April", "Mai", "Juni", "Juli", "Aug", "Sept", "Okt", "Nov", "Dez"}};

static const String settingsMenu[5][3] = {{"UTC OFFSET","LANGUAGE","DATE VIEW"},
{"OFFSET UTC","LINGUA","VISTA DATA"},
{"OFFSET UTC","IDIOMA","VISTA FECHA"},
{"OFFSET UTC","LANGUE","VUE DATE"},
{"OFFSET UTC","SPRACHE","ANZEIGEN DATUM"}};

static const String utcOffsetValues[27] = {"-12","-11","-10","-9","-8","-7","-6","-5","-4","-3","-2","-1","0","+1","+2","+3","+4","+5","+6","+7","+8","+9","+10","+11","+12","+13","+14"};

static const String languageValues[5] = {"English","Italiano","Espanol","Francais","Deutch"};

static const String dateViewValues[5][2] = {{"String","Number"},
{"Stringa","Numero"},
{"Cadena","Numero"},
{"Chaine","Nombre"},
{"String","Zahl"}};



void setup() {
  // put your setup code here, to run once:
   BTSerial.begin(38400);
   Serial.begin(38400);
   lcd.begin(16, 2);
   inputString.reserve(300);
   Serial.println("We are ready to begin!");
}

void loop() {
 if(BTSerial.available()>0){
    inputString = BTSerial.readStringUntil('\n');
    //Serial.println(inputString);
    if(inputString.startsWith(GPSCommandString)){
      //Serial.println(inputString);
      boolean result = elaborateValues(inputString);
    }
 }
}



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
//TODO: double check whether time / date data is correct even when Fix Status = V (void)
//TODO: double check also whether time / date data is valid when signal integrity != A and !=D

boolean elaborateValues(String myString){
  
  int hours,minutes,seconds,day,month,year;  
  String hourString,minuteString,secondString,dayString,monthString,yearString,timeString,dateString;

  Serial.println(myString);
  //The GPS Unit that we are using for our testing uses NMEA 2.3, so we have eleven commas instead of just ten

  int idxComma[12];
  for(int p=0;p<12;p++){
    if(p==0){ idxComma[p] = myString.indexOf(','); }
    else{ idxComma[p] = myString.indexOf(',', idxComma[p-1]+1); }
  }

  String valueArray[4];  
  valueArray[0] = myString.substring(idxComma[0]+1, idxComma[1]);   //TIME
  valueArray[1] = myString.substring(idxComma[8]+1, idxComma[9]);   //DATE
  valueArray[2] = myString.substring(idxComma[2]+1, idxComma[3]);   //LATITUDE
  valueArray[3] = myString.substring(idxComma[4]+1, idxComma[5]);   //LONGITUDE

/*
  for(q=0;q<13;q++){
    if(q==0){ 
      valueArray[q] = myString.substring(0,idxComma[q]); 
    }
    else if(q>0 && q <12){
      valueArray[q] = myString.substring(idxComma[q-1]+1,idxComma[q]); 
    }
    else if(q==12){
      valueArray[q] = myString.substring(idxComma[q-1]+1);
    }
  }
*/

  hours = valueArray[0].substring(0,2).toInt() + offsetUTC;
  //minutes=(valueArray[1].substring(2,4)).toInt();
  //seconds=(valueArray[1].substring(4,6)).toInt();
  //day = valueArray[9].substring(0,2).toInt();
  month = valueArray[1].substring(2,4).toInt();
  //year = valueArray[9].substring(4,6).toInt();
  
  hourString = String(hours);
  Serial.println(hourString);

  minuteString = valueArray[0].substring(2,4);
  secondString = valueArray[0].substring(4,6);  
  timeString = hourString+":"+minuteString+":"+secondString;

  dayString = valueArray[1].substring(0,2);
  yearString = valueArray[1].substring(4,6);
  
  if(useLangStrings){
    monthString = months[currentLocale][month];
    if(currentLocale == ENG){
      dateString = monthString + " " + dayString + ", 20" + yearString;
    }
    else{
      dateString = dayString + " " + monthString + " 20" + yearString;
    }
  }
  else{
    monthString = valueArray[1].substring(2,4);
    if(currentLocale == ENG){
      dateString = monthString + "/" + dayString + "/20" + yearString;
    }
    else{
      dateString = dayString + "/" + monthString + "/20" + yearString;
    }
  }
  lcd.setCursor(0,0);
  lcd.print(dateString);
  lcd.setCursor(0,1);
  lcd.print(timeString);
  
  return true; //perhaps if fix not valid, or signal integrity not A or D, return false?
}

