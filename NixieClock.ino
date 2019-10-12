
// Copyright Rob Latour, 2019, all rights reserved

// Board: DOIT ESP32 DEVKIT V1

// ref: https://nextion.ca/portfolio-items/nextion-iteadlib-and-esp32-step-by-step/
// ref: https://randomnerdtutorials.com/nextion-display-with-arduino-getting-started/
// ref: https://github.com/esp8266/Arduino/blob/master/tools/sdk/libc/xtensa-lx106-elf/include/time.h

#include <time.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include "Nextion.h"

// WIFI connection

// Primary 
const char* wifi_name1 = "****";
const char* wifi_pass1 = "****";

// Optional Backup
const char* wifi_name2 = "****";
const char* wifi_pass2 = "****";


WiFiServer server(80);

// time stuff
const long  timezone   = -5;
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 3600 * timezone;
const int   daylightOffset_sec = 3600;
bool ShowTimeIn24HourFormat = true;
struct tm timeinfo;

int LastSeconds = -1;
int LastDay = -1;

// Nextion display stuff
// use RX2 and TX2 on ESP32 Board

const int LoadingScreen = 0;
const int MenuScreen = 1;
const int DateOrTimeScreen = 2;
const int DateAndTimeScreen = 3;

int ScreenChoosen = 0;  // 0 = loading; 1 = menu; 2 = date only; 3 = time only; 4 = date and time

const int NixieTube0 = 0;
const int NixieTube1 = 1;
const int NixieTube2 = 2;
const int NixieTube3 = 3;
const int NixieTube4 = 4;
const int NixieTube5 = 5;
const int NixieTube6 = 6;
const int NixieTube7 = 7;
const int NixieTube8 = 8;
const int NixieTube9 = 9;
const int NixieTubeOff = 10;
const int NixieTubeColon = 11;
const int NixieTubeSlash = 12;

const int BigNumberOffset = 13;

const int NixieTubeBig0 = 13;
const int NixieTubeBig1 = 14;
const int NixieTubeBig2 = 15;
const int NixieTubeBig3 = 16;
const int NixieTubeBig4 = 17;
const int NixieTubeBig5 = 18;
const int NixieTubeBig6 = 18;
const int NixieTubeBig7 = 20;
const int NixieTubeBig8 = 21;
const int NixieTubeBig9 = 22;
const int NixieTubeBigOff = 23;
const int NixieTubeBigColon = 24;
const int NixieTubeBigSlash = 25;

NexPage NextionLoadingScreen = NexPage(LoadingScreen, 0, "Loading");
NexPage NextionMenuScreen = NexPage(MenuScreen, 0, "Menu");
NexPage NextionDateOrTimeScreen = NexPage(DateOrTimeScreen, 0, "DateOrTime");
NexPage NextionDateAndTimeScreen = NexPage(DateAndTimeScreen, 0, "DateAndTime");

NexHotspot hsOK = NexHotspot(MenuScreen, 1, "btnOK");
NexHotspot hsReset = NexHotspot(MenuScreen, 2, "btnReset");
NexCheckbox cbDate = NexCheckbox(MenuScreen, 3, "cbDate");
NexCheckbox cbTime = NexCheckbox(MenuScreen, 4, "cbTime");
NexCheckbox cb24HourFormat = NexCheckbox(MenuScreen, 5, "cb24HourFormat");

NexPicture BigD1 = NexPicture(DateOrTimeScreen, 1, "p0");
NexPicture BigD2 = NexPicture(DateOrTimeScreen, 2, "p1");
NexPicture BigSeperator = NexPicture(DateOrTimeScreen, 3, "p2");
NexPicture BigD3 = NexPicture(DateOrTimeScreen, 4, "p3");
NexPicture BigD4 = NexPicture(DateOrTimeScreen, 5, "p4");

NexHotspot hsReturnFromDateOrTime = NexHotspot(DateOrTimeScreen, 6, "m0");

