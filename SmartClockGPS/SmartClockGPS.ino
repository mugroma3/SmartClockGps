/*
 * GPS-SmartClock ™
 * 
 * SmartClock using GPS to synchronize time with an atomic UTC timestamp
 * This project uses a bluetooth GPS module together with an HC-05 bluetooth module
 * to stream the GPS data over serial to the Arduino.
 * A 16x2 LCD Display Module is used to display the date and time
 * 
 * Licensed under GNU General Public License
 * 
 * by Vincenzo d'Orso (icci87@gmail.com) e John D'Orazio (john.dorazio@cappellaniauniroma3.org)
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

#define MINUTEINMILLIS 60000     // 1 minute (in milliseconds)
#define HOURINMILLIS   3600000   // 1 hour (in milliseconds)
#define DAYINMILLIS    86400000  // 24 hours (in milliseconds)

//TABLES USED FOR CALCULATING DAY OF THE WEEK
//Formula: (d + m + y + [y / 4] + c)mod7
//static const int monthsTable[14] = {0,3,3,6,1,4,6,2,5,0,3,5,6,2}; //Jan~Dec,Jan_Leap_Year,Feb_Leap_Year
//static const int monthsTable[12] = {0,3,2,5,0,3,5,1,4,6,2,4};
//static const int century = 6;


// initialize the display library with the numbers of the interface pins
int RS = 11; //Register Select
int EN = 10; //Enable
LiquidCrystal lcd(RS, EN, 5, 4, 3, 2);

// initialize the bluetooth serial data from HC-05
SoftwareSerial BTSerial(13, 12); // RX | TX

//instantiate the timer object
Timer t;


// define user definable globals, can change value, held in RAM memory
int offsetUTC = 2;                      // until we can implement an automatic timezone correction based on coordinates, we will assume UTC+2 timezone (Europe/Rome)
int currentLocale = ENG; // we will be displaying our strings in Italian for our own test phase, can be changed to another european locale (EN, IT, ES, FR, DE)
boolean useLangStrings = false;



// define global variables that can change value, held in RAM memory
String GPSCommandString;              // the string that will hold the incoming GPS data stream (one line at a time)
//unsigned long currentTime;          // will hold the Arduino millis()
int currentYear = 0;
int currentMonth = 0;
int currentDay = 0;
int currentHour = 0;
int currentMinute = 0;
int currentSecond = 0;

int tickEvent;                      // event that will be called once every second to create the local Arduino clock
int firstSynch;                     // event that will be called once after 1 second to synch the local Arduino clock with GPS time data
int synchEvent;                     // event that will be called once every 24 hours to synch the local Arduino clock with GPS time data
String timeString;                  // final string of the time to display on the LCD
String dateString;                  // final string of the data to display on the LCD



// define static variables, will never change, will be held in Flash memory instead of RAM
static const String GPSCommand = "$GPRMC";     // the GPS command string that we are looking for

static const String months[5][13] = {{"EN","Jan","Feb","Mar","Apr","May","June","July","Aug","Sept","Oct","Nov","Dec"},
{"IT","Gen", "Feb", "Mar", "Apr", "Mag", "Giu", "Lug", "Ago", "Sett", "Ott", "Nov", "Dic"},
{"ES","Ene", "Feb", "Mar", "Abr", "May", "Jun", "Jul", "Ago", "Sep", "Oct", "Nov", "Dic"},
{"FR","Jan", "Fev", "Mar", "Avr", "Mai", "Juin", "Juil", "Aout", "Sept", "Oct", "Nov", "Dec"},
{"DE","Jan", "Febr", "Marz", "Apr", "Mai", "Juni", "Juli", "Aug", "Sept", "Okt", "Nov", "Dez"}};

static const String dayOfWeek[5][7] = {{"Sun","Mon","Tue","Wed","Thu","Fri","Sat"},
{"Dom","Lun","Mart","Merc","Giov","Ven","Sab"},
{"Dom","Lun","Mart","Mier","Juev","Vier","Sab"},
{"Dim","Lund","Mard","Merc","Jeud","Vend","Sam"},
{"Sonn","Mon","Diens","Mitt","Donn","Frei","Sams"}};

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
  
  //initialize GPS Bluetooth Serial data
  BTSerial.begin(38400);
  //optionally initialize Serial monitor (for debugging feedback)
  Serial.begin(38400);
  //initialize lcd display
  lcd.begin(16, 2);

  
  tickEvent   = t.every(1000,       updateClock);
  firstSynch  = t.after(10,         synchTime);
  synchEvent  = t.every(DAYINMILLIS,synchTime);   // 86400000 millis = 24 hours

  lcd.setCursor(0,0);
  lcd.print("00:00:00");
  lcd.setCursor(0,1);
  lcd.print("dddd dd/mm/yyyy");

}





void loop() {
  if(BTSerial.available()>0){
    String inputString = BTSerial.readStringUntil('\n');
    if(inputString.startsWith(GPSCommand)){
      GPSCommandString = inputString;
    }
  }
  t.update();
}



/**************************
 * elaborateValues function
 * ************************
 * This function receives a one-line string of data from the GPS streamed data
 * It only receives the GPRMC command string
 * Following is the explanation of the GPRMC standard format:
 *
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

  //Serial.println(myString);

  //The GPS Unit that we are using for our testing uses NMEA 2.3, so we have eleven commas instead of just ten
  int idxComma[12];
  for(int p=0;p<12;p++){
    if(p==0){ idxComma[p] = myString.indexOf(','); }
    else{ idxComma[p] = myString.indexOf(',', idxComma[p-1]+1); }
  }

  String valueArray[6];  
  valueArray[0] = myString.substring(idxComma[0]+1, idxComma[1]);   //TIME
  valueArray[1] = myString.substring(idxComma[8]+1, idxComma[9]);   //DATE
  valueArray[2] = myString.substring(idxComma[2]+1, idxComma[3]);   //LATITUDE
  valueArray[3] = myString.substring(idxComma[4]+1, idxComma[5]);   //LONGITUDE
  valueArray[4] = myString.substring(idxComma[1]+1, idxComma[2]);   //STATUS OF FIX (ACTIVE OR VOID)
  //valueArray[5] = myString.substring(idxComma[] ); //SIGNAL INTEGRITY

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

  currentHour   = valueArray[0].substring(0,2).toInt();
  currentMinute = valueArray[0].substring(2,4).toInt();
  currentSecond = valueArray[0].substring(4,6).toInt();
  
  currentDay    = valueArray[1].substring(0,2).toInt();
  currentMonth  = valueArray[1].substring(2,4).toInt();
  currentYear   = valueArray[1].substring(4,6).toInt() + 2000;

  return true; //perhaps if fix not valid, or signal integrity not A or D, return false?
}



/**********************
 * updateClock function
 * ********************
 * is called once every second
 * this is the heart of the local Arduino clock
 * ********************
*/
void updateClock()
{
  /*
  //currentTime = millis();
  //currentTime += 1000;
  
  int seconds = round(currentTime / 1000);  
  if(seconds>59){ seconds = seconds - (60 * (seconds / 60)); }
  
  int minutes = round(currentTime / 1000 / 60);  
  if(minutes>59){ minutes = minutes - (60 * (minutes / 60)); }
  
  int hours = round(currentTime / 1000 / 60 / 60);
  if(hours>23){ hours = hours - (24 * (hours / 24)); }
  
  int days = round(currentTime / 1000 / 60 / 60 / 24);
  if(days>29){ days = days - (30 * (days / 30)); } //approximation of 30 days to a month!
  
  int months = round(currentTime / 1000 / 60 / 60 / 24 / 30);
  if(months>11){ months = months - (12 * (months / 12)); }
  
  int years = round(currentTime / 1000 / 60 / 60 / 24 / 30 / 12);
  
  timeString = (hours<10?"0"+String(hours):String(hours)) + ":" + (minutes<10?"0"+String(minutes):String(minutes)) +  ":" + (seconds<10?"0"+String(seconds):String(seconds));
  String ddString = (days<10?"0"+String(days):String(days));
  String mmString = (months<10?"0"+String(months):String(months));
  String yyString = (years<10?"000"+String(years):(years<100?"00"+String(years):(years<1000?"0"+String(years):String(years))));
  */

  currentSecond++;
  
  if(currentSecond>59){
    currentSecond = 0;
    currentMinute++;
  }

  if(currentMinute>59){
    currentMinute = 0;
    currentHour++;
  }

  if(currentHour>23){
    currentHour = 0;
    currentDay++;  
  }

  int monthLimit;
  if(currentMonth==9||currentMonth==4||currentMonth==6||currentMonth==11) //30 days hath September, April June and November
  {
    monthLimit = 30;
  }
  else if(currentMonth==1||currentMonth==3||currentMonth==5||currentMonth==7||currentMonth==8||currentMonth==10||currentMonth==12) //all the rest have 31 except February...
  {
    monthLimit = 31;
  }
  else if(currentMonth==2) //which has 28, except when leap year comes it has one more
  {
    if(isLeapYear(currentYear)){
      monthLimit = 29;
    }
    else
    {
      monthLimit = 28;
    }
  }
  
  if(currentDay>monthLimit){
    currentDay = 1;
    currentMonth++;
  }

  if(currentMonth>12){
    currentMonth=1;
    currentYear++;  
  }

  int dotw = dayOfTheWeek(currentYear,currentMonth,currentDay);
  String today = dayOfWeek[currentLocale][dotw];

  timeString = (currentHour<10?"0"+String(currentHour):String(currentHour)) + ":" + (currentMinute<10?"0"+String(currentMinute):String(currentMinute)) +  ":" + (currentSecond<10?"0"+String(currentSecond):String(currentSecond));
  String ddString = (currentDay<10?"0"+String(currentDay):String(currentDay));
  String mmString = (currentMonth<10?"0"+String(currentMonth):String(currentMonth));
  String yyString = (currentYear<10?"000"+String(currentYear):(currentYear<100?"00"+String(currentYear):(currentYear<1000?"0"+String(currentYear):String(currentYear))));
  
  if(useLangStrings){
    String monthString = months[currentLocale][currentMonth];
    if(currentLocale==ENG){
      dateString = today + " " + monthString + " " + ddString + ", " + yyString; 
    }
    else{
      dateString = today + " " + ddString + " " + monthString + " " + yyString;
    }
  }
  else{
    if(currentLocale==ENG){
      dateString = today + " " + mmString + "/" + ddString + "/" + yyString;
    }
    else{
      dateString = today + " " + ddString + "/" + mmString + "/" + yyString;
    }
  }
  
  lcd.setCursor(0,0);
  lcd.print(timeString);

  if(useLangStrings){
    lcd.setCursor(0,1);
  }
  else{
    lcd.setCursor(0,1);  
  }
  lcd.print(dateString);
}


