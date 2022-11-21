/*****************
  Primer prototipo desarrollado para el sistema de control
  comisionado por Instrumentos Mora SAS.
  Desarrollado por: Juan David Castillo S.
  Iniciado el 23 de Agosto de 2022

  Script planeado para Arduino Uno
 ******************/

// Librerias necesarias
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <XPT2046_Touchscreen.h>
#include <DS3231.h>
#include <EEPROM.h>
#include "NDT.h"  // Libreria personal para el manejo de la recepcion de fecha y hora

// Definicion de pines utilizados en la PCB
#define CS_PIN 7
#define TFT_RST 8
#define TFT_DC 9
#define TFT_CS 10
#define in_TDS A0
#define out_SW A1
#define out_TDS A2
#define in_encoder 2
#define in_bat 3
#define in_NC 4
#define in_NO 5

// Declaracion de pantalla y sensor tactil
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);
XPT2046_Touchscreen ts(CS_PIN);
DS3231 myRTC;

// Declaracion del keypad para la pantalla de cambio de fecha y hora
const byte row = 4;
const byte col = 3;
char *btnTitle[col * row] = { "1", "2", "3", "4", "5", "6", "7", "8", "9", "BK", "0", "OK" };

// Definiciones para la instrumentacion
bool NO;
bool NC;

unsigned long TDS;

unsigned long memoryTimer;
const unsigned long memoryInterval = 60000;

unsigned long TDSFlushTimer = 0;
const int TDSFlushDuration = 20000;

bool flushing = false;
uint64_t volumen;
uint64_t contador;
float conversion = (1.0 / 3570.0);
DateTime fechaContador;

void setup() {

  Serial.begin(9600);
  Wire.begin();  // Iniciar comunicacion con el RTC
  pinMode(CS_PIN, OUTPUT);
  pinMode(TFT_CS, OUTPUT);
  digitalWrite(CS_PIN, HIGH);
  digitalWrite(TFT_CS, HIGH);
  tft.begin();
  ts.begin();

  tft.setRotation(3);
  ts.setRotation(3);
  tft.fillScreen(ILI9341_BLACK);

  pinMode(in_encoder, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(in_encoder), countVolume, CHANGE);

  pinMode(in_NC, INPUT);
  pinMode(in_NO, INPUT);
  pinMode(in_TDS, INPUT);
  pinMode(out_SW, OUTPUT);
  pinMode(out_TDS, OUTPUT);
  digitalWrite(out_SW, LOW);
  digitalWrite(out_TDS, LOW);

  EEPROM.get(0, contador);
  if (isnan(contador)) {
    contador = 0;
    fechaContador = RTClib::now();
    EEPROM.put(0, contador);
    EEPROM.put(100, fechaContador);
    Serial.println("Nuevo registro iniciado");
  } else {
    EEPROM.get(0, contador);
    EEPROM.get(100, fechaContador);
    Serial.println("Registro existente cargado");
  }
  memoryTimer = millis();
}

void loop() {
  NO = digitalRead(in_NO);
  NC = digitalRead(in_NC);
  TDS = map(analogRead(in_TDS), 0, 1023, 0, 1086);
  // Serial.println(TDS);

  if (millis() - memoryTimer >= memoryInterval) {
    EEPROM.put(0, contador);
    memoryTimer = millis();
    Serial.println("Guardado EEPROM intentado");
  }

  DateTime now = RTClib::now();
  if ((now.hour() == 1 && now.minute() == 59 && now.second() > 55)) {  // (flushing == false && TDS >= 10.0) ||
    TDSFlushTimer = millis();
    flushing = true;
  } else if (flushing == true && TDSFlushTimer + TDSFlushDuration <= millis()) {
    flushing = false;
  }
  digitalWrite(out_TDS, flushing);

  if ((!NO && !NC) || flushing) {  // Ambos circuitos deben estar cerrados para poder prender el rele
    digitalWrite(out_SW, HIGH);
  } else {
    digitalWrite(out_SW, LOW);
  }

  printMainScreen();
  if (ts.touched()) {
    TS_Point p = catchPoint();

    if ((0 < p.x && p.x < 160) && p.y < 50) {  // Se pulso el boton para cambiar la hora actual
      changeTimeScreen();
      tft.fillScreen(ILI9341_BLACK);
    } else if ((160 < p.x && p.x < 320) && p.y < 50) {
      resetContScreen();
      tft.fillScreen(ILI9341_BLACK);
    }
  }
}