NexPicture HourD1 = NexPicture(DateAndTimeScreen, 1, "HourD1");
NexPicture HourD2 = NexPicture(DateAndTimeScreen, 2, "HourD2");
NexPicture Colon01 = NexPicture(DateAndTimeScreen, 3, "Colon01");
NexPicture MinuteD1 = NexPicture(DateAndTimeScreen, 4, "MinuteD1");
NexPicture MinuteD2 = NexPicture(DateAndTimeScreen, 5, "MinuteD2");
NexPicture Colon02 = NexPicture(DateAndTimeScreen, 6, "Colon02");
NexPicture SecondD1 = NexPicture(DateAndTimeScreen, 7, "SecondD1");
NexPicture SecondD2 = NexPicture(DateAndTimeScreen, 8, "SecondD2");
NexPicture MonthD1 = NexPicture(DateAndTimeScreen, 9, "MonthD1");
NexPicture MonthD2 = NexPicture(DateAndTimeScreen, 10, "MonthD2");
NexPicture Slash01 = NexPicture(DateAndTimeScreen, 11, "Colon03");
NexPicture DayD1 = NexPicture(DateAndTimeScreen, 12, "DayD1");
NexPicture DayD2 = NexPicture(DateAndTimeScreen, 13, "DayD2");

NexHotspot hsReturnFromDateAndTime = NexHotspot(DateAndTimeScreen, 14, "m0");

NexTouch *nex_listen_list[] = {
  &hsOK, &hsReset, &hsReturnFromDateOrTime, &hsReturnFromDateAndTime, &cbDate, &cbTime, &cb24HourFormat, NULL
};

uint32_t DateIsChecked;
uint32_t TimeIsChecked;
uint32_t x24HourFormatIsChecked;

// the use of the Override fields is a hack to componsate for the fact that the date, time and 24 hour checkboxes will return false until pressed
boolean OverrideDate;
boolean OverrideTime;
boolean Override24HourFormat;

void ConnectToWifi() {

  bool notyetconnected = true;
  int  counter;

  int TimeBeforeRetryingInSeconds = 3;

  boolean flipflop = true;

  while (notyetconnected)
  {

    //Serial.print("Connecting to WiFi ");

    if (flipflop) {
      WiFi.begin(wifi_name1, wifi_pass1);
      //Serial.println(wifi_name1);
    }
    else
    {
      WiFi.begin(wifi_name2, wifi_pass2);
      //Serial.println(wifi_name2);
    }

    flipflop = !flipflop;

    delay(1000);

    counter = 0;

    while ( ( WiFi.status() != WL_CONNECTED ) && (counter < TimeBeforeRetryingInSeconds) )
    {
      delay (1000);
      // Serial.print(".");
      counter++;
    }

    if ( WiFi.status() == WL_CONNECTED )
    {
      notyetconnected = false;
    }
    else
    {
      TimeBeforeRetryingInSeconds++;

      Serial.println(" (retrying)");

      WiFi.disconnect(true);
      delay(1000);

      WiFi.mode(WIFI_STA);
      delay(1000);
    }

  };

  // Serial.println("Setting time");
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  getLocalTime(&timeinfo);

  // Serial.println("Time set, disconnecting from WiFi");
  WiFi.disconnect(true);

}

void hsOK_Press(void *ptr) {

  // Serial.println("OK pressed");

  cbDate.getValue(&DateIsChecked);
  boolean ShowDate = ( DateIsChecked == 0);

  cbTime.getValue(&TimeIsChecked);
  boolean ShowTime = ( TimeIsChecked == 0);

  cb24HourFormat.getValue(&x24HourFormatIsChecked);
  ShowTimeIn24HourFormat = ( ( x24HourFormatIsChecked == 0) || Override24HourFormat );

  if ( (ShowDate || OverrideDate) && (ShowTime || OverrideTime)) {

    // Serial.println("Date and time choosen");
    ScreenChoosen = 4;
    NextionDateAndTimeScreen.show();
    Slash01.setPic(NixieTubeSlash);
    Colon01.setPic(NixieTubeColon);
    Colon02.setPic(NixieTubeColon);

  }

  else if (ShowDate || OverrideDate) {

    // Serial.println("Date only choosen");
    ScreenChoosen = 2;
    NextionDateOrTimeScreen.show();
    BigSeperator.setPic(NixieTubeBigSlash);

  }
  else if (ShowTime || OverrideTime) {

    // Serial.println("Time only chosen");
    ScreenChoosen = 3;
    NextionDateOrTimeScreen.show();
    BigSeperator.setPic(NixieTubeBigColon);

  }

  LastDay = -1;
  LastSeconds = -1;

}