void synchTime(){
  boolean updatedInfo = elaborateValues(GPSCommandString);
  //if not updatedInfo, perhaps set another one-time synch in 15 mins?
}


/*********************
 * isLeapYear function
 * *******************
 * information taken from https://support.microsoft.com/en-us/kb/214019
*/
boolean isLeapYear(int year){
  boolean result = false;
  if(year%4==0){
    if(year%100==0){
      if(year%400==0){
        result = true;
      }
      else{ result = false; }
    }
    else{
      result = true;
    }
  }
  else{
    result = false;
  }
  return result;
}

/***********************
 * dayOfTheWeek function
 * 
 * returns int from 0-6
 * corresponding to day of the week 
 * where 0=Sunday,6=Saturday
*/
/*
int dayOfTheWeek(int day, int month, int year){
  int century = 0;
  //if January or February
  if(month==1||month==2){
    year--;
    century = 1;
  }
  int yy = year % 28;
  int y;
  if(yy==1||yy==7||yy==12||yy==8){
    y=1;
  }
  if(yy==2||yy==13||yy==19||yy==24){
    y=2;
  }
  if(yy==3||yy==8||yy==14||yy==25){
    y=3;
  }
  if(yy==9||yy==15||yy==20||yy==26){
    y=4;
  }
  if(yy==4||yy==10||yy==21||yy==27){
    y=5;
  }
  if(yy==5||yy==11||yy==16||yy==22){
    y=6;
  }
  if(yy==6||yy==17||yy==23||yy==0){
    y=0;
  }
  return (day + monthsTable[month] + y + century) % 7;
}
*/
int dayOfTheWeek(int y, int m, int d)  /* 1 <= m <= 12,  y > 1752 (in the U.K.) */
{
    static int t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
    y -= m < 3;
    return (y + y/4 - y/100 + y/400 + t[m-1] + d) % 7;
}