void printMainScreen() {
  tft.setTextColor(ILI9341_RED);
  tft.setCursor(0, 0);

  tft.setTextSize(3);
  tft.println("MORAEQUIPOS S.A.S");
  tft.println("");

  tft.setTextColor(ILI9341_GREEN, ILI9341_BLACK);
  tft.setTextSize(2);
  DateTime now = RTClib::now();
  char Buffer[20];
  sprintf(Buffer, "%02u-%02u-%04u  ", now.day(), now.month(), now.year());
  tft.print(Buffer);
  sprintf(Buffer, "%02u:%02u", now.hour(), now.minute());
  tft.print(Buffer);
  if (!NO) {
    tft.println("   PM ON ");
  } else {
    tft.setTextColor(ILI9341_RED, ILI9341_BLACK);
    tft.println("   PM OFF");
    tft.setTextColor(ILI9341_GREEN, ILI9341_BLACK);
  }

  sprintf(Buffer, "TDS: %04u PPM", TDS);
  tft.print(Buffer);
  if (!NC) {
    tft.println("       SP OFF");
  } else {
    tft.setTextColor(ILI9341_RED, ILI9341_BLACK);
    tft.println("       SP ON ");
    tft.setTextColor(ILI9341_GREEN, ILI9341_BLACK);
  }

  if (flushing) {
    tft.setTextColor(ILI9341_YELLOW, ILI9341_BLACK);
    tft.println("VALVULA LAVADO ON");
    tft.setTextColor(ILI9341_GREEN, ILI9341_BLACK);
  } else {
    tft.println("                 ");
  }

  tft.setTextSize(4);
  volumen = contador * conversion;
  sprintf(Buffer, "%06u L", volumen);
  tft.println(Buffer);

  tft.setTextSize(2);
  tft.println("");
  sprintf(Buffer, "DESDE: %02u-%02u-%04u ", fechaContador.day(), fechaContador.month(), fechaContador.year());
  tft.print(Buffer);
  tft.print(' ');
  sprintf(Buffer, "%02u:%02u", fechaContador.hour(), fechaContador.minute());
  tft.println(Buffer);
  tft.println("");

  tft.setTextSize(2);
  tft.drawRect(0, 190, 160, 50, ILI9341_WHITE);
  tft.setCursor(10, 210);
  tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
  tft.print("Cambiar Hora");

  tft.drawRect(160, 190, 160, 50, ILI9341_WHITE);
  tft.setCursor(175, 210);
  tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
  tft.print("Reset Cont.");
}

