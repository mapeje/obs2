// updated mqtt topics to align with SignalK

#include <ESP8266WiFi.h>
#include <Wire.h>
#include <PubSubClient.h>

#include "DHT.h"
#include <OneWire.h>
#include <DallasTemperature.h>

#include <Adafruit_Sensor.h>  // include Adafruit sensor library
#include <Adafruit_BMP280.h>  // include adafruit library for BMP280 sensor


//DHT11

DHT dht(D4, DHT11); // (PIN#, DHTTYPE)

//DS18B20

const int oneWireBus = D3;  //GPIO where the DS18B20 is connected to Lolin D1 mini    
OneWire oneWire(oneWireBus); // Setup a 1W instance to communicate with any 1W devices
DallasTemperature sensors(&oneWire); // Pass our 1W reference to Dallas Temperature sensor 

//BMP280
#define BMP280_I2C_ADDRESS  0x76
 
Adafruit_BMP280 bmp280;

//Wifi

#define wifi_ssid "naboo_NoT_24"
#define wifi_password "77NoT77NoT"

#define mqtt_server "obciot.ddns.net"
#define mqtt_user "mqtt"
#define mqtt_password "mqtt"

#define humidity_inside_topic "obs2/environment/inside/relativeHumidity"
#define temperature_inside_topic "obs2/environment/inside/temperature"
#define temperature_engine_topic "obs2/propulsion/main_engine/temperature"
#define temperature_outside_topic "obs2/environment/outside/temperature"
#define pressure_outside_topic "obs2/environment/outside/pressure"


WiFiClient espClient;
PubSubClient client(espClient);

void setup() {
  Serial.begin(115200);
  dht.begin();
  bmp280.begin(BMP280_I2C_ADDRESS);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
}

String macToStr(const uint8_t* mac)
{
  String result;
  for (int i = 0; i < 6; ++i) {
    result += String(mac[i], 16);
    if (i < 5)
      result += ':';
  }
  return result;
  
}


void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(wifi_ssid);

  WiFi.begin(wifi_ssid, wifi_password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
        Serial.print("Attempting MQTT connection...");
    
          // Generate client name based on MAC address and last 8 bits of microsecond counter
      String clientName;  
      clientName += "esp8266-";
      uint8_t mac[6];
      WiFi.macAddress(mac);
      clientName += macToStr(mac);
      clientName += "-";
      clientName += String(micros() & 0xff, 16);
      Serial.print("Connecting to ");
      Serial.print(mqtt_server);
      Serial.print(" as ");
      Serial.println(clientName);


    // Attempt to connect
    // If you do not want to use a username and password, change next line to
  //if (client.connect((char*) clientName.c_str())) {
  if (client.connect((char*) clientName.c_str(), mqtt_user, mqtt_password)) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

// DS18B20 calibration
      float temp =0;
      float RawHigh = 95.0; //Reading from calibration in boiling water
      float RawLow = -1.5; // REading from calibration in ice-bucket
      float ReferenceHigh = 99.9; //boiling water
      float ReferenceLow = 0; //ice-bucket
      float RawRange = RawHigh - RawLow;
      float ReferenceRange = ReferenceHigh - ReferenceLow;

void loop() {
  
      if (!client.connected()) {
        reconnect();
      }
      client.loop();

      // Wait a few seconds between measurements.(20s)
      delay(20000);
      
      // Reading temperature or humidity takes about 250 milliseconds!
      // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
      float h = dht.readHumidity();
      float t = dht.readTemperature();
           
      // DS18B20 Temperature value
      sensors.requestTemperatures(); 
      float temp = sensors.getTempCByIndex(0);
      float tempCorrectedValue = (((temp - RawLow) * ReferenceRange) / RawRange) + ReferenceLow;

      // get temperature, pressure and altitude from library
      float temperature = bmp280.readTemperature();  // get temperature
      float pressure    = bmp280.readPressure();     // get pressure
       

      // Check if any reads failed and exit early (to try again).
      if (isnan(temp) || isnan(h) || isnan(t)|| isnan(temperature)|| isnan(pressure) ) {
      Serial.println("Failed to read from sensor!");
      return;
      }

     
     // Print data on serial
      
      Serial.print("Inside Humidity: ");
      Serial.print(h);
      Serial.print("%\t");
      Serial.print("Inside Temperature: ");
      Serial.print(t);
      Serial.print("ºC\t ");
      Serial.print("Engine temperature: ");
      Serial.print(tempCorrectedValue);
      Serial.print("ºC\t");
      Serial.print("Outside Temperature: ");
      Serial.print(temperature);
      Serial.print("ºC\t ");
      Serial.print("Outside Pressure: ");
      Serial.print(pressure/100);
      Serial.println("hPa\t ");

      // Publish on mqtt
      
      client.publish(temperature_inside_topic, String(t).c_str(), true);
      client.publish(humidity_inside_topic, String(h).c_str(), true);
      client.publish(temperature_engine_topic, String(tempCorrectedValue).c_str(), true);
      client.publish(temperature_outside_topic, String(temperature).c_str(), true);
      client.publish(pressure_outside_topic, String(pressure).c_str(), true);
}
