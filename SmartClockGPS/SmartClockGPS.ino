/*
 * Project:       GPS-SmartClock ™
 * Description:   Clock using GPS serial data in form of NMEA strings 
 *                to synchronize it's time with the atomic UTC timestamp provided in such strings
 *                This project uses a bluetooth GPS module together with an HC-05 bluetooth module
 *                to stream the GPS data over serial to the Arduino.
 *                A 16x2 LCD Display Module is used to display the date and time.
 * Board:         Atmega 1284P on a breadboard using bootloader "maniacbug Mighty 1284P 16MHZ using Optiboot"
 * Bootloader:    https://github.com/JChristensen/mighty-1284p/tree/v1.6.3 (for usage with Arduino 1.6.3 and higher)
 * Authors:       Vincenzo d'Orso (icci87@gmail.com) and John D'Orazio (john.dorazio@cappellaniauniroma3.org)
 * License:       GPLv3 (see full license at bottom of this file)
 * Last Modified: 23 January 2016
 * 
 * A project of the Microcontrollers Users Group - Roma Tre University
 * MUG UniRoma3 http://muglab.uniroma3.it/
 * 
 *     
 */
#include <avr/wdt.h>

#include <LiquidCrystal.h>
#include "Timer.h"                     //http://github.com/JChristensen/Timer
#include "SmartClock.h"

//module max inquire devices, can change to optimize HC-05 connectivity
#define MAX_DEVICES   15


//locale definitions
//#define ENG 0
//#define ITA 1
//#define ESP 2
//#define FRA 3
//#define DEU 4

//time definitions
#define SECONDINMILLIS      1000      // 1 second (in milliseconds)
#define MINUTEINMILLIS      60000     // 1 minute (in milliseconds)
#define HOURINMILLIS        3600000   // 1 hour (in milliseconds)
#define DAYINMILLIS         86400000  // 24 hours (in milliseconds)

//menu item definitions
#define UTCOFFSETMENUITEM   0
#define LANGUAGEMENUITEM    1
#define DATEVIEWMENUITEM    2

//pin definitions
#define LED_PIN             0
#define LCD_RS              16 //Register Select
#define LCD_EN              17 //Enable
#define LCD_D0              24
#define LCD_D1              25
#define LCD_D2              26
#define LCD_D3              27
#define LCD_D4              28
#define LCD_D5              29
#define LCD_D6              30
#define LCD_D7              31
#define MENUBUTTON          19
#define NAVIGATEBUTTON      18
#define HC05_STATE_PIN      12  //hc-05 state digital pin
#define HC05_KEY_PIN        13  //hc-05 key digital pin
#define HC05_EN_PIN         14  //hc-05 enable digital pin

static const String HC05_ERRORMESSAGE[29] = {
  "Command Error/Invalid Command",
  "Results in default value",
  "PSKEY write error",
  "Device name is too long (>32 characters)",
  "No device name specified (0 length)",
  "Bluetooth address NAP is too long",
  "Bluetooth address UAP is too long",
  "Bluetooth address LAP is too long",
  "PIO map not specified (0 length)",
  "Invalid PIO port Number entered",
  "Device Class not specified (0 length)",
  "Device Class too long",
  "Inquire Access Code not Specified (0 length)",
  "Inquire Access Code too long",
  "Invalid Inquire Access Code entered",
  "Pairing Password not specified (0 length)",
  "Pairing Password too long (> 16 characters)",
  "Invalid Role entered",
  "Invalid Baud Rate entered",
  "Invalid Stop Bit entered",
  "Invalid Parity Bit entered",
  "No device in the Pairing List",
  "SPP not initialized",
  "SPP already initialized",
  "Invalid Inquiry Mode",
  "Inquiry Timeout occured",
  "Invalid/zero length address entered",
  "Invalid Security Mode entered",
  "Invalid Encryption Mode entered"
};

static const String initphase[4] = {
  "INITIALIZING.   ",
  "INITIALIZING..  ",
  "INITIALIZING... ",
  "INITIALIZING...."
};
static const String searchphase[2] = {
  "SearchingDevices",
  "-v-^-v-^-v-^-v-^",
};

int initcount = 0;
int searchcount = 0;

ProgramState OLDPROGRAMSTATE;
ProgramState CURRENTPROGRAMSTATE;
boolean PROGRAMSTATECHANGED;

boolean SETTINGHC05MODE;
boolean INITIALIZING;


// initialize the display library with the numbers of the interface pins
//LiquidCrystal lcd(LCD_RS, LCD_EN, LCD_D4, LCD_D5, LCD_D6, LCD_D7);
LiquidCrystal lcd(LCD_RS, LCD_EN, LCD_D0, LCD_D1, LCD_D2, LCD_D3, LCD_D4, LCD_D5, LCD_D6, LCD_D7);

// initialize the bluetooth serial data from HC-05
//SoftwareSerial BTSerial(13, 12); // RX | TX

//instantiate the timer object
Timer t;
int dynamicEvent;

