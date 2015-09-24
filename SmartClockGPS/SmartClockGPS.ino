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

#define ENG 0
#define ITA 1
#define ESP 2
#define FRA 3
#define DEU 4


// initialize the display library with the numbers of the interface pins
int rs = 11;
int en = 10;
LiquidCrystal lcd(rs, en, 5, 4, 3, 2);

// initialize the bluetooth serial data from HC-05
SoftwareSerial BTSerial(13, 12); // RX | TX

// define some global variables
String inputString = "";                // a string to hold incoming data
String GPSCommandString = "$GPRMC";     // the GPS command string that we are looking for
int offsetUTC = 2;                      // until we can implement an automatic timezone correction based on coordinates, we will assume UTC+2 timezone (Europe/Rome)

String months[5][13] = {{"EN","January","February","March","April","May","June","July","August","September","October","November","December"},
{"IT","Gennaio", "Febbraio", "Marzo", "Aprile", "può", "Giugno", "Luglio", "Agosto", "Settembre", "Ottobre", "Novembre", "December"},
{"ES","Enero", "Febrero", "Marzo", "Abril", "Mayo", "Junio", "Julio", "Agosto", "Septiembre", "Octubre", "Noviembre", "Diciembre"},
{"FR","Janvier", "Février", "Mars", "Avril", "Mai", "Juin", "Juillet", "Août", "Septembre", "Octobre", "Novembre", "Décembre"},
{"DE","Januar", "Februar", "März", "April", "Mai", "Juni", "Juli", "August", "September", "Oktober", "November", "Dezember"}};

String languages[5] = {"English","Italiano","Español","Français","Deutch"}; //perhaps implement button for changing languages

int currentLocale = ITA; // we will be displaying our strings in Italian for our own test phase, can be changed to another european locale (EN, IT, ES, FR, DE)


void setup() {
  // put your setup code here, to run once:
   BTSerial.begin(38400);
   Serial.begin(38400);
   lcd.begin(16, 2);
   inputString.reserve(300);
}

void loop() {
 if(BTSerial.available()>0){
    inputString = BTSerial.readStringUntil('\n');
    if(inputString.startsWith(GPSCommandString)){
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
/*
  int idxComma[12];
  for(int p=0;p<12;p++){
    if(p==0){ idxComma[p] = myString.indexOf(','); }
    else{ idxComma[p] = myString.indexOf(',', idxComma[p-1]+1); }
  }
*/


  String valueArray[13];
  valueArray[0] = myString.substring(0,idxFirstComma);                          //GPS Command (in this case, $GPRMC)
  valueArray[1] = myString.substring(idxFirstComma+1, idxSecondComma);          //Time of fix (this is an atomic, precise time!)
  valueArray[2] = myString.substring(idxSecondComma+1, idxThirdComma);          //Status
  valueArray[3] = myString.substring(idxThirdComma+1, idxFourthComma);          //Latitude
  valueArray[4] = myString.substring(idxFourthComma+1, idxFifthComma);          //N
  valueArray[5] = myString.substring(idxFifthComma+1, idxSixthComma);           //Longitude
  valueArray[6] = myString.substring(idxSixthComma+1, idxSeventhComma);         //E
  valueArray[7] = myString.substring(idxSeventhComma+1, idxEighthComma);        //Speed
  valueArray[8] = myString.substring(idxEighthComma+1, idxNinthComma);          //Track angle
  valueArray[9] = myString.substring(idxNinthComma+1, idxTenthComma);           //Date
  valueArray[10] = myString.substring(idxTenthComma+1, idxEleventhComma);       //Magnetic variation
  valueArray[11] = myString.substring(idxEleventhComma+1, idxTwelfthComma);     //Signal integrity
  valueArray[12] = myString.substring(idxTwelfthComma+1);                       //Checksum
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
  
  hours = valueArray[1].substring(0,2).toInt() + offsetUTC;
  //minutes=(valueArray[1].substring(2,4)).toInt();
  //seconds=(valueArray[1].substring(4,6)).toInt();
  //day = valueArray[9].substring(0,2).toInt();
  month = valueArray[9].substring(2,4).toInt();
  //year = valueArray[9].substring(4,6).toInt();
  
  hourString = String(hours);
  minuteString = valueArray[1].substring(2,4);
  secondString = valueArray[1].substring(4,6);  
  timeString = hourString+":"+minuteString+":"+secondString;

  dayString = valueArray[9].substring(0,2);
  monthString = months[currentLocale][month];
  yearString = valueArray[9].substring(4,6);
  
  if(currentLocale == ENG){
    dateString = monthString + " " + dayString + ", 20" + yearString;
  }
  else{
    dateString = dayString + " " + monthString + " 20" + yearString;
  }
  
  //Serial.println(valueArray[1]);
  //Serial.println(hourString);
  //Serial.println("\n");
  lcd.setCursor(0,0);
  lcd.print(dateString);
  lcd.setCursor(1, 0);
  lcd.print(hourString);

  return true; //perhaps if fix not valid, or signal integrity not A or D, return false?
}

