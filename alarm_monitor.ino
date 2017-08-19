#include <DHT.h>
#include <DHT_U.h>
#include <PubSubClient.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>
#include <WiFiUdp.h>
#include <PubSubClient.h>
#include <ArduinoOTA.h>
#include <Wire.h>  
#include "SSD1306.h"
#include <Adafruit_ADS1015.h>

///I2C
SSD1306  display(0x3c, 0, 2); // Initialise the OLED display using Wire library
Adafruit_ADS1115 ads1115(0x48);  // construct an ads1115 at address 0x48
Adafruit_ADS1115 ads1116(0x49);  // construct an ads1115 at address 0x4B
ESP8266WebServer server(80); // Webserver, to watch the temperature from the web on the IP address

// Replace with your network credentials
const char* ssid = "hoving";
const char* password = "groningen";
const char* mqtt_server = "192.168.1.195";
const char* clientID = "housewatchinator";
const char* outTopic = "housewatchinator/temp";
const char* inTopic = "housewatchinator"; 

WiFiClient espClient;
PubSubClient client(espClient);

// DHT Sensor
#define DHTTYPE DHT11   // DHT 11
const int DHTPin = 4;
DHT dht(DHTPin, DHTTYPE);

float Voltage1 = 0.0;
float Voltage2 = 0.0;
float ZonePrevValue[] = {0.0,0.0,0.0,0.0,0.0,0.0,0.0};
float Zone[] = {0.0,0.0,0.0,0.0,0.0,0.0,0.0};
int counter=1;

String webPage = "";
String zone1 = "low";
String zone6 = "high";
String result[]={"","","","","","",""};

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
  ads1116.begin();
  dht.begin();
  
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

  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

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

    //Relays
    int16_t adcVcc = ads1115.readADC_SingleEnded(0);
    int16_t adcZone4 = ads1116.readADC_SingleEnded(0);
    ads1115.startComparator_SingleEnded(0, adcVcc+500);
    ads1116.startComparator_SingleEnded(0, adcZone4-500);
    ads1115.getLastConversionResults();
    ads1116.getLastConversionResults();
  
     float h = dht.readHumidity();
    // Read temperature as Celsius (the default)
    float t = dht.readTemperature();
    Serial.print("Temperature = ");
    Serial.println(t);
    Serial.print("Humidity = ");
    Serial.println(h);
  
    //Show the temperatures on a web page
    char temp1Char[20] = "";
    char temp2Char[20] = "";
    dtostrf(Voltage1, 3, 1, temp1Char);
    String bodyText =webPage;
    bodyText = bodyText + "adcVcc:" + adcVcc + "<br>";
    bodyText = bodyText + "adcZone4:" + adcZone4 + "<br>";
    
    for (int i=1; i <= 7; i++){
      bodyText = bodyText + "<br>Zone " + i + " : " + Zone[i] + " - " + result[i];
     }
     bodyText = bodyText + "<br>Temperature = ";
     bodyText = bodyText + t;
     bodyText = bodyText + "<br>Humidity = ";
     bodyText = bodyText + h;
     
     server.send(200, "text/html", bodyText );
  });
  server.begin();
  Serial.println("HTTP server started");
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback); 
  
  
}

void loop() {
   ArduinoOTA.handle();
   ESP.wdtDisable();
   display.clear();
   drawVoltage();
   drawIPAddress();
   display.display();
   server.handleClient();
   if (!client.connected()) {
      reconnect();
    }
    client.loop();
  
    delay(500);
}

void drawVoltage()
{     int16_t adc0;  // we read from the ADC, we have a sixteen bit integer as a result
    adc0 = ads1115.readADC_SingleEnded(0);  // Alarm system voltage
    int16_t adcZone;
    for (int i=1; i <= 7; i++){
      if (i<4){
        adcZone = ads1115.readADC_SingleEnded(i);
      }else{
        adcZone = ads1116.readADC_SingleEnded(i-4);
      }
      Zone[i] = (((float)100*adcZone)/adc0);  //adc0 represents the voltage of the alarm system, adc1 the first zone voltage. we want the percentage of the zone against the system.
      if (ZonePrevValue[i]!=Zone[i]){
        //TODO: send message to mqtt
        ZonePrevValue[i]=Zone[i];
      }
    } 

    
    
//Debug
    Voltage1 = (adc0 * 0.1875)/1000;
    char result1[20] = "";
    dtostrf(Voltage1, 3, 4, result1);
    adcZone = ads1116.readADC_SingleEnded(4);
    Voltage2 = (adcZone * 0.1875)/1000;
    char result2[20] = "";
    dtostrf(Voltage2, 3, 4, result2);

    for (int i=1; i <= 7; i++){
        if (Zone[i]<10.00)
          result[i]="Not connected";
        else if (Zone[i]<47.75) 
          result[i]="all in rest";
        else if (Zone[i]<60.1) 
          result[i] = zone6;
        else if (Zone[i]<82.7) 
          result[i] = zone1;
        else if (Zone[i]==100.0)
          result[i] = "ADC not connected";  //ADC not connected
        else
          result[i] = zone1+"-"+zone6;
    }
    
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.setFont(ArialMT_Plain_10);
    display.drawString(0, 0, result1);
    display.drawString(0, 10, result2);
    display.setFont(ArialMT_Plain_16);
    char tempChar[20] = "";
    dtostrf(Zone[1], 3, 1, tempChar);
    display.drawString(0, 20, tempChar);
  //  display.setFont(ArialMT_Plain_10);
  //  display.drawString(0, 36, result[1]);

    
  
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
    drawVoltage();
    display.setFont(ArialMT_Plain_10);
    display.drawString(30, 50, "Connecting");
}

void callback(char* topic, byte* payload, unsigned int length) {
  // Conver the incoming byte array to a string
  payload[length] = '\0'; // Null terminator used to terminate the char array
  String message = (char*)payload;

  Serial.print("Message arrived on topic: [");
  Serial.print(topic);
  Serial.print("], ");
  Serial.println(message);
  webPage = webPage + "Message arrived on topic: [";
  webPage = webPage + topic;
  webPage = webPage + "] " + message + "<br>";
  display.setFont(ArialMT_Plain_10);
  display.drawString(0, 36, topic + '\\' + message);
  display.display();  
  if(message == "temperature, c"){
  
  
    client.publish(outTopic, "30 graden");
  } else if (message == "humidity"){
   
   
   
    client.publish(outTopic, "20 percent");
  }
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    webPage = webPage +"Attempting MQTT connection...<br>";
    // Attempt to connect
    if (client.connect(clientID)) {
      Serial.println("connected");
      webPage = webPage +"connected.<br>";
      // Once connected, publish an announcement...
      client.publish(outTopic, clientID);
      // ... and resubscribe
      client.subscribe(inTopic);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      webPage = webPage +"failed, rc=";
      webPage = webPage + client.state();
      webPage = webPage + " try again in 5 seconds<br>";
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}



