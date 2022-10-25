/***************************************************
Primer prototipo desarrollado para el sistema de control
comisionado por Instrumentos Mora SAS.
Desarrollado por: Juan David Castillo S.
Iniciado el 23 de Agosto de 2022

Script planeado para Arduino Uno
 ****************************************************/

// Librerias y definiciones para dibujar en la pantalla
#include <Adafruit_GFX.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_ILI9341.h>
#define TFT_DC 9 // Valor por defecto para DC
#define TFT_CS 10 // Valor por defecto para el pin CS
Adafruit_ILI9341 tftA = Adafruit_ILI9341(TFT_CS, TFT_DC);
uint8_t orientation = 3; // Orientacion de la pantalla

// Librerias y definiciones para detectar el tacto
#include <DmTftSsd2119.h>
#include <DmTftIli9341.h>
#include <DmTftIli9325.h>
#include <DmTouch.h>
#include <utility/DmTouchCalibration.h>
#define SD_CS   8
#define F_CS    6
#define T_CS    4
#define T_IRQ   2
DmTftIli9341 tft = DmTftIli9341();
DmTouch dmTouch = DmTouch(DmTouch::DM_TFT28_105);
DmTouchCalibration calibration = DmTouchCalibration(&tft, &dmTouch);

// Librerias y definiciones para el manejo del RTC
#include <DS3231.h>
RTClib myRTC;

void setup() {
  Serial.begin(57600); // Iniciar comunicacion serial, 57,600 Baud
  Wire.begin(); // Iniciar comunicacion con el RTC

  // Establecer los pines relacionados a la pantalla como salidas de datos
  pinMode(TFT_CS, OUTPUT);
  pinMode(T_CS, OUTPUT);
  pinMode(SD_CS, OUTPUT);
  pinMode(F_CS, OUTPUT);
  digitalWrite(TFT_CS, HIGH);
  digitalWrite(T_CS, HIGH);
  digitalWrite(SD_CS, HIGH);
  digitalWrite(F_CS, HIGH);
  
  tft.init(); // Iniciar controlador de DFRobot
  dmTouch.init(); // Inicializar el control de tacto
  CalibrationMatrix calibrationMatrix = calibration.getDefaultCalibrationData(DmTouch::DM_TFT28_105);
  dmTouch.setCalibrationMatrix(calibrationMatrix); // Cargar calibracion TFT

  tftA.begin(); // Inicializar el display segun Adafruit
  tftA.setRotation(orientation); 
  printMainScreen();
}

void loop() {
  uint16_t x, y = 0;
  bool touched = false;
  
  if (dmTouch.isTouched()) {
    dmTouch.readTouchData(x,y,touched);
    Serial.print(x);
    Serial.print(" ");
    Serial.println(y);
    tftA.fillCircle(y, x, 10, ILI9341_RED);
  }
  else {
    printMainScreen();
  }
}

void printMainScreen() {
  // tftA.fillScreen(ILI9341_BLACK);
  tftA.setTextColor(ILI9341_RED);
  tftA.setCursor(0, 0);
  
  tftA.setTextSize(3);
  tftA.println("MORAEQUIPOS S.A.S");
  
  tftA.setTextColor(ILI9341_YELLOW);
  tftA.setTextSize(1);
  tftA.println("Software ver. 1.0");
  
  tftA.setTextColor(ILI9341_GREEN, ILI9341_BLACK);
  tftA.setTextSize(2);
  DateTime now = myRTC.now();
  tftA.print(now.year(), DEC);
  tftA.print('/');
  tftA.print(now.month(), DEC);
  tftA.print('/');
  tftA.print(now.day(), DEC);
  tftA.print(' ');
  tftA.print(now.hour(), DEC);
  tftA.print(':');
  tftA.print(now.minute(), DEC);
  tftA.print(':');
  tftA.print(now.second(), DEC);
  tftA.println();
  
  tftA.println("Fecha de inicio: ");
  tftA.println("Dias transcurridos: ");
  tftA.println("Volumen producido: ");
  tftA.println("Detector de presion: ON");
  tftA.setTextColor(ILI9341_RED);
  tftA.println("Interruptor: OFF");
  tftA.setTextColor(ILI9341_GREEN);
  tftA.println("Fecha y hora actual: ");
}