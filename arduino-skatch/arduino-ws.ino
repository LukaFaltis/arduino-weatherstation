#define uS_TO_S_FACTOR 1000000  /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP  15        /* Time ESP32 will go to sleep (in seconds) */
// Example testing sketch for various DHT humidity/temperature sensors written by ladyada
// REQUIRES the following Arduino libraries:
// - DHT Sensor Library: https://github.com/adafruit/DHT-sensor-library
// - Adafruit Unified Sensor Lib: https://github.com/adafruit/Adafruit_Sensor


#define DEVICE "ESP32"
#include <WiFiMulti.h>
WiFiMulti wifiMulti;
#include <InfluxDbClient.h>
#include <InfluxDbCloud.h>

                                                                                
                                                                                //Network Password
// WiFi AP SSID
#define WIFI_SSID "<your wifi SSID>"
// WiFi password
#define WIFI_PASSWORD "<your wifi password>"
// InfluxDB v2 server url, e.g. https://eu-central-1-1.aws.cloud2.influxdata.com (Use: InfluxDB UI -> Load Data -> Client Libraries)
#define INFLUXDB_URL "http://10.0.0.5:8086"
// InfluxDB v2 server or cloud API token (Use: InfluxDB UI -> Data -> API Tokens -> <select token>)
#define INFLUXDB_TOKEN "<your token>"
// InfluxDB v2 organization id (Use: InfluxDB UI -> User -> About -> Common Ids )
#define INFLUXDB_ORG "<your org>"
// InfluxDB v2 bucket name (Use: InfluxDB UI ->  Data -> Buckets)
#define INFLUXDB_BUCKET "<bucket name>"

// Set timezone string according to https://www.gnu.org/software/libc/manual/html_node/TZ-Variable.html
// Examples:
//  Pacific Time: "PST8PDT"
//  Eastern: "EST5EDT"
//  Japanesse: "JST-9"
//  Central Europe: "CET-1CEST,M3.5.0,M10.5.0/3"
#define TZ_INFO "CET-1CEST,M3.5.0,M10.5.0/3"

// InfluxDB client instance with preconfigured InfluxCloud certificate
InfluxDBClient client(INFLUXDB_URL, INFLUXDB_ORG, INFLUXDB_BUCKET, INFLUXDB_TOKEN, InfluxDbCloud2CACert);

Point sensor("weather");


#include "Adafruit_SHTC3.h"
#include "Adafruit_SI1145.h"
#include <Adafruit_Sensor.h>
#include "Adafruit_TSL2591.h"
#include <Adafruit_BMP280.h>
#include <Adafruit_MLX90614.h>
#include <Adafruit_INA219.h>


#include <Arduino.h>

#include <WiFi.h>


#include <HTTPClient.h>

#define USE_SERIAL Serial






RTC_DATA_ATTR int bootCount = 0;

Adafruit_SHTC3 shtc3 = Adafruit_SHTC3();
Adafruit_SI1145 uv = Adafruit_SI1145();
Adafruit_TSL2591 tsl = Adafruit_TSL2591(2591);
Adafruit_BMP280 bmp;
Adafruit_MLX90614 mlx = Adafruit_MLX90614();
Adafruit_INA219 ina219;



//TSL2591 config
void configureSensor(void)
{
 tsl.setGain(TSL2591_GAIN_MED);
 tsl.setTiming(TSL2591_INTEGRATIONTIME_300MS);
}



