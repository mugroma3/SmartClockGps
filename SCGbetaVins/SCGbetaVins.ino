#include <SoftwareSerial.h>
#include <LiquidCrystal.h>
int rs = 11;
int en = 10;
// initialize the display library with the numbers of the interface pins
LiquidCrystal lcd(rs, en, 5, 4, 3, 2);
SoftwareSerial BTSerial(13, 12); // RX | TX


String inputString = "";         // a string to hold incoming data
String GPSCommandString="$GPRMC";
String valueArray[13];
int utc = 2;

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
      elaborateValues(inputString);
    }
 }
}

void elaborateValues(String myString){
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
  
  int hours,minutes,seconds,day,month,year;
  
  String hourString,minuteString,secondString;
  
  //The GPS Unit that we are using for our testing uses NMEA 2.3, so we have eleven commas instead of just ten
  int idxFirstComma = myString.indexOf(',');
  int idxSecondComma = myString.indexOf(',', idxFirstComma+1);
  int idxThirdComma = myString.indexOf(',', idxSecondComma+1);
  int idxFourthComma = myString.indexOf(',', idxThirdComma+1);
  int idxFifthComma = myString.indexOf(',', idxFourthComma+1);
  int idxSixthComma = myString.indexOf(',', idxFifthComma+1);
  int idxSeventhComma = myString.indexOf(',', idxSixthComma+1);
  int idxEigthComma = myString.indexOf(',', idxSeventhComma+1);
  int idxNinthComma = myString.indexOf(',', idxEigthComma+1);
  int idxTenthComma = myString.indexOf(',', idxNinthComma+1);
  int idxEleventhComma = myString.indexOf(',', idxTenthComma+1);

  valueArray[0] = myString.substring(0,idxFirstComma);                          //GPS Command (in this case, $GPRMC)
  valueArray[1] = myString.substring(idxFirstComma+1, idxSecondComma);          //Time of fix (this is an atomic, precise time!)
  valueArray[2] = myString.substring(idxSecondComma+1, idxThirdComma);          //Status
  valueArray[3] = myString.substring(idxThirdComma+1, idxFourthComma);          //Latitude
  valueArray[4] = myString.substring(idxFourthComma+1, idxFifthComma);          //N
  valueArray[5] = myString.substring(idxFifthComma+1, idxSixthComma);           //Longitude
  valueArray[6] = myString.substring(idxSixthComma+1, idxSeventhComma);         //E
  valueArray[7] = myString.substring(idxSeventhComma+1, idxEigthComma);         //Speed
  valueArray[8] = myString.substring(idxEigthComma+1, idxNinthComma);           //Track angle
  valueArray[9] = myString.substring(idxNinthComma+1, idxTenthComma);           //Date
  valueArray[10] = myString.substring(idxTenthComma+1, idxEleventhComma);       //Magnetic variation
  valueArray[11] = myString.substring(idxEleventhComma+1, idxTwelfthComma);     //Signal integrity
  valueArray[12] = myString.substring(idxTwelfthComma+1);                       //Checksum
  
  hours=((valueArray[1].substring(0,2)).toInt())+utc;
 // minutes=(valueArray[1].substring(2,4)).toInt();
 // seconds=(valueArray[1].substring(4,6)).toInt();
  hourString+=hours;
  minuteString=valueArray[1].substring(2,4);
  secondString=valueArray[1].substring(4,6);
  
  String timeString = hourString+":"+minuteString+":"+secondString;
  String dateString = valueArray[9].substring(0,2) + "/" + valueArray[9].substring(2,4) + "/20" + valueArray[9].substring(4,6);
  
  //Serial.println(valueArray[1]);
  //Serial.println(hourString);
  //Serial.println("\n");
  lcd.setCursor(0,0);
  lcd.print(dateString);
  lcd.setCursor(1, 0);
  lcd.print(hourString);
}

