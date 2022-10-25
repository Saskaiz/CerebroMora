// NDT.h
#ifndef NDT_h
#define NDT_h

#include <Arduino.h>

class NDT {
  private:
    int maxLength;
    String input;
    int newYear;
    int newMonth;
    int newDay;
    int newHour;
    int newMinute;
    int newSecond;
    bool completed;
    bool requestCancel;

  public:
    NDT(int len);
    void addNumber(int n);
    void rmNumber();
    String currentInput();
    void complete();
    int retYear();
    int retMonth();
    int retDay();
    int retHour();
    int retMinute();
    int retSecond();
    bool getState();
    bool getCancel();
};

#endif