void changeTimeScreen() {
  tft.fillScreen(ILI9341_BLUE);
  tft.setTextColor(ILI9341_WHITE, ILI9341_BLUE);
  tft.setTextSize(3);

  // Parametros de construccion de los botones
  int left, top;
  int l = 30;
  int t = 60;
  int w = 80;
  int h = 35;
  int hgap = 10;
  int vgap = 10;

  // Dibujo de los botones
  byte id = 0;
  for (byte j = 0; j < row; j++) {
    for (byte i = 0; i < col; i++) {
      left = l + i * (w + vgap);
      top = t + j * (h + hgap);
      tft.drawRect(left, top, w, h, ILI9341_WHITE);
      tft.setCursor(left + 10, top + 8);
      tft.print(btnTitle[id]);
      id++;
    }
  }

  tft.setTextSize(2);
  NDT newDT(12);

  while (!newDT.getState() && !newDT.getCancel()) {
    tft.setCursor(0, 0);
    tft.println("Formato de fecha y hora:");
    tft.println("YYMMDDhhmmss");
    tft.println(newDT.currentInput());

    if (ts.touched()) {
      TS_Point p = catchPoint();
      int cy = 240 - p.y;

      byte id = 0;
      int idwin = -1;
      for (byte j = 0; j < row; j++) {
        for (byte i = 0; i < col; i++) {
          left = l + i * (w + vgap);
          top = t + j * (h + hgap);
          if ((left < p.x && p.x < left + w) && (top < cy && cy < top + h)) {
            idwin = id;
          }
          id++;
        }
      }

      if (idwin >= 0 && idwin <= 8) {
        newDT.addNumber(idwin + 1);
      } else if (idwin == 10) {
        newDT.addNumber(0);
      } else if (idwin == 9) {
        newDT.rmNumber();
        tft.fillRect(0, 0, 320, 60, ILI9341_BLUE);
      } else if (idwin == 11) {
        newDT.complete();
        if (!newDT.getState()) {
          tft.setCursor(0, 0);
          tft.setTextColor(ILI9341_RED, ILI9341_BLUE);
          tft.setTextSize(3);
          tft.fillRect(0, 0, 320, 60, ILI9341_BLUE);
          tft.println("VALOR INVALIDO");
          tft.setTextColor(ILI9341_WHITE, ILI9341_BLUE);
          tft.setTextSize(2);
          delay(3000);
          tft.fillRect(0, 0, 320, 60, ILI9341_BLUE);
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

void resetContScreen() {
  tft.fillScreen(ILI9341_BLUE);
  tft.setTextColor(ILI9341_WHITE, ILI9341_BLUE);
  tft.setTextSize(2);
  tft.setCursor(0, 0);

  DateTime now = RTClib::now();
  char Buffer[20];
  sprintf(Buffer, "ULTIMA FECHA: %02u-%02u-%04u", fechaContador.day(), fechaContador.month(), fechaContador.year());
  tft.println(Buffer);
  sprintf(Buffer, "ULTIMA HORA: %02u:%02u:%02u", fechaContador.hour(), fechaContador.minute(), fechaContador.second());
  tft.println(Buffer);
  sprintf(Buffer, "VOLUMEN: %08u clicks", volumen);
  tft.println(Buffer);
  tft.println("");

  tft.setTextColor(ILI9341_YELLOW, ILI9341_BLUE);
  sprintf(Buffer, "NUEVA FECHA: %02u-%02u-%04u", now.day(), now.month(), now.year());
  tft.println(Buffer);
  sprintf(Buffer, "NUEVA HORA: %02u:%02u:%02u", now.hour(), now.minute(), now.second());
  tft.println(Buffer);
  tft.println("");
  tft.setTextSize(3);
  tft.println("DESEA REINICIAR?");

  tft.drawRect(0, 190, 160, 50, ILI9341_WHITE);
  tft.setCursor(10, 210);
  tft.setTextColor(ILI9341_RED, ILI9341_BLUE);
  tft.print("CANCELAR");

  tft.drawRect(160, 190, 160, 50, ILI9341_WHITE);
  tft.setCursor(175, 210);
  tft.setTextColor(ILI9341_GREEN, ILI9341_BLUE);
  tft.print("ACEPTAR");

  int contAccept = 0;
  bool cancel = false;
  bool accept = false;
  while (contAccept < 3 && !cancel) {
    if (ts.touched()) {
      TS_Point p = catchPoint();

      if ((0 < p.x && p.x < 160) && p.y < 50) {
        cancel = true;
      } else if ((160 < p.x && p.x < 320) && p.y < 50) {
        accept = true;
      }
    } else {
      if (accept) {
        contAccept += 1;
        accept = false;
      }
    }
  }

  if (cancel) {
    return;
  } else if (contAccept >= 3) {
    contador = 0;
    fechaContador = RTClib::now();
    EEPROM.put(0, contador);
    EEPROM.put(100, fechaContador);
  }
}

void countVolume() {
  contador += 1;
}

TS_Point catchPoint() {
  TS_Point p = ts.getPoint();
  long x = map(p.x, 200, 3700, 0, 320);
  long y = map(p.y, 500, 3700, 240, 0);
  TS_Point np = TS_Point(x, y, p.z);
  return np;
}