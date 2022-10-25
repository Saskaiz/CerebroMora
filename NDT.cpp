#include "NDT.h"

NDT::NDT(int len) {
  maxLength = len;
  input = "";
  input.reserve(maxLength);
  completed = false;
  requestCancel = false;
  newYear = 0;
  newMonth = 0;
  newDay = 0;
  newHour = 0;
  newMinute = 0;
  newSecond = 0;
}

void NDT::addNumber(int n) {
  int len = input.length();
  if (len < maxLength) {
    input = input + String(n);
  }
}

void NDT::rmNumber() {
  int ind = input.length() - 1;
  if (ind >= 0) {
    input.remove(ind);
  }
  else if (ind < 0) {
    requestCancel = true;
  }
}

String NDT::currentInput() {
  return input;
}

void NDT::complete() {
  int len = input.length();
  if (len == maxLength) {
    newYear = input.substring(0, 2).toInt();
    newMonth = input.substring(2, 4).toInt();
    newDay = input.substring(4, 6).toInt();
    newHour = input.substring(6, 8).toInt();
    newMinute = input.substring(8, 10).toInt();
    newSecond = input.substring(10, 12).toInt();
    if (newMonth <= 12 && newDay <= 31 && newHour <= 24 && newSecond <= 59) {
      completed = true;
    }
  }

}

int NDT::retYear() {
  return newYear;
}

int NDT::retMonth() {
  return newMonth;
}

int NDT::retDay() {
  return newDay;
}

int NDT::retHour() {
  return newHour;
}

int NDT::retMinute() {
  return newMinute;
}

int NDT::retSecond() {
  return newSecond;
}

bool NDT::getState() {
  return completed;
}

bool NDT::getCancel() {
  return requestCancel;
}
