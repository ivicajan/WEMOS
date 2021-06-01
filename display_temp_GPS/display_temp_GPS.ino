/*********************************************************************
Playground for EPS32 Wemos Lite Micropython module with 
OLED 1,3" display SH1106 together with 
Dallas Temperature DS18B20 and 
Ublox GPS 
*********************************************************************/

#include <SPI.h>
#include <Wire.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <HardwareSerial.h>
#include <TinyGPS++.h>

// Display part, I2C address is 0x3C, connected to 23 and 19 pin
#define OLED_SDA 23
#define OLED_SCL 19
#include <U8g2lib.h>
U8G2_SH1106_128X64_NONAME_F_SW_I2C u8g2(U8G2_R0, /* clock=*/ OLED_SCL, /* data=*/ OLED_SDA, /* reset=*/ U8X8_PIN_NONE);

// Data wire is plugged into digital pin 4
#define ONE_WIRE_BUS 4
// Setup a oneWire instance to communicate with any OneWire device
OneWire oneWire(ONE_WIRE_BUS);  
// Pass oneWire reference to DallasTemperature library
DallasTemperature sensors(&oneWire);

// GPS setup GPIO 16 -> GPS RX , GPIO 17 -> GPS TX
static const int RXPin = 16, TXPin = 17;
static const uint32_t GPSBaud = 9600;
TinyGPSPlus gps;
HardwareSerial ss(1);

#include <Time.h>
const int time_offset = 8*3600;  // Local Time (AWST)

byte last_second, Second, Minute, Hour, Day, Month;
int Year;
double last_lng = NULL;
double last_lat = NULL;
int total_distance;
String tTemp, tSpeed;
String csvOutStr = "";
String tTime, tDate, tDateTime;
String tmonth, tday, thour, tminute, tsecond;
String tLocation, tAge, tDist, tTDist, tSat;
String dataMessage;
bool sdOK, gpsDateOK, gpsLocationOK;
int readingID = 0;
float temp;

void setup()   
{        
  total_distance = 0;
  Serial.begin(115200);
  sensors.begin();  // Start up the library for temp        
  u8g2.begin();     // Start display library
  ss.begin(GPSBaud, SERIAL_8N1, RXPin, TXPin); // GPS init
  delay(50);
}

void loop() 
{
  GetTemp();

  SerialGPSDecode(ss, gps);

  DisplayStuff();
  
  delay(200);
}

// ***************************************************** //

void GetTemp() {
  sensors.requestTemperatures();
  temp = sensors.getTempCByIndex(0);
  if (temp<10) 
  {
  tTemp = "  " + String(temp,0);
  }
  else if ((temp > 10) && (temp<100))
  {
  tTemp = " " + String(temp,0);
  } else {
  tTemp = String(temp,0);
  }
  Serial.println(tTemp); 
}

void DisplayStuff() {
  int y1 = 8;
  int y2 = 50;
  int y3 = 64;
  u8g2.clearBuffer();  
  u8g2.setFont(u8g_font_profont12);
  u8g2.setCursor(1,y1);  
  u8g2.print("S:");
  u8g2.print(tSat);
  u8g2.setCursor(30,y1);  
  u8g2.print("A:");
  u8g2.print(tAge);
  u8g2.setCursor(80,y1);
  u8g2.print(tTime);
  u8g2.setFont(u8g2_font_logisoso42_tf);
  u8g2.setCursor(0,y3);
  u8g2.print(tSpeed);
  u8g2.setFont(u8g2_font_logisoso30_tf);
  u8g2.setCursor(68,y2);
  u8g2.print(tTemp);
  u8g2.setFont(u8g_font_profont12);
/*  
  u8g2.setCursor(65,40);  
  u8g2.print("km");
  u8g2.setCursor(65,50);  
  u8g2.print("h");  
*/
  u8g2.setCursor(123,25);  
  u8g2.print("O");
  u8g2.setCursor(90,y3);
  u8g2.print("TD:");
  u8g2.print(tTDist);
  u8g2.sendBuffer();  
}


void SerialGPSDecode(Stream &mySerial, TinyGPSPlus &myGPS) {
    unsigned long start = millis();
    do
    {
     while (ss.available() > 0)
     gps.encode(ss.read());
    } while (millis() - start < 800);
      gpsDateOK = false;
      gpsLocationOK = false;
       if (gps.date.isValid()) 
        { gpsDateOK = true;
          tSat = String(gps.satellites.value());
          Minute = gps.time.minute();
          Second = gps.time.second();
          Hour   = gps.time.hour();
          Day   = gps.date.day();
          Month = gps.date.month();
          Year  = gps.date.year();
          // set current UTC time
          setTime(Hour, Minute, Second, Day, Month, Year);
          // add the offset to get local time
          adjustTime(time_offset);
          if (month()<10) {
            tmonth = "0" + String(month());
          } else {
            tmonth = String(month());
          }
          if (day()<10) {
            tday = "0" + String(day());
          } else {
            tday = String(day());
          }
          if (hour()<10) {
            thour = "0" + String(hour());
          } else {
            thour = String(hour());
          }
          if (minute()<10) {
            tminute = "0" + String(minute());
          } else {
            tminute = String(minute());
          }
          if (second()<10) {
            tsecond = "0" + String(second());
          } else {
            tsecond = String(second());
          }
          tDate = String(year()) + "/" + tmonth + "/" + tday;
          tTime = thour + ":" + tminute + ":" + tsecond;
          tDateTime = tDate + " " + tTime;
          last_second = gps.time.second();
          Serial.println(tDateTime);
        }
        if (gps.location.isValid()) 
        { gpsLocationOK = true;
          tAge = String(gps.location.age());
          tLocation = String(gps.location.lng(),6) + "," + String(gps.location.lat(),6);
          int speed = gps.speed.kmph();
          tSpeed = String(speed);
        /*  
          if (speed < 10) 
          {
          tSpeed = "  " + String(speed);
          }
          else if ((speed > 10) && (speed <100))
          {
          tSpeed = " " + String(speed);
          } else {
          tSpeed = String(speed);
          }  
        */
          if ((last_lat != NULL))
          { int distance = gps.distanceBetween(gps.location.lat(),gps.location.lng(),last_lat,last_lng);  // in meters
            Serial.println("distance =" + String(distance));
            tDist = String(distance);
            last_lng = gps.location.lng();
            last_lat = gps.location.lat();
           // if ((distance > 2) && (gps.location.age()<500)) {
           // if ((distance > 2)) 
            {
            total_distance += distance;
           }  
          }
        }  
          tTDist= String(total_distance);
          Serial.println("total distance =" + String(total_distance));
          csvOutStr = tDateTime + "," + tLocation + "," + tTemp + "," + tSpeed + "\n";
          if (gpsDateOK == false) {
          tTime = "--:--:--";
          tAge = "--";
          }
          if (gpsLocationOK == false) {
          tSpeed = "---";
          tAge = "--";
          tDist = "--";
          }
}
