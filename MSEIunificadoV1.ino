#include <LoRa.h>
#include "boards.h"

int counter = 0;

#include <TinyGPS++.h>
#include <axp20x.h>

TinyGPSPlus gps;
HardwareSerial GPS(1);
AXP20X_Class axp;

float rumboideal;
double azimut;

//37.95555113466476, -1.227871183213924 ATERRIZAJE
//37.983418404796325, -1.114136464416263 PRUEBA AUDITORIO
const float latitud_destino = 37.95555113466476;  // Latitud del destino
const float longitud_destino = -1.227871183213924;   // Longitud del destino

#include <ESP32Servo.h>
Servo servo1;
Servo servo2;
float angle1;
float angle2;
float X;

#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

#define BME_SCK 13
#define BME_MISO 12
#define BME_MOSI 11
#define BME_CS 10

#define SEALEVELPRESSURE_HPA (1013)//variable, consultar QNH localización

Adafruit_BME280 bme; // I2C
//Adafruit_BME280 bme(BME_CS); // hardware SPI
//Adafruit_BME280 bme(BME_CS, BME_MOSI, BME_MISO, BME_SCK); // software SPI

void setup()
{
  Serial.begin(115200);
  Wire.begin(21, 22);
  if (!axp.begin(Wire, AXP192_SLAVE_ADDRESS)) {
    Serial.println("AXP192 Begin PASS");
  } else {
    Serial.println("AXP192 Begin FAIL");
  }
  axp.setPowerOutPut(AXP192_LDO2, AXP202_ON);
  axp.setPowerOutPut(AXP192_LDO3, AXP202_ON);
  axp.setPowerOutPut(AXP192_DCDC2, AXP202_ON);
  axp.setPowerOutPut(AXP192_EXTEN, AXP202_ON);
  axp.setPowerOutPut(AXP192_DCDC1, AXP202_ON);
  GPS.begin(9600, SERIAL_8N1, 34, 12);   //34-TX 12-RX

    unsigned status;
  
    status = bme.begin();  

  servo1.attach(14);
  servo2.attach(15);
      
      initBoard();
    // When the power is turned on, a delay is required.
    delay(1500);
    LoRa.setPins(RADIO_CS_PIN, RADIO_RST_PIN, RADIO_DIO0_PIN);
    if (!LoRa.begin(LoRa_frequency)) {
        LoRa.print("Starting LoRa failed!");
        while (1);
    }
}

void loop()
{
  float operacion1 = (float) gps.location.lat() * (PI / 180);
  float operacion2 = (float) gps.location.lng() * (PI / 180);
  float operacion3 = (float) latitud_destino * (PI / 180);
  float operacion4 = (float) longitud_destino * (PI / 180);
      
  float incremento_longitud = (float) operacion3 - operacion1;
      
  float x = (sin(incremento_longitud));
  float y = cos(operacion2) * tan(operacion4) - sin(operacion2) * cos(incremento_longitud);
  double azimut = atan2(y, x);
    
  rumboideal = azimut * (180 / PI);
      X = gps.course.deg() - rumboideal; //real menos ideal

      LoRa.beginPacket();
      LoRa.print("MSEI");
      LoRa.print(", ");

      if (X <= 370 && X >= 350, X >= -10 && X <= 10){
        LoRa.print("N");
        LoRa.print(", ");
        servo1.write(75);
        servo2.write(0);
      }
      else if (X >= 180) { //derecha
        LoRa.print("R");
        LoRa.print(", ");
        servo1.write(75);
        servo2.write(75);
      }
      else if (X > 0 && X < 180) { //izquierda
        LoRa.print("L");
        LoRa.print(", ");
        servo2.write(0);
        servo1.write(0);
      }
      else if (X > -180 && X <= 0) { //derecha
        LoRa.print("R");
        LoRa.print(", ");
        servo1.write(75);
        servo2.write(75);
      }
      else if (X <= -180) { //izquierda
        LoRa.print("L");
        LoRa.print(", ");
        servo2.write(0);
        servo1.write(0);
      }

    LoRa.print(rumboideal);
    LoRa.print(", ");
    LoRa.print(X);
    LoRa.print(", ");
  
    LoRa.print(bme.readTemperature());
    LoRa.print(", ");
    LoRa.print(bme.readPressure() / 100.0F);
    LoRa.print(", ");
    LoRa.print(bme.readAltitude(SEALEVELPRESSURE_HPA));
    LoRa.print(", ");

    LoRa.print(gps.location.lat(), 5);
    LoRa.print(", ");
    LoRa.print(gps.location.lng(), 4);
    LoRa.print(", ");
    LoRa.print(gps.satellites.value());
    LoRa.print(", ");
    LoRa.print(gps.altitude.feet() / 3.2808); //metros altura
    LoRa.print(", ");
    LoRa.print(gps.time.hour());
    LoRa.print(":");
    LoRa.print(gps.time.minute());
    LoRa.print(":");
    LoRa.print(gps.time.second());
    LoRa.print(", ");
    LoRa.print(gps.speed.kmph()); 
    LoRa.print(", ");
    LoRa.print(gps.course.deg()); //track

  if (millis() > 5000 && gps.charsProcessed() < 10){
    LoRa.print(", ");
    LoRa.print(F("NO GPS DATA: CHECK WIRING"));}
    smartDelay(1000);
    LoRa.endPacket();
}

static void smartDelay(unsigned long ms)
{
  unsigned long start = millis();
  do
  {
    while (GPS.available())
      gps.encode(GPS.read());
  } while (millis() - start < ms);
}