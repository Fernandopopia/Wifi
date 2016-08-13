/* Wifi module
 *  
 *  Version: 1.1
 *  Developed by: Fernando Popia
 *  
 *  Connects the ESP8266 to the network saved in the EPROM memory.
 *  If no network data is saved starts up a webserver for network configuration
 *  GPIO2 is selected as Wifi configuration button (hardware) which is used when the network data needs to be updated
 *  
 *  To be developed:
 *  1 - Save to EPROM Senergy APIKEY
 *  2 - Include power save functionality
*/

#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino

//needed for library
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager
#include <EasyTransfer.h>         //For the serial transfering

//Define GPIO2 as trigger pin for configuration of Wifi
#define TRIGGER_PIN 2

//Create EasyTransfer object
const byte    MaxOnewire=6;
//byte allAddress [MaxOnewire][8];  // 8 bytes per address

EasyTransfer ET; 
struct RECEIVED_DATA_STRUCTURE{
  int16_t power1;
  int16_t power2;
  int16_t power3;
  int16_t power4;
  int16_t Vrms;
  int16_t pulseCount;
  int16_t temp[MaxOnewire];
};
RECEIVED_DATA_STRUCTURE emontx;

//Create server information variables
byte          server[] = {104,41,59,139};
byte          housekeep[] = {104,41,59,139};

//Create flags for debug monitor
boolean       _debug = true;

//Create control variables
int32_t       conn = 0;
int32_t       loss = 0;
int32_t       cycle = 0;
int32_t       ok = 0;
int32_t       nok = 0;

//Create ID/Serial Number varable
int32_t       ID;

//For enabling the serial debug monitor
void setDebug(boolean debug) {
  _debug = debug;
}
template <typename Generic> void useDebug(Generic text){
  if (_debug) {
    Serial.print("*WM: ");
    Serial.println(text);
  }
}

void setup() {
    // put your setup code here, to run once:
    setDebug(0);
    Serial.begin(9600);
    ET.begin(details(emontx), &Serial);

    //Get Chip ID for node reference
    ID = ESP.getChipId();

    //WiFiManager
    //Local intialization. Once its business is done, there is no need to keep it around
    WiFiManager wifiManager;
    //reset saved settings
    //wifiManager.resetSettings();
   
    //sets timeout until configuration portal gets turned off
    //useful to make it all retry or go to sleep
    //in seconds
    wifiManager.setTimeout(180);
    
    //fetches ssid and pass and tries to connect
    //if it does not connect it starts an access point with the specified name
    //here  "Senergy"
    //and goes into a blocking loop awaiting configuration
    if(!wifiManager.autoConnect("Senergy")) {
      useDebug("failed to connect and hit timeout");
      delay(3000);
      //reset and try again, or maybe put it to deep sleep
      ESP.reset();
      delay(5000);
    }    
    
    //if you get here you have connected to the WiFi
    pinMode(TRIGGER_PIN, INPUT);
    useDebug("Connected...");
}

void loop() {
    //Is configuration portal requested?
    if ( digitalRead(TRIGGER_PIN) == LOW ) {
      
      //WiFiManager
      //Local intialization. Once its business is done, there is no need to keep it around
      WiFiManager wifiManager;

      //it starts an access point with the specified name
      //here  "AutoConnectAP"
      //and goes into a blocking loop awaiting configuration
      if (WiFi.status() == WL_CONNECTED) {
          WiFi.disconnect();
      }
      if (!wifiManager.startConfigPortal("SenergyOnDemand")) {
        useDebug("failed to connect and hit timeout");
        delay(3000);
        //reset and try again, or maybe put it to deep sleep
        ESP.reset();
        delay(5000);
      }
      //if you get here you have connected to the WiFi
      useDebug("Connected...");
    }

    //If data is received, start sending data
    if(ET.receiveData()){
      
      WiFiClient client;
      
      cycle++;
      if (WiFi.status() == WL_CONNECTED){
        conn++;
      }
      if (WiFi.status() == WL_CONNECTION_LOST){
        loss++;
      }
      client.setTimeout(3000);
      if (client.connect(server, 80)) {
        useDebug("ENVIANDO AO SERVIDOR");
        client.print("GET /input/post.json?node=");
        client.print(ID);
        client.print("&csv=");
        client.print(emontx.Vrms);
        client.print(",");
        client.print(emontx.power1);
        client.print(",");
        client.print(emontx.power2);
        client.print(",");
        client.print(emontx.power3);
        client.print(",");
        client.print(emontx.power4);
        client.println("&apikey=c7d0738d96dad8a67be04e2333ab361d HTTP/1.1");
        client.println("Host: 104.41.59.139");  
        client.println("User-Agent: Arduino/1.0");
        // last pieces of the HTTP PUT request:
        client.println("Connection: close");
        client.println();
        ok++;
      } 
      else {
        useDebug("connection failed");
        useDebug("disconnecting.");
        client.stop();
        nok++;
      }
      // note the time that the connection was made or attempted:
      int lastConnectionTime = millis();
      delay(10);

      // Read all the lines of the reply from server and print them to Serial
      while(client.available()){
        String line = client.readStringUntil('\r');
        useDebug(line);
      }
      useDebug("closing connection with Host");

      // Access the housekeeping server
      if (client.connect(housekeep, 80)) {
        // send the HTTP PUT request:
        useDebug("ENVIANDO AO SERVIDOR");
        client.print("GET /input/post.json?node=");
        client.print(ID);
        client.print("&csv=");
        client.print(cycle);
        client.print(",");
        client.print(ok);
        client.print(",");
        client.print(nok);
        client.print(",");
        client.print(conn);
        client.print(",");
        client.print(loss);
        client.println("&apikey=aa36fe26b104a466194e13796c6a99b4 HTTP/1.1");
        client.println("Host: 104.41.59.139");  
        client.println("User-Agent: Arduino/1.0");
        // last pieces of the HTTP PUT request:
        client.println("Connection: close");
        client.println();
      } 
      else {
        // if you couldn't make a connection:
        useDebug("connection failed");
        useDebug("disconnecting.");
        client.stop();
    }
      // Read all the lines of the reply from server and print them to Serial
      while(client.available()){
        String line = client.readStringUntil('\r');
        useDebug(line);
      }
      useDebug("closing connection with Housekeeping server");
      }
}