void hsReset_Press(void *ptr) {

  // Serial.println("Reset pressed");
  ScreenChoosen = 0;
  NextionLoadingScreen.show();

  ConnectToWifi();

  ScreenChoosen = 1;
  NextionMenuScreen.show();

}

void hsReturnFromDateOrTime_Press(void *ptr) {

  // Serial.println("Return from date or time screen");
  ScreenChoosen = 1;
  NextionMenuScreen.show();

}

void hsReturnFromDateAndTime_Press(void *ptr) {

  // Serial.println("Return from date and time screen");
  ScreenChoosen = 1;
  NextionMenuScreen.show();

}

void cbDate_Press(void *ptr) {

  OverrideDate = false;

  cbDate.getValue(&DateIsChecked);

  if (DateIsChecked == 0)
  {
    cbDate.setValue(1);
  } else
  {
    cbDate.setValue(0);
  }

  cbDate.getValue(&DateIsChecked);

}

void cbTime_Press(void *ptr) {

  OverrideTime = false;

  cbTime.getValue(&TimeIsChecked);

  if (TimeIsChecked == 0)
  {
    cbTime.setValue(1);
  } else
  {
    cbTime.setValue(0);
  }

}

void cb24HourFormat_Press(void *ptr) {

  Override24HourFormat = false;

  cb24HourFormat.getValue(&x24HourFormatIsChecked);

  if (x24HourFormatIsChecked == 0)
  {
    cb24HourFormat.setValue(1);
  } else
  {
    cb24HourFormat.setValue(0);
  }

}

void SetupDisplay() {

  // Serial.println("Setting up display");

  while (!nexInit()) {};

  OverrideDate = true;
  OverrideTime = true;
  Override24HourFormat = true;

  // Serial.println("Nextion init ok");

  cbDate.attachPush(cbDate_Press, &cbDate);
  cbTime.attachPush(cbTime_Press, &cbTime);
  cb24HourFormat.attachPush(cb24HourFormat_Press, &cb24HourFormat);
  hsOK.attachPush(hsOK_Press, &hsOK);
  hsReset.attachPush(hsReset_Press, &hsReset);

  hsReturnFromDateOrTime.attachPush(hsReturnFromDateOrTime_Press, &hsReturnFromDateOrTime);
  hsReturnFromDateAndTime.attachPush(hsReturnFromDateAndTime_Press, &hsReturnFromDateAndTime);

};

void setup() {

  // Serial.begin(115200);

  SetupDisplay();

  ConnectToWifi();

  NextionMenuScreen.show();

}

boolean ticktock;