void setup(){
  
  Serial.begin(115200);

  //Increment boot number and print it every reboot
  ++bootCount;
  Serial.println("Boot number: " + String(bootCount));

  //MAIN CODE
  
 Serial.println("Sensors metrics");
  if (! shtc3.begin()) {
    Serial.println("Couldn't find SHTC3");
    while (1) delay(1);
  }
  Serial.println("Found SHTC3 sensor");


  if (! uv.begin()) {
    Serial.println("Didn't find SI1145");
    while (1);
  }

  Serial.println("Found SI1145 sensor");


  if (tsl.begin()) 
  {
    Serial.println(F("Found a TSL2591 sensor"));
  } 
  else 
  {
    Serial.println(F("No sensor found ... check your wiring?"));
    while (1);
  }


  if (! bmp.begin(0x76)) {
    Serial.println("Couldn't find BMP280");
    while (1) delay(1);
  }
  Serial.println("Found BMP280 sensor");


  if (! mlx.begin()) {
    Serial.println("Couldn't find MLX90614");
    while (1) delay(1);
  }
  Serial.println("Found MLX90614 sensor");


  if (! ina219.begin()) {
    Serial.println("Couldn't find INA219");
    while (1) delay(1);
  }
  Serial.println("Found INA219 sensor");

  

   // Setup wifi
  WiFi.mode(WIFI_STA);
  wifiMulti.addAP(WIFI_SSID, WIFI_PASSWORD);

  Serial.print("Connecting to wifi");
  while (wifiMulti.run() != WL_CONNECTED) {
    Serial.print(".");
    delay(100);
  }
  Serial.println("Wifi ok!");

   timeSync(TZ_INFO, "pool.ntp.org", "time.nis.gov");

  if (client.validateConnection())                                   //Check server connection
  {
    Serial.print("Connected to InfluxDB: ");
    Serial.println(client.getServerUrl());
  } 
  else 
  {
    Serial.print("InfluxDB connection failed: ");
    Serial.println(client.getLastErrorMessage());
  }
   
  // Wait a few seconds between measurements.
  //delay(2000);

  //SHTC3
  sensors_event_t humidity, temp;
  shtc3.getEvent(&humidity, &temp);

  //SI1145
  float UV = uv.readVisible();
  float IR = uv.readIR();
  float UVindex = uv.readUV();
  UVindex /= 100.0;

  //TSL2591
  float lum = tsl.getLuminosity(TSL2591_VISIBLE);

  //BMP280
  float pres = bmp.readPressure();

  //MLX90614
  float obTemp = mlx.readObjectTempC();
  float amTemp = mlx.readAmbientTempC();

  //INA219
  float shVolt = ina219.getShuntVoltage_mV();
  float busVolt = ina219.getBusVoltage_V();
  float current = ina219.getCurrent_mA();
  float watt = ina219.getPower_mW();

  sensor.clearFields();
  sensor.addField("temperature", temp.temperature);
  sensor.addField("humidity", humidity.relative_humidity);
  sensor.addField("uv", UV);
  sensor.addField("uvIndex", UVindex);
  sensor.addField("ir", IR);
  sensor.addField("pres", pres);
  sensor.addField("obTemp", obTemp);
  sensor.addField("amTemp", amTemp);
  sensor.addField("Lux", lum);
  sensor.addField("shVolt", shVolt);
  sensor.addField("busVolt", busVolt);
  sensor.addField("current", current);
  sensor.addField("power", watt);

  if (wifiMulti.run() != WL_CONNECTED)                               //Check WiFi connection and reconnect if needed
    Serial.println("Wifi connection lost");

  if (!client.writePoint(sensor))                                    //Write data point
  {
    Serial.print("InfluxDB write failed: ");
    Serial.println(client.getLastErrorMessage());
  }


  String metrics = "temperature,id=dev value=" + String(temp.temperature, 5) + "\n" +
                   "humidity,id=dev value=" + String(humidity.relative_humidity, 1) + "\n" +
                   "uv,id=dev value=" + String(UV, 5) + "\n" +
                   "UVindex,id=dev value=" + String(UVindex, 1) + "\n" +
                   "ir,id=dev value=" + String(IR, 5) + "\n" +
                   "pres,id=dev value=" + String(pres, 5) + "\n" +
                   "obTemp,id=dev value=" + String(obTemp, 5) + "\n" +
                   "amTemp,id=dev value=" + String(amTemp, 5) + "\n" +
                   "Lux,id=dev value=" + String(lum, 5);

 
  /*
  First we configure the wake up source
  We set our ESP32 to wake up every 5 seconds
  */
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  Serial.println("Setup ESP32 to sleep for every " + String(TIME_TO_SLEEP) +
  " Seconds");

  /*
  Now that we have setup a wake cause and if needed setup the
  peripherals state in deep sleep, we can now start going to
  deep sleep.
  In the case that no wake up sources were provided but deep
  sleep was started, it will sleep forever unless hardware
  reset occurs.
  */
  Serial.println("Going to sleep now");
  Serial.flush(); 
  esp_deep_sleep_start();
}

void loop(){
  //This is not going to be called
}
