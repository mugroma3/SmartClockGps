//module max inquire devices, can change to optimize HC-05 connectivity
#define MAX_DEVICES         15


enum HC05MODE { COMMUNICATION_MODE=0, AT_MODE }; //0,1 = LOW,HIGH
enum HC05ROLE { SLAVE=0, MASTER, SLAVE_LOOP };
enum HC05CMODE { CONNECT_BOUND=0, CONNECT_ANY, CONNECT_SLAVE_LOOP };
enum HC05STATE { DISCONNECTED=0, CONNECTED };

//possible states for the current program
enum ProgramState { //typedef not necessary in C++, only in C
  INITIALSTATECHECK = 0,
  DO_ADCN,
  COUNTINGRECENTDEVICES,
  COUNTEDRECENTDEVICES,
  SEARCHAUTHENTICATEDDEVICE,
  CONNECTINGRECENTDEVICE,
  SETTINGCONNECTIONMODE,
  INITIATINGINQUIRY,
  INQUIRINGDEVICES,
  CONFRONTINGUSER,
  SETTINGBINDADDRESS,
  CONNECTINGTODEVICE,
  LISTENNMEA
};

enum LOCALE {
  ENG = 0,
  ITA,
  ESP,
  FRA,
  DEU
};

enum MENUITEM {
  INITIALIZEMENUITEM = 0,
  UTCOFFSETMENUITEM,
  LANGUAGEMENUITEM,
  DATEVIEWMENUITEM,
  VERSIONMENUITEM
};

enum BASEMENU {
  CLOCK = 0,
  CHRONOMETER,
  GPSDATA
};

enum CHRONOSTATE {
  CHRONOINIT = 0,
  CHRONORUNNING,
  CHRONOSTOPPED
};

int dynamicEvent;
int tickEvent;                      // event that will be called once every second to create the local Arduino clock
int firstSynch;                     // event that will be called once after 1 second to synch the local Arduino clock with GPS time data
int synchEvent;                     // event that will be called once every so often to synch the local Arduino clock with GPS time data
int chronometerEvent;               // event that will be called once every 50 milliseconds as a chronometer function

String GPSValueStrings[7];


/**********************************************************************
 * Define Program Level Global variables 
 * (not user changeable, only the program can change their value) 
 * held in RAM memory
 * ********************************************************************
*/

ProgramState OLDPROGRAMSTATE;
ProgramState CURRENTPROGRAMSTATE;
boolean PROGRAMSTATECHANGED;
boolean SETTINGHC05MODE;
boolean INITIALIZING;

HC05MODE HC05_MODE;                   //can be AT_MODE or COMMUNICATION_MODE
HC05CMODE currentCMODE;               //can be CONNECT_BOUND, CONNECT_ANY, or CONNECT_SLAVE_LOOP
HC05STATE HC05_STATE;                 //can be CONNECTED or DISCONNECTED
HC05STATE HC05_OLDSTATE;              //can be CONNECTED or DISCONNECTED
CHRONOSTATE ChronoState;              //can be CHRONOINIT, CHRONORUNNING or CHRONOSTOPPED

LOCALE currentLocale;                 //can be ENG, ITA, ESP, FRA, DEU
BASEMENU BaseMenu;                    //can be CLOCK, CHRONOMETER, GPSDATA
int CURRENTMENUITEM;                  //declaring as int rather than MENUITEM so that we can iterate over the items
int PREVIOUSMENUITEM;                 //declaring as int rather than MENUITEM so that we can iterate over the items
boolean MENUACTIVE;                   //whether user has entered into main menu
int MENULEVEL;                        //current level within main menu
int MENUITEMS;                        //item count in current menu level
int initcount;                        //for iterating over initphase strings
int searchcount;                      //for iterating over searchphase and and linkphase strings

String devices[MAX_DEVICES];          //array of device addresses
int deviceCount;                      //total device addresses found
int currentDeviceIdx;                 //index of iteration through device addresses
String currentDeviceAddr;             //address of device at current index
String currentDeviceName;             //name of device at current index
int recentDeviceCount;                //result of recent paired devices count request
int currentFunctionStep;              //SetHC05Mode function has different steps
String incoming;                      //for reading incoming Serial data (from HC-05 module to Serial monitor)
String outgoing;                      //outgoing Serial data (from user / Serial monitor to HC-05 module)
String GPSCommandString;              //the string that will hold the incoming GPS data stream (one line at a time)
int currentYear;                      //used for creating clock date-time
int currentMonth;                     //used for creating clock date-time
int currentDay;                       //used for creating clock date-time
int currentHour;                      //used for creating clock date-time
int currentMinute;                    //used for creating clock date-time
int currentSecond;                    //used for creating clock date-time
String timeString;                    //final string of the time to display on the LCD
String dateString;                    //final string of the data to display on the LCD
int GPSDataCounter;                   //GPS DATA

unsigned long currentMillis;          //will count the current milliseconds of the Arduino when the chronometer function is activated
unsigned long oldtime;                //useful for filtering button press
unsigned long newtime;                //useful for filtering button press
int MENUBUTTONSTATE;                  //button state LOW / HIGH
int NAVIGATEBUTTONSTATE;              //button state LOW / HIGH
boolean MENUBUTTONPRESSED;            //button press event
boolean NAVIGATEBUTTONPRESSED;        //button press event

int addressIntUTCOffset;
int addressIntLanguage;
int addressIntDateView;
int addressIntSynchFrequency;
float addressFloatVersion;

/****************************************
 * Define User Preference globals (held in RAM memory)
 * TODO: should be stored in EEPROM!
 ****************************************
 */
int offsetUTC;
boolean useLangStrings;

