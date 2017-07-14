/*********
  Rui Santos
  Complete project details at http://randomnerdtutorials.com
  Arduino IDE example: Examples > Arduino OTA > BasicOTA.ino
*********/

#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <Wire.h>  
#include "SSD1306.h"
#include <Adafruit_ADS1015.h>

SSD1306  display(0x3c, 0, 2); // Initialise the OLED display using Wire library
Adafruit_ADS1115 ads1115(0x48);  // construct an ads1115 at address 0x48
ESP8266WebServer server(80); // Webserver, to watch the temperature from the web on the IP address

float Voltage1 = 0.0;
float Voltage2 = 0.0;
float ZonePrevValue[] = {0.0,0.0,0.0,0.0,0.0,0.0,0.0};
float Zone[] = {0.0,0.0,0.0,0.0,0.0,0.0,0.0};


String webPage = "";
String zone1 = "kitchen door";
String zone6 = "bathroom window";
String result="";




// Replace with your network credentials
const char* ssid = "hoving";
const char* password = "groningen";


void setup() {
  Serial.begin(9600);
  delay(1000);
  
  display.init(); // Initialising the UI will init the display too.
//  display.flipScreenVertically();
  display.clear();
  drawBootText(); 
  display.display();

  webPage += "<h1>ESP8266 Web Server</h1>";
  ads1115.begin();
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    display.clear();
    drawVoltage();
    display.setFont(ArialMT_Plain_10);
    display.drawString(30, 50, "Connecting Failed");
    display.display();
    delay(500);
    ESP.restart();
  }

  ArduinoOTA.onStart([]() {
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  
  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP address: "); Serial.println(WiFi.localIP());
  drawIPAddress();
  
  
  server.on("/", [](){
    //Show the temperatures on a web page
    char temp1Char[20] = "";
    char temp2Char[20] = "";
    dtostrf(Voltage1, 3, 1, temp1Char);
    
    server.send(200, "text/html", webPage + "<br>Zone 1: " + Zone[1] + " - " + result );
  });
  server.begin();
  Serial.println("HTTP server started");
  
}

void loop() {
   ArduinoOTA.handle();
   ESP.wdtDisable();
   display.clear();
   drawVoltage();
   drawIPAddress();
   display.display();
   server.handleClient();
    delay(500);
}

void drawVoltage()
{
    int16_t adc0;  // we read from the ADC, we have a sixteen bit integer as a result
    adc0 = ads1115.readADC_SingleEnded(0);  // Alarm system voltage

    int16_t adcZone = ads1115.readADC_SingleEnded(1);
    Zone[1] = (((float)100*adcZone)/adc0);  //adc0 represents the voltage of the alarm system, adc1 the first zone voltage. we want the percentage of the zone against the system.
    if (ZonePrevValue[1]!=Zone[1]){
      //TODO: send message to mqtt
      ZonePrevValue[1]=Zone[1];
    }
    
//Debug
    Voltage1 = (adc0 * 0.1875)/1000;
    char result1[20] = "";
    dtostrf(Voltage1, 3, 4, result1);
    
    Voltage2 = (adcZone * 0.1875)/1000;
    char result2[20] = "";
    dtostrf(Voltage2, 3, 4, result2);

    if (Zone[1]<47.75) 
      result="all in rest";
    else if (Zone[1]<60.1) 
      result = zone6;
    else if (Zone[1]<82.7) 
      result = zone1;
    else
      result = zone1+"-"+zone6;
    
    
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.setFont(ArialMT_Plain_10);
    display.drawString(0, 0, result1);
    display.drawString(0, 10, result2);
    display.setFont(ArialMT_Plain_16);
    char tempChar[20] = "";
    dtostrf(Zone[1], 3, 1, tempChar);
    display.drawString(0, 20, tempChar);
    display.setFont(ArialMT_Plain_10);
    display.drawString(0, 36, result);

    
  
}

void drawIPAddress() 
{
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.setFont(ArialMT_Plain_10);
    display.drawString(0, 50, "IP:");
    display.drawString(30, 50, WiFi.localIP().toString());
    display.display();
}

void drawBootText() 
{
    //display.setTextAlignment(TEXT_ALIGN_LEFT);
    //display.setFont(ArialMT_Plain_10);
   // display.drawString(0, 0, "------");
  //  display.setFont(ArialMT_Plain_16);
   // display.drawString(0, 10, "Booting");
   // display.setFont(ArialMT_Plain_24);
    //display.drawString(0, 26, "Hello Lucas & Leo");
 //   
    drawVoltage();
    display.setFont(ArialMT_Plain_10);
    display.drawString(30, 50, "Connecting");
}




