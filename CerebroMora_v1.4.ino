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

// Rotations 0,2 = portrait  : 0->USB=right,upper : 2->USB=left,lower
// Rotations 1,3 = landscape : 1->USB=left,upper  : 3->USB=right,lower
// Orientacion de la pantalla y dimensiones del teclado
const byte rotation = 3; //(0->3)

// Definiciones para la pantalla de cambio de tiempo
const byte row = 4;
const byte col = 3;
char *btnTitle[col * row] = {"1", "2", "3", "4", "5", "6", "7", "8", "9", "BK", "0", "OK"};
#include "NDT.h"

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
DS3231 myRTC;

// Libreria y definiciones para el manejo de la permamencia de los datos.
#include <EEPROM.h>
#include <BigNumber.h>

const byte eeAddress = 0;
struct volumeRecord {
  DateTime dateStarted;
  BigNumber clicks;
};
volumeRecord currentRecord;
bool recordExists;

// Definiciones para la instrumentacion
const byte TDSPin = A0;
const byte encoderPin = 3;
const byte NOPin = 5;
const byte NCPin = 7;
bool NO;
bool NC;
unsigned long TDS;
unsigned long volumeCount;

void setup() {
  Wire.begin(); // Iniciar comunicacion con el RTC

  // Establecer los pines relacionados a la pantalla como salidas de datos
  pinMode(TFT_CS, OUTPUT);
  pinMode(T_CS, OUTPUT);
  pinMode(SD_CS, OUTPUT);
  pinMode(F_CS, OUTPUT);
  pinMode(encoderPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(encoderPin), countVolume, CHANGE);
  pinMode(NOPin, INPUT);
  pinMode(NCPin, INPUT);
  digitalWrite(TFT_CS, HIGH);
  digitalWrite(T_CS, HIGH);
  digitalWrite(SD_CS, HIGH);
  digitalWrite(F_CS, HIGH);

  tft.init(); // Iniciar controlador de DFRobot
  dmTouch.init(); // Inicializar el control de tacto
  CalibrationMatrix calibrationMatrix = calibration.getDefaultCalibrationData(DmTouch::DM_TFT28_105);
  dmTouch.setCalibrationMatrix(calibrationMatrix); // Cargar calibracion TFT

  tftA.begin(); // Inicializar el display segun Adafruit
  tftA.setRotation(rotation);
}

void loop() {
  NO = digitalRead(NOPin);
  NC = digitalRead(NCPin);
  TDS = analogRead(TDSPin) * 0.1127;

  printMainScreen();
  uint16_t x, y = 0;
  bool touched = false;
  if (dmTouch.isTouched()) {
    dmTouch.readTouchData(x, y, touched);
    int cx = 320 - y; // en y se debe compensar la rotacion del display
    int cy = x;
    if (0 < cx < 160 && 190 < cy < 240) { // Se pulso el boton para cambiar la hora actual
      changeTimeScreen();
      tftA.fillScreen(BLACK);
    }
    if (0 < cx < 160 && 190 < cy < 240) { // Se pulso el boton para cambiar la hora actual
      changeTimeScreen();
      tftA.fillScreen(BLACK);
    }
  }
}