HC05MODE HC05_MODE;             //can be AT_MODE or COMMUNICATION_MODE
HC05CMODE currentCMODE;         //can be CONNECT_BOUND, CONNECT_ANY, or CONNECT_SLAVE_LOOP
HC05STATE HC05_STATE;           //can be CONNECTED or DISCONNECTED
HC05STATE HC05_OLDSTATE;        //can be CONNECTED or DISCONNECTED
LOCALE currentLocale;           //can be ENG, ITA, ESP, FRA, DEU
// define user changeable globals (held in RAM memory)
int recentDeviceCount = 0;

int deviceCount = 0;
String devices[MAX_DEVICES];
int currentDeviceIdx = 0;
String currentDeviceAddr;
String currentDeviceName;

int currentFunctionStep = 0;

String incoming;
String outgoing;

int offsetUTC;
boolean useLangStrings = true;



// define global variables (not user changeable, only the program can change their value) held in RAM memory
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
int chronometerEvent;               // event that will be called once every 50 milliseconds as a chronometer function
String timeString;                  // final string of the time to display on the LCD
String dateString;                  // final string of the data to display on the LCD
unsigned long currentMillis;        // will count the current milliseconds of the Arduino when the chronometer function is activated

unsigned long oldtime; //useful for filtering button press
unsigned long newtime; //useful for filtering button press
int MENUBUTTONSTATE = LOW;
int NAVIGATEBUTTONSTATE = LOW;
boolean MENUBUTTONPRESSED = false;
boolean NAVIGATEBUTTONPRESSED = false;

boolean MENUACTIVE = false;
int MENULEVEL = 0;
int CURRENTMENUITEM = 0;
int PREVIOUSMENUITEM = 0;
int MENUITEMS;


// define static variables, will never change (held in Flash memory instead of RAM)
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

static const String settingsMenu[5] = {"SETTINGS","IMPOSTAZIONI","AJUSTES","PARAMETRES","EINSTELLUNGEN"};

static const String settingsMenuItems[5][3] = {{"UTC OFFSET","LANGUAGE","DATE VIEW"},
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
  //wdt_disable();
  pinMode(LED_PIN, OUTPUT);
  pinMode(HC05_STATE_PIN, INPUT);
  pinMode(HC05_KEY_PIN, OUTPUT);
  pinMode(HC05_EN_PIN, OUTPUT);
  pinMode(MENUBUTTON, INPUT);
  pinMode(NAVIGATEBUTTON, INPUT);
  oldtime = millis(); //useful for filtering button presses

  //optionally initialize Serial monitor (for debugging feedback)
  Serial.begin(38400);
  while(!Serial){} //wait until Serial is ready
  //Serial.println("Serial communication is ready!");
  
  //initialize GPS Bluetooth Serial data
  Serial1.begin(38400);
  while(!Serial1){} //wait until Serial1 is ready
  //Serial.println("HC-05 serial is ready too!");

  //we will be displaying our strings in English for our own test phase
  //supported locales are ENG, ITA, ESP, FRA, DEU
  currentLocale = ENG; 
  offsetUTC = 1;
  
  //initialize lcd display
  lcd.begin(16, 2);
  //lcd.setCursor(0,0);
  //lcd.print("hh:mm:ss");
  //lcd.setCursor(0,1);
  //lcd.print("ddd dd/mm/yyyy");

  tickEvent   = t.every(1000,       updateClock);
  firstSynch  = t.after(1100,         synchTime);
  synchEvent  = t.every(SECONDINMILLIS,  synchTime);   // 86400000 millis = 24 hours
  chronometerEvent = t.every(50,    chronometer);

  MENUBUTTONSTATE = LOW;
  NAVIGATEBUTTONSTATE = LOW;
  MENUBUTTONPRESSED = false;
  NAVIGATEBUTTONPRESSED = false;

  MENUACTIVE = false;
  MENULEVEL = 0;
  CURRENTMENUITEM = 0;
  PREVIOUSMENUITEM = 0;

  SETTINGHC05MODE = false;
  INITIALIZING = false;
  CURRENTPROGRAMSTATE = INITIALSTATECHECK;
  //Start the HC-05 module in communication mode
  HC05_MODE = COMMUNICATION_MODE;  
  //Set_HC05_MODE function uses HC05_MODE global instead of sending the mode in as a parameter,
  //because this function is called in multiple steps by the timer object, as a callback,
  //making it complicated to handle parameters...
  Set_HC05_MODE(); 

}





