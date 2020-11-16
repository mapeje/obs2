#include <Arduino.h>

//#define SERIAL_DEBUG_DISABLED

#define USE_LIB_WEBSOCKET true

#include "sensesp_app.h"
#include "sensors/onewire_temperature.h"
#include "signalk/signalk_output.h"
#include "transforms/linear.h"
#include "wiring_helpers.h"
#include "sensors/bmp280.h"
#include "sensors/dhtxx.h"

ReactESP app([]() {
#ifndef SERIAL_DEBUG_DISABLED
  SetupSerialDebug(115200);
#endif

  sensesp_app = new SensESPApp();

  /*
     Find all the sensors and their unique addresses. Then, each new instance
     of OneWireTemperature will use one of those addresses. You can't specify
     which address will initially be assigned to a particular sensor, so if you
     have more than one sensor, you may have to swap the addresses around on
     the configuration page for the device. (You get to the configuration page
     by entering the IP address of the device into a browser.)
  */

  /*
     Tell SensESP where the sensor is connected to the board
     ESP8266 pins are specified as DX
     ESP32 pins are specified as just the X in GPIOX
  */

  DallasTemperatureSensors* dts = new DallasTemperatureSensors(D3);

  // Define how often SensESP should read the sensor(s) in milliseconds [updates every 5s]
  uint engine_read_delay = 5000;

  // Below are temperatures sampled and sent to Signal K server
  // To find valid Signal K Paths that fits your need you look at this link:
  // https://signalk.org/specification/1.4.0/doc/vesselsBranch.html

  // Measure engine temperature
  auto* engine_temp =
      new OneWireTemperature(dts, engine_read_delay, "/Temperature/oneWire");

  engine_temp->connect_to(new Linear(1.0, 0.0, "/Temperature/linear"))
      ->connect_to(
          new SKOutputNumber("propulsion.mainEngine.Temperature",
                             "/Temperature/skPath"));


// Environment
  // Create a BMP280, which represents the physical sensor.
  // 0x77 is the default address. Some chips use 0x76, which is shown here.
  auto* bmp280 = new BMP280(0x76);


  // Define the read_delays you're going to use:
  const uint env_read_delay = 1000;            // once per second
  const uint pressure_read_delay = 60000;  // once per minute

  // Create a BMP280Value, which is used to read a specific value from the
  // BMP280, and send its output to Signal K as a number (float). This one is for
  // the temperature reading.
  auto* bmp_temperature =
      new BMP280Value(bmp280, temperature, env_read_delay, "/Outside/Temperature");

  bmp_temperature->connect_to(
      new SKOutputNumber("environment.outside.temperature"));

  // Do the same for the barometric pressure value. Its read_delay is longer,
  // since barometric pressure can't change all that quickly. It could be much
  // longer for that reason.
  auto* bmp_pressure = new BMP280Value(bmp280, pressure, pressure_read_delay,
                                       "/Outside/Pressure");

  bmp_pressure->connect_to(new SKOutputNumber("environment.outside.pressure"));

// DHT11
#define DHTTYPE DHT11   // DHT 11
#ifdef ESP8266
  uint8_t pin = D4;
#endif

  // Create a DHTxx object, which represents the physical sensor.
  auto* pDHTxx = new DHTxx(pin, DHTTYPE);

  // Define the read_delays you're going to use:
  const uint read_delay = 2000; // once every other second

  // Create a DHTvalue, which is used to read a specific value from the DHTxx sensor,
  // and send its output to SignalK as a number (float). This one is for the temperature reading.
  auto* pDHTtemperature = new DHTvalue(pDHTxx, temperature, read_delay, "Inside/Temperature");
      
      pDHTtemperature->connectTo(new SKOutputNumber("environment.inside.temperature"))

        // Do the same for the humidity value.
  auto* pDHThumidity = new DHTvalue(pDHTxx, humidity,  read_delay, "Inside/Humidity");
      
      pDHThumidity->connectTo(new SKOutputNumber("environment.inside.humidity"));      



  // Configuration is done, lets start the readings of the sensors!
  sensesp_app->enable();
});
