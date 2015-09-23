#include <SoftwareSerial.h>
#include <LiquidCrystal.h>
int rs = 11;
int en = 10;
// initialize the display library with the numbers of the interface pins
LiquidCrystal lcd(rs, en, 5, 4, 3, 2);

SoftwareSerial BTSerial(13, 12); // RX | TX
String inputString = "";         // a string to hold incoming data
String gprmc="$GPRMC";
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
    if(inputString.startsWith(gprmc)){
      elaborateValues(inputString);
    }
 }
}

void elaborateValues(String myString){
  int ore,minuti,secondi;
  String oreString,minutiString,secondiString;
  int commaIndex = myString.indexOf(',');
  int secondCommaIndex = myString.indexOf(',', commaIndex+1);
  String secondValue = myString.substring(commaIndex+1, secondCommaIndex);
  ore=((secondValue.substring(0,2)).toInt())+utc;
  minuti=(secondValue.substring(2,4)).toInt();
  secondi=(secondValue.substring(4,6)).toInt();
  oreString+=ore;
  minutiString=secondValue.substring(2,4);
  secondiString=secondValue.substring(4,6);
  String orario=oreString+":"+minutiString+":"+secondiString;
  Serial.println(secondValue);
  Serial.println(orario);
  Serial.println("\n");
  lcd.setCursor(0, 0);
  lcd.print(orario);
}