void loop() {
  
  t.update();

  if(SETTINGHC05MODE == false && CURRENTPROGRAMSTATE == INITIALSTATECHECK){
    HC05_STATE = Check_HC05_STATE();
    if(HC05_STATE == CONNECTED){
      lcd.setCursor(0,1);
      lcd.print("LINKED TO GPS!  ");
      Serial.println("HC-05 is connected and is now listening for NMEA data...");
      CURRENTPROGRAMSTATE = LISTENNMEA;
    }
    else if(HC05_STATE == DISCONNECTED){
      lcd.setCursor(0,1);
      lcd.print("NO LINK TO GPS..");
      Serial.println("HC-05 is not connected, so let's see what we can do about that.");
      HC05_MODE = AT_MODE;      
      Set_HC05_MODE();
    }
  }
  else if(SETTINGHC05MODE == false && CURRENTPROGRAMSTATE == DO_ADCN){
    CountRecentAuthenticatedDevices();
  }
  else if(SETTINGHC05MODE == false && (CURRENTPROGRAMSTATE == LISTENNMEA || CURRENTPROGRAMSTATE == CONFRONTINGUSER)){
    newtime = millis(); //useful for filtering button press
    checkButtonsPressed();
  }
  else if(SETTINGHC05MODE == true){
  }

  //TODO: instead of reading input from Serial monitor, use buttons!
  //read serial data from serial monitor and send to HC-05 module:
  if (Serial.available() > 0) {
    outgoing = Serial.readStringUntil('\n');
    //Serial.println(outgoing);
    if(CURRENTPROGRAMSTATE == CONFRONTINGUSER){
      if(outgoing.startsWith("Y")){
        currentCMODE = SetConnectionMode(CONNECT_BOUND);
      }
      else if(outgoing.startsWith("N")){
        currentDeviceIdx++;
        if(currentDeviceIdx < deviceCount){
          while(devices[currentDeviceIdx] == devices[currentDeviceIdx-1]){
            if(currentDeviceIdx < deviceCount-1){ currentDeviceIdx++; }
            else{ break; }            
          }
          if(devices[currentDeviceIdx] != devices[currentDeviceIdx-1]){
            currentDeviceAddr = devices[currentDeviceIdx];
            ConfrontUserWithDevice(currentDeviceAddr);
          }
          else{
            //Serial.println("Devices are exhausted. Perhaps you need to turn on the correct Bluetooth Device and make it searchable.");            
            InitiateInquiry();
          }
        }
        else{
          //Serial.println("Devices are exhausted. Perhaps you need to turn on the correct Bluetooth Device and make it searchable.");
          InitiateInquiry();
        }
      }
    }
    else{
      Serial1.println(outgoing);
    }
    //Serial1.write(Serial.read()); 
  }

  if(Serial1.available()>0){
    if(SETTINGHC05MODE && INITIALIZING==false){
      Serial1.read(); //just throw it away, don't do anything with it
    }
    else if(INITIALIZING){
      //let's not worry about parsing all the results... just print them out
      Serial.write(Serial1.read());
    }
    else{
      incoming = "";
  
      byte inc = Serial1.read(); //check the incoming character    
      if(inc != 0 && inc < 127){ //if the incoming character is not junk then continue reading the string
        //Serial.println("[incoming serial data is not junk, now reading line...]");
        incoming = char(inc) + Serial1.readStringUntil('\n');
        //Serial.println(incoming);

        if(incoming.startsWith("ERROR")){
          int idx1 = incoming.indexOf('(');
          int idx2 = incoming.indexOf(')');            
          String errCode = incoming.substring(idx1+1,idx2);            
          //Serial.println(incoming + ": " + getErrorMessage(errCode));
        }
        
        switch(CURRENTPROGRAMSTATE){
          
          case COUNTINGRECENTDEVICES:
            if(incoming.startsWith("+ADCN")){
              Serial.println("Response from recent device count is positive!");
              String deviceCountStr = incoming.substring(incoming.indexOf(':')+1);
              recentDeviceCount = deviceCountStr.toInt();
              
              flushOkString();
              
              if(recentDeviceCount>0){
                String dev = (recentDeviceCount==1?" device":" devices");
                String mex = "HC-05 has been recently paired with " + deviceCountStr + dev;
                Serial.println(mex);
                CheckMostRecentAuthenticatedDevice();
              }else{
                Serial.println("There do not seem to be any recent devices.");
                //We will have to initiate inquiry of nearby devices. First, we have to make sure we that the HC-05 is set to connect to any device.
                currentCMODE = SetConnectionMode(CONNECT_ANY);
              }
            }
            else{
              Serial.println("COUNTINGRECENTDEVICES ???");
            }

            break;
            
          case COUNTEDRECENTDEVICES:
            if(incoming.startsWith("+MRAD")){
              Serial.println("I believe I have detected the Address of the most recent device...");
              
              flushOkString();
    
              int idx = incoming.indexOf(':'); // should this be a space perhaps...
              //int idx1 = incoming.indexOf(',');
              if(idx != -1){ // && idx1 != -1
                String addr = incoming.substring(idx+1); //,idx
                addr.replace(':',',');
                addr.trim();
                currentDeviceAddr = addr;
                SearchAuthenticatedDevice(currentDeviceAddr); 
              }
              else{
                Serial.println("The response string surprised me, I don't know what to say.");
              }
            }
            else{
              Serial.println("The response string does not seem to be an address for the most recent device...");
            }
            break;

          case SEARCHAUTHENTICATEDDEVICE:
            if(incoming.startsWith("OK")){
                Serial.println("Now ready to connect to device with address <" + currentDeviceAddr + ">.");
                ConnectRecentAuthenticatedDevice(currentDeviceAddr);
            }
            else if(incoming.startsWith("FAIL")){
              Serial.println("Unable to prepare for connection with device whose address is <" + currentDeviceAddr + ">.");
            }
            else{
              Serial.println("SEARCHAUTHENTICATEDDEVICE ???");
            }
            break;
            
          case CONNECTINGRECENTDEVICE:
            if(incoming.startsWith("OK")){
              //Serial.println("HC-05 is now connected to most recent authenticated device");
            }
            else if(incoming.startsWith("FAIL")){
              Serial.println("Connection to most recent authenticated device has failed.");
              InquireDevices();
            }
            else{
              Serial.println("CONNECTINGRECENTDEVICE ???");
            }
            break;

          case SETTINGCONNECTIONMODE:
            if(incoming.startsWith("OK")){
              if(currentCMODE == CONNECT_ANY){
                Serial.println("CMODE successfully set to connect to any device.");
                InitiateInquiry();
              }
              else if(currentCMODE == CONNECT_BOUND){
                Serial.println("CMODE successfully set to connect only to bound device.");
                SetBindAddress();
              }
            }
            break;

          case INITIATINGINQUIRY:
            if(incoming.startsWith("OK")){
              InquireDevices();
            }
            else if(incoming.startsWith("ERROR")){
              int idx1 = incoming.indexOf('(');
              int idx2 = incoming.indexOf(')');            
              String errCode = incoming.substring(idx1+1,idx2);            
              if(errCode == "17"){ //already initiated, no problem, continue just the same
                InquireDevices();  
              }
              else{
                Serial.println(incoming + ": " + getErrorMessage(errCode));
              }                
            }
            break;
            
          case INQUIRINGDEVICES:
            if(incoming.startsWith("OK")){
              Serial.println("Finished inquiring devices.");
              lcd.setCursor(0,1);
              lcd.print("FOUND ");
              lcd.print(deviceCount);
              lcd.print(" DEVICES ");
              currentDeviceIdx=0;
              if(deviceCount > 0){
                currentDeviceAddr = devices[currentDeviceIdx];
                ConfrontUserWithDevice(currentDeviceAddr);  
              }        
              else{
                Serial.print("Sorry, I have not found any useful devices. Trying again.");
                InitiateInquiry();
              }
            }
            else if(incoming.startsWith("+INQ")){
              Serial.println(incoming);
              int idx = incoming.indexOf(':');
              int idx2 = incoming.indexOf(',');
              String addr = incoming.substring(idx+1,idx2);
              addr.replace(':',',');
              addr.trim();
              lcd.setCursor(0,1);
              int spaces = 16-addr.length();
              lcd.print(addr);
              for(int p=0;p<spaces;p++){
                lcd.print(" ");
              }
              if(inArray(addr,devices,MAX_DEVICES) == -1){ 
                devices[deviceCount] = addr;
                deviceCount++;
              }
            }                           
            break;

          case CONFRONTINGUSER:
            if(incoming.startsWith("+RNAME")){                                  
              incoming.trim();
              currentDeviceName = incoming.substring(incoming.indexOf(':')+1);
              
              flushOkString();
              
              Serial.println("Would you like to connect to '" + currentDeviceName + "'? Please type Y or N.");
              lcd.clear();
              lcd.setCursor(0,0);
              lcd.print(" ConnectDevice? ");
              lcd.setCursor(0,1);
              lcd.print("Y");
              int devNameLen = currentDeviceName.length();
              int space = 16-(devNameLen+2);
              if(space>0){
                if(space % 2 == 0){
                   for(int p=0;p<(space/2);p++){
                    lcd.print(" ");
                   }
                   lcd.print(currentDeviceName);
                   for(int p=0;p<(space/2);p++){
                    lcd.print(" ");
                   }                   
                }
                else{
                   for(int p=0;p<(ceil(space/2));p++){
                    lcd.print(" ");
                   }
                   lcd.print(currentDeviceName);
                   for(int p=0;p<(floor(space/2));p++){
                    lcd.print(" ");
                   }                                     
                }
              }
              else{
                lcd.print(currentDeviceName);
                lcd.setCursor(14,1);
                lcd.print(" ");
              }
              lcd.print("N");
            }
            else if(incoming.startsWith("FAIL")){
              Serial.println("Could not retrieve name of detected device... continuing to next");
              currentDeviceIdx++;
              if(currentDeviceIdx < deviceCount){
                currentDeviceAddr = devices[currentDeviceIdx];
                ConfrontUserWithDevice(currentDeviceAddr);
              }
              else{
                Serial.println("Devices are exhausted. Searching again.");
                InitiateInquiry();
              }
            }
            else{
              Serial.println("There was an error retrieving the device name... continuing to next");
              currentDeviceIdx++;
              if(currentDeviceIdx < deviceCount){
                currentDeviceAddr = devices[currentDeviceIdx];
                ConfrontUserWithDevice(currentDeviceAddr);
              }
              else{
                Serial.println("Devices are exhausted. Searching again.");
                InitiateInquiry();
              }
            }
            break;

          case SETTINGBINDADDRESS:
            if(incoming.startsWith("OK")){
              Serial.println("Bound HC-05 to bluetooth device.");
              LinkToCurrentDevice(currentDeviceAddr);          
            }
            else{
              Serial.println("SETTINGBINDADDRESS what could possible have gone wrong?");
            }
            break;

          case CONNECTINGTODEVICE:
            if(incoming.startsWith("OK")){
              Serial.println("Successfully connected to "+currentDeviceName);
              lcd.clear();              
              //WE HAVE MOST PROBABLY LEFT AT MODE AND ARE IN COMMUNICATION MODE NOW
              CURRENTPROGRAMSTATE = LISTENNMEA;
            }
            else if(incoming.startsWith("FAIL")){
              Serial.println("Failed to connect to "+currentDeviceName + ". Resetting device...");
              resetAllVariables();
              HC05_MODE = AT_MODE;            
              Set_HC05_MODE();
            }
            else{
              Serial.println("Error attempting connection to "+currentDeviceName + ". Resetting device...");
              resetAllVariables();
              HC05_MODE = AT_MODE;            
              Set_HC05_MODE();
            }
            break;
          case LISTENNMEA:
            if(incoming.startsWith(GPSCommand)){
              GPSCommandString = incoming;
            }
            break;
        }        
      }
    }
  }

  if(CURRENTPROGRAMSTATE == LISTENNMEA && MENUACTIVE){
    printMenu();
  }
  
}