void loop() {

  nexLoop(nex_listen_list);

  switch (ScreenChoosen) {

    case 2:  // date only screen
      {
        getLocalTime(&timeinfo);

        if (timeinfo.tm_mday != LastDay)
        {

          // tm_mon = 0 .. 11 for January ... December
          int xMonthD1 = (timeinfo.tm_mon + 1) / 10;
          int xMonthD2 = (timeinfo.tm_mon + 1) - (xMonthD1 * 10);

          // tm_mday = day in month 1 .. 31
          int xDayD1 = timeinfo.tm_mday / 10;
          int xDayD2 = timeinfo.tm_mday - (xDayD1 * 10);

          xMonthD1 = xMonthD1 + BigNumberOffset;
          xMonthD2 = xMonthD2 + BigNumberOffset;
          xDayD1 = xDayD1 + BigNumberOffset;
          xDayD2 = xDayD2 + BigNumberOffset;

          // update display

          if (xMonthD1 == BigNumberOffset) {
            BigD1.setPic(NixieTubeBigOff);
          } else {
            BigD1.setPic(xMonthD1);
          };
          BigD2.setPic(xMonthD2);

          BigD3.setPic(xDayD1);
          BigD4.setPic(xDayD2);

          LastDay = timeinfo.tm_mday;

        }

        break;
      }

    case 3:  // time only screen

      {

        getLocalTime(&timeinfo);

        if (timeinfo.tm_sec != LastSeconds)
        {

          // tm_hour = 0 to 23

          int xHourD1;
          int xHourD2;

          if ( ShowTimeIn24HourFormat ) {}
          else {

            if (timeinfo.tm_hour == 0 ) {
              timeinfo.tm_hour = 12;
            }
            else
            {
              if (timeinfo.tm_hour > 12 ) {
                timeinfo.tm_hour = timeinfo.tm_hour - 12;
              }
            }
          }

          xHourD1 = timeinfo.tm_hour / 10;
          xHourD2 = timeinfo.tm_hour - (xHourD1 * 10);

          int xMinuteD1 = timeinfo.tm_min / 10;
          int xMinuteD2 = timeinfo.tm_min - (xMinuteD1 * 10);

          xHourD1 = xHourD1 + BigNumberOffset;
          xHourD2 = xHourD2 + BigNumberOffset;
          xMinuteD1 = xMinuteD1 + BigNumberOffset;
          xMinuteD2 = xMinuteD2 + BigNumberOffset;

          // update display

          if (xHourD1 == BigNumberOffset) {
            BigD1.setPic(NixieTubeBigOff);
          } else {
            BigD1.setPic(xHourD1);
          };
          BigD2.setPic(xHourD2);

          BigD3.setPic(xMinuteD1);
          BigD4.setPic(xMinuteD2);

          if (ticktock) {
            ticktock = false;
            BigSeperator.setPic(NixieTubeBigColon);
          } else {
            BigSeperator.setPic(NixieTubeBigOff);
            ticktock = true;
          }

          LastSeconds = timeinfo.tm_sec;

        }

        break;

      }


    case 4:  // date and time screen

      {

        getLocalTime(&timeinfo);

        if (timeinfo.tm_sec != LastSeconds)
        {

          // tm_mon = 0 .. 11 for January ... December
          int xMonthD1 = (timeinfo.tm_mon + 1) / 10;
          int xMonthD2 = (timeinfo.tm_mon + 1) - (xMonthD1 * 10);

          // tm_mday = day in month 1 .. 31
          int xDayD1 = timeinfo.tm_mday / 10;
          int xDayD2 = timeinfo.tm_mday - (xDayD1 * 10);

          // tm_hour = 0 to 23

          int xHourD1;
          int xHourD2;

          if ( ShowTimeIn24HourFormat ) {}
          else
          {

            if (timeinfo.tm_hour == 0 ) {
              timeinfo.tm_hour = 12;
            }
            else
            {
              if (timeinfo.tm_hour > 12 ) {
                timeinfo.tm_hour = timeinfo.tm_hour - 12;
              }
            }
          }

          xHourD1 = timeinfo.tm_hour / 10;
          xHourD2 = timeinfo.tm_hour - (xHourD1 * 10);

          int xMinuteD1 = timeinfo.tm_min / 10;
          int xMinuteD2 = timeinfo.tm_min - (xMinuteD1 * 10);

          int xSecondD1 = timeinfo.tm_sec / 10;
          int xSecondD2 = timeinfo.tm_sec - (xSecondD1 * 10);

          // update display

          if (xMonthD1 == 0) {
            MonthD1.setPic(NixieTubeOff);
          } else {
            MonthD1.setPic(xMonthD1);
          };
          MonthD2.setPic(xMonthD2);

          DayD1.setPic(xDayD1);
          DayD2.setPic(xDayD2);

          if (xHourD1 == 0) {
            HourD1.setPic(NixieTubeOff);
          } else {
            HourD1.setPic(xHourD1);
          };
          HourD2.setPic(xHourD2);

          MinuteD1.setPic(xMinuteD1);
          MinuteD2.setPic(xMinuteD2);

          SecondD1.setPic(xSecondD1);
          SecondD2.setPic(xSecondD2);

          LastSeconds = timeinfo.tm_sec;

        }

        break;

      }

  }

}