void printMainScreen() {
  tftA.setTextColor(RED);
  tftA.setCursor(0, 0);

  tftA.setTextSize(3);
  tftA.println("MORAEQUIPOS S.A.S");
  tftA.setTextColor(YELLOW);
  tftA.setTextSize(1);
  tftA.println("Software ver. 1.3");
  tftA.println("");

  tftA.setTextColor(GREEN, BLACK);
  tftA.setTextSize(2);
  DateTime now = RTClib::now();
  char Buffer[20];
  sprintf(Buffer, "%02u-%02u-%04u ", now.day(), now.month(), now.year());
  tftA.print(Buffer);
  tftA.print(' ');
  sprintf(Buffer, "%02u:%02u:%02u ", now.hour(), now.minute(), now.second());
  tftA.println(Buffer);
  tftA.println("");

  if (NC) {
    tftA.println("SENSOR PRESION MINIMA ON ");
  }
  else {
    tftA.setTextColor(RED, BLACK);
    tftA.println("SENSOR PRESION MINIMA OFF");
    tftA.setTextColor(GREEN, BLACK);
  }
  if (!NO) {
    tftA.println("SENSOR SOBREPRESION OFF");
  }
  else {
    tftA.setTextColor(RED, BLACK);
    tftA.println("SENSOR SOBREPRESION ON ");
    tftA.setTextColor(GREEN, BLACK);
  }
  tftA.println("");
  sprintf(Buffer, "TDS: %04u PPM", TDS);
  tftA.println(Buffer);

  tftA.println("");
  sprintf(Buffer, "VOLUMEN: %08u clicks", volumeCount);
  tftA.println(Buffer);

  tftA.drawRect(0, 190, 160, 50, WHITE);
  tftA.setCursor(10, 210);
  tftA.setTextColor(WHITE, BLACK);
  tftA.print("Cambiar Hora");

  tftA.drawRect(160, 190, 160, 50, WHITE);
  tftA.setCursor(175, 210);
  tftA.setTextColor(WHITE, BLACK);
  tftA.print("Reset Cont.");
}

void changeTimeScreen()
{
  tftA.fillScreen(BLUE);
  tftA.setTextColor(WHITE, BLUE);
  tftA.setTextSize(3);

  // Parametros de construccion de los botones
  int left, top;
  int l = 10;
  int t = 70;
  int w = 50;
  int h = 35;
  int hgap = 10;
  int vgap = 10;

  // Dibujo de los botones
  byte id = 0;
  for (byte j = 0; j < row; j++)
  {
    for (byte i = 0; i < col; i++)
    {
      left = l + i * (w + vgap);
      top = t + j * (h + hgap);
      tftA.drawRect( left, top, w, h, WHITE);
      tftA.setCursor(left + 10, top + 8);
      tftA.print(btnTitle[id]);
      id++;
    }
  }

  tftA.setTextSize(2);
  NDT newDT(12);

  while (!newDT.getState() && !newDT.getCancel()) {
    tftA.setCursor(0, 0);
    tftA.println("Formato de fecha y hora:");
    tftA.println("YYMMDDhhmmss");
    tftA.println(newDT.currentInput());

    uint16_t x, y = 0;
    bool touched = false;
    if (dmTouch.isTouched()) {
      dmTouch.readTouchData(x, y, touched);
      int cx = 320 - y; // en y se debe compensar la rotacion del display
      int cy = x;

      byte id = 0;
      int idwin = -1;
      for (byte j = 0; j < row; j++)
      {
        for (byte i = 0; i < col; i++)
        {
          left = l + i * (w + vgap);
          top = t + j * (h + hgap);
          if ((left < cx && cx < left + w) && (top < cy && cy < top + h)) {
            idwin = id;
          }
          id++;
        }
      }

      if (idwin <= 8) {
        newDT.addNumber(idwin + 1);
      }
      else if (idwin == 10) {
        newDT.addNumber(0);
      }
      else if (idwin == 9) {
        newDT.rmNumber();
        tftA.fillRect( 0, 0, 320, 60, BLUE);
      }
      else if (idwin == 11) {
        newDT.complete();
        if (!newDT.getState()) {
          tftA.setCursor(0, 0);
          tftA.setTextColor(RED, BLUE);
          tftA.setTextSize(3);
          tftA.fillRect( 0, 0, 320, 60, BLUE);
          tftA.println("VALOR INVALIDO");
          tftA.setTextColor(WHITE, BLUE);
          tftA.setTextSize(2);
          delay(3000);
          tftA.fillRect( 0, 0, 320, 60, BLUE);
        }
      }
    }

  }

  if (!newDT.getCancel()) {
    myRTC.setYear(newDT.retYear());
    myRTC.setMonth(newDT.retMonth());
    myRTC.setDate(newDT.retDay());
    myRTC.setHour(newDT.retHour());
    myRTC.setMinute(newDT.retMinute());
    myRTC.setSecond(newDT.retSecond());
  }
}

void countVolume() {
  volumeCount = volumeCount + 1;
}