/**************************
 * elaborateGPSValues function
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
boolean elaborateGPSValues(String myString){
  
  int hours,minutes,seconds,day,month,year;  
  String hourString,minuteString,secondString,dayString,monthString,yearString,timeString,dateString;

  Serial.println(myString);

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

  currentHour   = valueArray[0].substring(0,2).toInt() + offsetUTC;
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
  String today = dayOfWeek[(int)currentLocale][dotw];

  timeString = (currentHour<10?"0"+String(currentHour):String(currentHour)) + ":" + (currentMinute<10?"0"+String(currentMinute):String(currentMinute)) +  ":" + (currentSecond<10?"0"+String(currentSecond):String(currentSecond));
  String ddString = (currentDay<10?"0"+String(currentDay):String(currentDay));
  String mmString = (currentMonth<10?"0"+String(currentMonth):String(currentMonth));
  String yyString = (currentYear<10?"000"+String(currentYear):(currentYear<100?"00"+String(currentYear):(currentYear<1000?"0"+String(currentYear):String(currentYear))));
  
  if(useLangStrings){
    String monthString = months[(int)currentLocale][currentMonth];
    if(currentLocale==ENG){
      dateString = today + " " + monthString + " " + ddString + " " + yyString; 
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
  
  if(CURRENTPROGRAMSTATE == LISTENNMEA && MENUACTIVE==false){
    lcd.setCursor(0,0);
    lcd.print(timeString);
  
    if(useLangStrings){ //TODO: what is this?
      lcd.setCursor(0,1);
    }
    else{
      lcd.setCursor(0,1);  
    }
    lcd.print(dateString);
  }
  else if(SETTINGHC05MODE == true || (CURRENTPROGRAMSTATE == INITIALSTATECHECK || CURRENTPROGRAMSTATE == DO_ADCN)){
    lcd.setCursor(0,0);
    lcd.print(initphase[initcount]);
    initcount++;
    if(initcount > 3){ initcount = 0; }
  }
  else if(CURRENTPROGRAMSTATE == INQUIRINGDEVICES){
    lcd.setCursor(0,0);
    lcd.print(searchphase[searchcount]);
    searchcount = 1 ^ searchcount;
    Serial.println("(searchcount = " + (String)searchcount + ")");
  }
}

/********************
 * synchTime function
 * ******************
 * tries to get time values from GPS
 * if not succeeds, tries again?
*/
void synchTime(){
  if(CURRENTPROGRAMSTATE == LISTENNMEA){
    boolean updatedInfo = elaborateGPSValues(GPSCommandString);
  }
  //if not updatedInfo, perhaps set another one-time synch in 1 min?
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
 * *********************
 * returns int from 0-6
 * corresponding to day of the week 
 * where 0=Sunday,6=Saturday
 * information gathered from wikipedia article https://en.wikipedia.org/wiki/Determination_of_the_day_of_the_week
*/
int dayOfTheWeek(int y, int m, int d)  /* 1 <= m <= 12,  y > 1752 (in the U.K.) */
{
    static int t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
    y -= m < 3;
    return (y + y/4 - y/100 + y/400 + t[m-1] + d) % 7;
}




/******************************
 * checkButtonsPressed function
 * ****************************
 * 
 */

void checkButtonsPressed(){
  
  // CHECK STATE OF MENU BUTTON
  if(digitalRead(MENUBUTTON) == HIGH && MENUBUTTONPRESSED == false && (newtime-oldtime) > 400){
    MENUBUTTONPRESSED = true;
    oldtime = newtime;
  }
  else{
    MENUBUTTONPRESSED = false;
  }
  
  // CHECK STATE OF NAVIGATE BUTTON
  if(digitalRead(NAVIGATEBUTTON) == HIGH && NAVIGATEBUTTONPRESSED == false && (newtime-oldtime) > 400){
    NAVIGATEBUTTONPRESSED = true;
    oldtime = newtime;
    lcd.clear();
  }
  else{
    NAVIGATEBUTTONPRESSED = false;
  }

  if(CURRENTPROGRAMSTATE == CONFRONTINGUSER){
    if(MENUBUTTONPRESSED){
      MENUBUTTONPRESSED = false;
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("-Linking device-");
      currentCMODE = SetConnectionMode(CONNECT_BOUND);
    }
    if(NAVIGATEBUTTONPRESSED){
      NAVIGATEBUTTONPRESSED = false;
      currentDeviceIdx++;
      if(currentDeviceIdx < deviceCount){
        while(devices[currentDeviceIdx] == devices[currentDeviceIdx-1]){
          if(currentDeviceIdx < deviceCount-1){ currentDeviceIdx++; }
          else{ break; }            
        }
        if(devices[currentDeviceIdx] != devices[currentDeviceIdx-1]){
          currentDeviceAddr = devices[currentDeviceIdx];
          ConfrontUserWithDevice(currentDeviceAddr);
        }
        else{
          //Serial.println("Devices are exhausted. Perhaps you need to turn on the correct Bluetooth Device and make it searchable.");            
          InitiateInquiry();
        }
      }
      else{
        //Serial.println("Devices are exhausted. Perhaps you need to turn on the correct Bluetooth Device and make it searchable.");
        InitiateInquiry();
      }      
    }
  }
  else{
    if(MENUBUTTONPRESSED && MENUACTIVE == false){
      MENUBUTTONPRESSED = false;
      MENUACTIVE = true; //will stay true until a final menu selection has taken place
      lcd.clear();
    }
    else if(MENUBUTTONPRESSED && MENUACTIVE == true){
      MENUBUTTONPRESSED = false;
      if(MENULEVEL == 0){
        PREVIOUSMENUITEM = CURRENTMENUITEM;
        CURRENTMENUITEM = 0;
        MENULEVEL++;
        lcd.clear();
      }
      else{
        saveSettings();
        resetAndExitMenu();
        lcd.clear();
      }
    }
  
    if(NAVIGATEBUTTONPRESSED && MENUACTIVE){
      NAVIGATEBUTTONPRESSED = false;
      CURRENTMENUITEM++;
      if(CURRENTMENUITEM >= MENUITEMS){
        CURRENTMENUITEM = 0;
      }
    }
  }
  //CHRONOMETER FUNCTION BUTTON
  //IF CHRONOMETER PRESSED, THEN {
  //currentMillis = millis();
  //lcd.clear();
  //chronometer();
  //}
}


/********************
 * printMenu function
 * ******************
 */

void printMenu(){
    //lcd.clear();
    if(MENULEVEL == 0){
      lcd.setCursor(0,0);  
      lcd.print(settingsMenu[currentLocale]);

      MENUITEMS = 3;
      lcd.setCursor(0,1);
      lcd.print(">> "+settingsMenuItems[currentLocale][CURRENTMENUITEM]);      
    }  
    else if(MENULEVEL == 1){
      lcd.setCursor(0,0);
      lcd.print(""+settingsMenuItems[currentLocale][PREVIOUSMENUITEM]);
      
      //determine number of menu items
      if(PREVIOUSMENUITEM == UTCOFFSETMENUITEM){
        MENUITEMS = 27;
        lcd.setCursor(0,1);
        lcd.print(utcOffsetValues[CURRENTMENUITEM]);
      }
      else if(PREVIOUSMENUITEM == LANGUAGEMENUITEM){
        MENUITEMS = 5;
        lcd.setCursor(0,1);
        lcd.print(languageValues[CURRENTMENUITEM]);
      }
      else if(PREVIOUSMENUITEM == DATEVIEWMENUITEM){
        MENUITEMS = 2;
        lcd.setCursor(0,1);
        lcd.print(dateViewValues[currentLocale][CURRENTMENUITEM]);
      }
      
    }
}





/***********************
 * saveSettings function
 * *********************
 * 
 */

void saveSettings(){
    if(PREVIOUSMENUITEM == UTCOFFSETMENUITEM){
      //Serial.println("Saving UTC OFFSET... current selected value = "+utcOffsetValues[CURRENTMENUITEM]);
      if(utcOffsetValues[CURRENTMENUITEM].startsWith("+") || utcOffsetValues[CURRENTMENUITEM].startsWith("-")){
        String signVal = utcOffsetValues[CURRENTMENUITEM].substring(0,1);
        int numVal = utcOffsetValues[CURRENTMENUITEM].substring(1).toInt();
        //Serial.println("Split into sign = <"+signVal+"> and number = <"+numVal+">");
        if(signVal == "+"){
          offsetUTC = (0 + numVal);
        }
        else if(signVal == "-"){
          offsetUTC = (0 - numVal);
        }
        Serial.print("offsetUTC now has value <");
        Serial.print(offsetUTC);
        Serial.println(">");
        synchTime();
      }
    }
    else if(PREVIOUSMENUITEM == LANGUAGEMENUITEM){
      currentLocale = (LOCALE)CURRENTMENUITEM;
    }
    else if(PREVIOUSMENUITEM == DATEVIEWMENUITEM){
      useLangStrings = CURRENTMENUITEM==0?true:false;
    }
}






/***************************
 * resetAndExitMenu function
 * *************************
 * 
 */


void resetAndExitMenu(){
    PREVIOUSMENUITEM = 0;
    CURRENTMENUITEM = 0;
    MENULEVEL = 0;
    MENUITEMS = 0;
    MENUACTIVE = false;
}

/*********************************
 * chronometer function
 * *******************************
 * is called every 50 milliseconds
 */

void chronometer(){
  
  unsigned long currentTime = millis() - currentMillis;
  
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
  
  //lcd print only milliseconds if second has not yet changed...
  //lcd print seconds and milliseconds when second has changed...
  //lcd print minutes, seconds, and milliseconds when minute has changed...
  //This way the screen will not flicker constantly
}

void Set_HC05_MODE(){
  Serial.print(F("[currentFunctionStep = "));
  Serial.print(String(currentFunctionStep));
  Serial.println(F("]"));
  lcd.setCursor(0,1);
  //lcd.print("ddd dd/mm/yyyy"); //TODO: add useful message
  if(currentFunctionStep==0 && SETTINGHC05MODE == false){
    SETTINGHC05MODE = true;
    Serial.print(F("Now setting HC-05 mode to "));
    if(HC05_MODE == COMMUNICATION_MODE){
      Serial.println(F("COMMUNICATION_MODE"));  
    }else if(HC05_MODE == AT_MODE){
      Serial.println(F("AT_MODE"));
      CURRENTPROGRAMSTATE = DO_ADCN;
    }
    digitalWrite(HC05_EN_PIN, LOW); //EN to LOW = disable (pull low to reset when changing modes!)
    currentFunctionStep++;
    Serial.println("Preparing next step = " + (String)currentFunctionStep);
    dynamicEvent = t.after(500,Set_HC05_MODE);
  }
  else if(currentFunctionStep==1){
    Serial.print(F("HC05_MODE 0 o 1? >> "));
    Serial.println((int)HC05_MODE);
    if(HC05_MODE==COMMUNICATION_MODE){
      digitalWrite(HC05_KEY_PIN, LOW); //KEY to HIGH = full AT mode, to LOW = communication mode
    }
    else if(HC05_MODE==AT_MODE){
      digitalWrite(HC05_KEY_PIN, HIGH); //KEY to HIGH = full AT mode, to LOW = communication mode    
    }
    Serial.println(F("Changed KEY pin state!"));
    currentFunctionStep++;
    Serial.println("Preparing next step = " + (String)currentFunctionStep);
     dynamicEvent = t.after(500,Set_HC05_MODE);
  }
  else if(currentFunctionStep==2){
    Serial.println(F("We have effectively entered step 2!"));
    digitalWrite(HC05_EN_PIN, HIGH); //EN to HIGH = enable
    currentFunctionStep++;
    if(HC05_MODE == AT_MODE){
      dynamicEvent = t.after(2000,Set_HC05_MODE);
    }
    else if(HC05_MODE == COMMUNICATION_MODE){
      dynamicEvent = t.after(5000,Set_HC05_MODE);
    }
  }
  else if(currentFunctionStep==3){
    if(HC05_MODE == AT_MODE){
      currentFunctionStep++;
      INITIALIZING = true;
      Serial.println(F(">>AT+ROLE=1"));
      Serial1.println(F("AT+ROLE=1"));
      dynamicEvent = t.after(1000,Set_HC05_MODE);
    }
    else if(HC05_MODE == COMMUNICATION_MODE){
      currentFunctionStep=0;    
      SETTINGHC05MODE = false;      
    }
  }  
  else if(currentFunctionStep==4){
    currentFunctionStep++;
    Serial.println(F(">>AT+CMODE=1"));
    Serial1.println(F("AT+CMODE=1"));
    dynamicEvent = t.after(1000,Set_HC05_MODE);    
  }
  else if(currentFunctionStep==5){
    currentFunctionStep++;
    Serial.println(F(">>AT+IPSCAN=1024,512,1024,512"));
    Serial1.println(F("AT+IPSCAN=1024,512,1024,512"));
    dynamicEvent = t.after(1000,Set_HC05_MODE);        
  }
  else if(currentFunctionStep==6){
    currentFunctionStep++;
    Serial.println(F(">>AT+INQM=1,15,12"));
    Serial1.println(F("AT+INQM=1,15,12"));
    dynamicEvent = t.after(1000,Set_HC05_MODE);        
  }
  else if(currentFunctionStep==7){
    currentFunctionStep++;
    Serial.println(F(">>AT+PSWD=0000"));
    Serial1.println(F("AT+PSWD=0000"));
    dynamicEvent = t.after(1000,Set_HC05_MODE);        
  }
  else if(currentFunctionStep==8){
    currentFunctionStep=0;    
    INITIALIZING=false;
    SETTINGHC05MODE = false;
  }
}

HC05STATE Check_HC05_STATE(){
  Serial.println(F("Checking HC_05 connected state..."));
  int stateReading = digitalRead(HC05_STATE_PIN);
  HC05STATE state = static_cast<HC05STATE>(stateReading);
  if(state != HC05_OLDSTATE){
    HC05_OLDSTATE = state;
    if(state == CONNECTED){
      Serial.println(F("HC-05 is connected to a bluetooth device."));
    }
    else if(state == DISCONNECTED){
      Serial.println(F("HC-05 is disconnected from all bluetooth devices."));
    }
    digitalWrite(LED_PIN, state);
  }
  return state;  
}

void CountRecentAuthenticatedDevices(){
  CURRENTPROGRAMSTATE = COUNTINGRECENTDEVICES;
  Serial.println(F("Now counting recent connected devices..."));
  Serial.println(F("->AT+ADCN"));
  Serial1.println(F("AT+ADCN"));  
}

void CheckMostRecentAuthenticatedDevice(){
  CURRENTPROGRAMSTATE = COUNTEDRECENTDEVICES;
  Serial.println(F("Now checking the most recent authenticated device..."));
  Serial.println(F("->AT+MRAD"));
  Serial1.println(F("AT+MRAD"));  
}

void SearchAuthenticatedDevice(String addr){
  CURRENTPROGRAMSTATE = SEARCHAUTHENTICATEDDEVICE;
  Serial.print(F("Now preparing to link to device whose address is: <"));
  Serial.print(addr);
  Serial.println(F(">"));
  Serial.println(F("->AT+INIT"));
  Serial1.println(F("AT+INIT"));  
}

void ConnectRecentAuthenticatedDevice(String addr){
  CURRENTPROGRAMSTATE = CONNECTINGRECENTDEVICE;
  Serial.print(F("Now connecting to device whose address is: <"));
  Serial.print(addr);
  Serial.println(F(">"));
  Serial.print(F("->AT+LINK="));
  Serial.println(addr);
  Serial1.print(F("AT+LINK="));
  Serial1.println(addr);  
}

HC05CMODE SetConnectionMode(HC05CMODE mode){
  CURRENTPROGRAMSTATE = SETTINGCONNECTIONMODE;
  Serial.println(F("Setting connection mode..."));
  Serial.print(F("->AT+CMODE="));
  Serial.println(String(mode));
  Serial1.print(F("AT+CMODE="));
  Serial1.println(String(mode));  
  return mode;
}

void InitiateInquiry(){
  CURRENTPROGRAMSTATE = INITIATINGINQUIRY;
  resetAllVariables();
  Serial.println(F("Initiating inquiry..."));
  Serial.println(F("->AT+INIT"));
  Serial1.println(F("AT+INIT"));  
}

void InquireDevices(){
  CURRENTPROGRAMSTATE = INQUIRINGDEVICES;
  Serial.println(F("Inquiring devices..."));
  Serial.println(F("->AT+INQ"));
  Serial1.println(F("AT+INQ"));  
}

void ConfrontUserWithDevice(String devicexAddr){
  CURRENTPROGRAMSTATE = CONFRONTINGUSER;
  Serial.print(F("Retrieving name of device whose address is "));
  Serial.println(devicexAddr);
  Serial.print(F("->AT+RNAME "));
  Serial.println(devicexAddr);  
  Serial1.print(F("AT+RNAME "));
  Serial1.println(devicexAddr);  
}

void SetBindAddress(){
  CURRENTPROGRAMSTATE = SETTINGBINDADDRESS;
  Serial.print(F("Setting bind address to '"));
  Serial.print(currentDeviceName);
  Serial.print(F("'s address <"));
  Serial.print(currentDeviceAddr);
  Serial.println(F(">"));
  Serial.print(F("->AT+BIND="));
  Serial.println(currentDeviceAddr);
  Serial1.print(F("AT+BIND="));
  Serial1.println(currentDeviceAddr);
}

void LinkToCurrentDevice(String devicexAddr){
  CURRENTPROGRAMSTATE = CONNECTINGTODEVICE;
  Serial.print(F("Very well! Connecting to '"));
  Serial.print(currentDeviceName);
  Serial.println(F("'"));
  Serial.print(F("->AT+LINK="));
  Serial.println(devicexAddr);
  
  Serial1.print(F("AT+LINK="));
  Serial1.println(devicexAddr);
}

void flushOkString(){
  byte inc2 = Serial1.read();
  if(inc2 != -1){
    String okstring = char(inc2) + Serial1.readStringUntil('\n');
    Serial.println(okstring);
  }  
}

void resetAllVariables(){
  SETTINGHC05MODE = false;
  INITIALIZING = false;
  deviceCount = 0;
  recentDeviceCount = 0;
  currentDeviceIdx = 0;
  currentFunctionStep = 0;
  for(int i=0;i<MAX_DEVICES;i++){
    devices[i] = "";
  }
  currentDeviceAddr = "";
  currentDeviceName = "";
}


String getErrorMessage(String errCodeStr){
  char errCodeHex[3];
  Serial.print(F("errCodeStr = "));
  Serial.println(errCodeStr);
  long errCode = strtol(errCodeStr.c_str(),NULL,16);
  return HC05_ERRORMESSAGE[errCode];
}

int inArray(String needle,String* haystack, int len){  
  int index = -1;
  for (int i=0; i<len; i++) {
     if (needle == haystack[i]) {
       index = i;
       break;
     }
  }
  return index;
}

/*
 *  Copyright (c) 2016 John Romano D'Orazio (john.dorazio@cappellaniauniroma3.org)
 *  and Vincenzo d'Orso (icci87@gmail.com)
 * 
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
