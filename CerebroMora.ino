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
// Asignacion de colores
#define BLACK   0x0000
#define BLUE    0x001F
#define RED     0xF800
#define GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF

typedef struct {
  int left;
  int top;
  int width;
  int height;
} rectangle;

// Rotations 0,2 = portrait  : 0->USB=right,upper : 2->USB=left,lower
// Rotations 1,3 = landscape : 1->USB=left,upper  : 3->USB=right,lower

// Orientacion de la pantalla y dimensiones del teclado
const byte rotation = 3; //(0->3)
const byte row = 4;
const byte col = 3;

rectangle rect[col * row];
char *btnTitle[col * row] = {"1", "2", "3", "4", "5", "6", "7", "8", "9", "BK", "0", "OK"};

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

// Libreria para el manejo de la permamencia de los datos.
#include <EEPROM.h>

void setup() {
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
  tftA.setRotation(rotation);
}

void loop() {
  uint16_t x, y = 0;
  bool touched = false;

  if (dmTouch.isTouched()) {
    dmTouch.readTouchData(x, y, touched);
    int cx = 320 - y; // en y se debe compensar la rotacion del display
    int cy = x;
    if (0 < cx < 160 && 190 < cy < 240) { // Se pulso el boton para cambiar la hora actual
     showTimeScreen();
    }
  }
  else {
    printMainScreen();
  }
}

void printMainScreen() {
  tftA.setTextColor(RED);
  tftA.setCursor(0, 0);

  tftA.setTextSize(3);
  tftA.println("MORAEQUIPOS S.A.S");
  tftA.setTextColor(YELLOW);
  tftA.setTextSize(1);
  tftA.println("Software ver. 1.2");
  tftA.println("");

  tftA.setTextColor(GREEN, BLACK);
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
  tftA.println(now.second(), DEC);
  tftA.println("");

  tftA.print("S PRESION OK \t ");
  tftA.println("SWITCH OK");

  tftA.drawRect(0, 190, 160, 50, WHITE);
  tftA.setCursor(10, 210);
  tftA.setTextColor(WHITE, BLACK);
  tftA.print("Cambiar Hora");

  tftA.drawRect(160, 190, 160, 50, WHITE);
  tftA.setCursor(175, 210);
  tftA.setTextColor(WHITE, BLACK);
  tftA.print("Reset Cont.");
}

// Crear grilla para input de numeros
void showTimeScreen()
{
  tftA.fillScreen(BLUE);
  tftA.setTextColor(WHITE, BLUE);
  tftA.setTextSize(3);
  int left, top;
  int l = 10;
  int t = 70;
  int w = 50;
  int h = 35;
  int hgap = 10;
  int vgap = 10;
  byte id = 0;
  for (byte j = 0; j < row; j++)
  {
    for (byte i = 0; i < col; i++)
    {
      left = l + i * (w + vgap);
      top = t + j * (h + hgap);
      rect[id].left = left;
      rect[id].top = top;
      rect[id].width = w;
      rect[id].height = h;
      tftA.drawRect( left, top, w, h, WHITE);
      tftA.setCursor(left + 10, top + 8);
      tftA.print(btnTitle[id]);
      id++;
    }
  }

  tftA.setTextSize(2);
  tftA.setCursor(0, 0);
  tftA.println("Formato de fecha y hora:");
  tftA.println("YYMMDDwhhmmss");
  while (true){}

  
}