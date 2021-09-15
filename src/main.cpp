#include <Arduino.h>

#include <Adafruit_SCD30.h>
#include <M5Stack.h>
#include "Free_Fonts.h"

#include <WiFi.h>
#include <WiFiMulti.h>
#include <ESPmDNS.h>
#include <time.h>

// SCD30から値を読み込むサイクル（秒）
#define PERIOD 5

#include "Ambient.h"
Ambient ambient;  // ambientの接続先設定は WiFiConnect.h ファイルに書いてある。

// Ambientへのデータ送信間隔は最低5秒ごと，1日に最大3,000 平均すると28.8秒に1回の
// https://ambidata.io/refs/spec/
#define AMBIENT_SEND_SPAN (30 / PERIOD)
short ambient_send_span_counter = 0;

void WiFiConeect(WiFiMulti &wifiMulti);
#include "WiFiConnect.h"
/*
// WiFiConnect.h ファイルの例
    void WiFiConeect(WiFiMulti& wifiMulti)
    {
        wifiMulti.addAP("SSID1", "PASS1");
        wifiMulti.addAP("SSID2", "PASS2");
    }
    unsigned int channelId = 0000; // AmbientのチャネルID
    const char* writeKey = "XXXXXX"; // ライトキー
*/
WiFiMulti wifiMulti;
WiFiClient client;
Adafruit_SCD30 scd30;

bool bInitTime = false;
//#include "FS.h"
//#include <SPIFFS.h>

// 画面下の履歴のグラフ表示の設定
#define CO2_LOG_G_AREA_WIDTH 320
#define CO2_LOG_COUNT (CO2_LOG_G_AREA_WIDTH - 2)
#define CO2_LOG_G_AREA_HIGHT 40
#define CO2_LOG_G_MAX_HIGHT (CO2_LOG_G_AREA_HIGHT - 2)
#define CO2_LOG_G_AREA_HIGHT_CO2_VAL 3000.

#define CO2_LOG_G_AREA_TOP 197

#define LCD_HIGH_LEVEL 150
#define LCD_LOW_LEVEL 5

template <typename T>
void sxprint(const T &str)
{
  Serial.print(str);
  M5.Lcd.print(str);
}
void sxprintln(const char *str)
{
  Serial.println(str);
  M5.Lcd.println(str);
}

/*
void listDir(fs::FS &fs, const char * dirname, uint8_t levels){
    Serial.printf("Listing directory: %s\n", dirname);
 
    File root = fs.open(dirname);
    if(!root){
        Serial.println("Failed to open directory");
        return;
    }
    if(!root.isDirectory()){
        Serial.println("Not a directory");
        return;
    }
 
    File file = root.openNextFile();
    while(file){
        if(file.isDirectory()){
            Serial.print("  DIR : ");
            Serial.println(file.name());
            if(levels){
                listDir(fs, file.name(), levels -1);
            }
        } else {
            Serial.print("  FILE: ");
            Serial.print(file.name());
            Serial.print("  SIZE: ");
            Serial.println(file.size());
        }
        file = root.openNextFile();
    }
}
*/




void setup(void)
{
  M5.begin(true, false, false, false);
  M5.Power.begin();
  M5.update();

  M5.Lcd.setBrightness(LCD_HIGH_LEVEL);
  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.clear(BLACK);

  M5.Lcd.setCursor(0, 0);
  M5.Lcd.setTextSize(2);

  Serial.begin(115200);
  while (!Serial)
    delay(10); // will pause Zero, Leonardo, etc until serial console opens

  /*
  if (!SPIFFS.begin()) {
    Serial.println("SPIFFS Mount Failed");
//    return;
  }
  listDir(SPIFFS, "/", 0); //SPIFFSフラッシュ　ルートのファイルリスト表示
 
  SPIFFS.end();
*/
  //  sxprintln("Adafruit SCD30 Sensor adjustment test!");

  // Try to initialize!
  if (!scd30.begin())
  {
    sxprintln("Failed to find SCD30 chip");
    while (1)
    {
      delay(10);
    }
  }
  sxprintln("SCD30 Found!");

  /***
   * The code below will report the current settings for each of the
   * settings that can be changed. To see how they work, uncomment the setting
   * code above a status message and adjust the value
   *
   * **Note:** Since Automatic self calibration and forcing recalibration with
   * a reference value overwrite each other, you should only set one or the other
  ***/

  /*** Adjust the rate at which measurements are taken, from 2-1800 seconds */
   if (!scd30.setMeasurementInterval(PERIOD)) {
     Serial.println("Failed to set measurement interval");
     //while(1){ delay(10);}
  }
  sxprint("Measurement interval: ");
  sxprint(scd30.getMeasurementInterval());
  sxprintln(" seconds");

  /*** Restart continuous measurement with a pressure offset from 700 to 1400 millibar.
   * Giving no argument or setting the offset to 0 will disable offset correction
   */
  // if (!scd30.startContinuousMeasurement(15)){
  //   Serial.println("Failed to set ambient pressure offset");
  //   while(1){ delay(10);}
  // }
  sxprint("Ambient pressure offset: ");
  sxprint(scd30.getAmbientPressureOffset());
  sxprintln(" mBar");

  /*** Set an altitude offset in meters above sea level.
   * Offset value stored in non-volatile memory of SCD30.
   * Setting an altitude offset will override any pressure offset.
   */
  // if (!scd30.setAltitudeOffset(110)){
  //   Serial.println("Failed to set altitude offset");
  //   while(1){ delay(10);}
  // }
  sxprint("Altitude offset: ");
  sxprint(scd30.getAltitudeOffset());
  sxprintln(" meters");

  /*** Set a temperature offset in hundredths of a degree celcius.
   * Offset value stored in non-volatile memory of SCD30.
   */
  // if (!scd30.setTemperatureOffset(1984)){ // 19.84 degrees celcius
  //   Serial.println("Failed to set temperature offset");
  //   while(1){ delay(10);}
  // }
  sxprint("Temperature offset: ");
  sxprint((float)scd30.getTemperatureOffset() / 100.0);
  sxprintln(" degrees C");

  /*** Force the sensor to recalibrate with the given reference value
   * from 400-2000 ppm. Writing a recalibration reference will overwrite
   * any previous self calibration values.
   * Reference value stored in non-volatile memory of SCD30.
   */
  // if (!scd30.forceRecalibrationWithReference(400)){
  //   Serial.println("Failed to force recalibration with reference");
  //   while(1) { delay(10); }
  // }
  sxprint("Forced Recalibration reference: ");
  sxprint(scd30.getForcedCalibrationReference());
  sxprintln(" ppm");

  /*** Enable or disable automatic self calibration (ASC).
   * Parameter stored in non-volatile memory of SCD30.
   * Enabling self calibration will override any previously set
   * forced calibration value.
   * ASC needs continuous operation with at least 1 hour
   * 400ppm CO2 concentration daily.
   */
  // if (!scd30.selfCalibrationEnabled(true)){
  //   Serial.println("Failed to enable or disable self calibration");
  //   while(1) { delay(10); }
  // }
  if (scd30.selfCalibrationEnabled())
  {
    sxprint("Self calibration enabled");
  }
  else
  {
    sxprint("Self calibration disabled");
  }

  sxprintln("");

  // 最初は値が不正確？なので，ダミー読み込み
  if (scd30.dataReady())
  {
    if (!scd30.read())
    {
    }
  }
  sxprint("WiFi connecting.");
  WiFiConeect(wifiMulti); // このWiFi接続関数は　WiFiConnect.h ファイルに書いてある

  M5.update();

  for (int i = 0; i < 15 && (wifiMulti.run() != WL_CONNECTED); i++)
  {
    for(int j = 0; j < 10; j++){
      delay(50);
      // ボタンを押すとWiFi接続キャンセル
      if (M5.BtnA.isPressed() + M5.BtnB.isPressed() + M5.BtnC.isPressed() >= 1)
      {
        sxprintln("cancel.");
        goto WiFiCancel;
      }
      M5.update();
    }
    sxprint(".");

  }


  if (WiFi.status() == WL_CONNECTED)
  {
    sxprintln("OK.");
    ambient.begin(channelId, writeKey, &client);

    sxprint("IP:");
    sxprint(WiFi.localIP());
    if (MDNS.begin("co2"))
    {
      sxprint("( co2.local )");
    }
    sxprintln("");
    configTime(9 * 3600L, 0, "ntp.nict.jp", "time.google.com", "ntp.jst.mfeed.ad.jp"); //NTP
    bInitTime = true;

  }
  else
  {
    sxprintln("Failed.");
  }

WiFiCancel:
  M5.update();

  delay(2000);
  M5.Lcd.clear(BLACK);

  // title bar
  M5.Lcd.setFreeFont(NULL); // Select the font
  M5.Lcd.fillRect(0, 0, 320, 15, BLUE);
  M5.Lcd.drawString("CO2 sensor", 0, 0, GFXFF); // Print the string name of the font

  M5.Lcd.setTextColor(WHITE, BLACK);

  // co2 label
  M5.Lcd.setFreeFont(NULL);                // Select the font
  M5.Lcd.drawString("CO2", 35, 31, GFXFF); // Print the string name of the font

  // Temperature label
  M5.Lcd.setFreeFont(NULL);                              // Select the font
  M5.Lcd.drawString("Temperature", 48 - 16, 133, GFXFF); // Print the string name of the font

  // Humidity label
  M5.Lcd.setFreeFont(NULL);                       // Select the font
  M5.Lcd.drawString("Humidity", 183, 133, GFXFF); // Print the string name of the font


  M5.Lcd.drawRect(0, CO2_LOG_G_AREA_TOP - 1, CO2_LOG_G_AREA_WIDTH, CO2_LOG_G_AREA_HIGHT + 1, TFT_WHITE);
}

char co2_log[CO2_LOG_COUNT];
short co2_log_index = 0;

void loop()
{
  int t = millis();

  if (scd30.dataReady())
  {

    if (!scd30.read())
    {
      sxprintln("Error reading sensor data");
      return;
    }

    // log bar;
    {
      int level = (int)(scd30.CO2 * CO2_LOG_G_MAX_HIGHT / CO2_LOG_G_AREA_HIGHT_CO2_VAL); // MAX 3000ppmで高さ
      if (level >= CO2_LOG_G_MAX_HIGHT)
      {
        level = CO2_LOG_G_MAX_HIGHT;
      }
      co2_log[co2_log_index] = level;
      int count = 0;
      int i = co2_log_index;
      for (count = 0; count < CO2_LOG_COUNT; count++)
      {

        level = co2_log[i];
        int color;
        if (level <= (int)(1000 * CO2_LOG_G_MAX_HIGHT / CO2_LOG_G_AREA_HIGHT_CO2_VAL))
        {
          color = BLUE;
        }
        else if (level <= (int)(2500 * CO2_LOG_G_MAX_HIGHT / CO2_LOG_G_AREA_HIGHT_CO2_VAL))
        {
          color = YELLOW;
        }
        else
        {
          color = RED;
        }

        M5.Lcd.drawLine(
            (CO2_LOG_G_AREA_WIDTH - 2) - count, CO2_LOG_G_AREA_TOP,
            (CO2_LOG_G_AREA_WIDTH - 2) - count, CO2_LOG_G_AREA_TOP + CO2_LOG_G_MAX_HIGHT - level,
            BLACK);

        M5.Lcd.drawLine(
            (CO2_LOG_G_AREA_WIDTH - 2) - count, CO2_LOG_G_AREA_TOP + CO2_LOG_G_MAX_HIGHT - level,
            (CO2_LOG_G_AREA_WIDTH - 2) - count, CO2_LOG_G_AREA_TOP + CO2_LOG_G_MAX_HIGHT,
            color);

        i++;
        if (i >= CO2_LOG_COUNT)
        {
          i = 0;
        }
      }

      co2_log_index--;
      if (co2_log_index < 0)
      {
        co2_log_index = CO2_LOG_COUNT - 1;
      }
    }

    // co2 val
    {
      char buf[20];
      M5.Lcd.setFreeFont(FSS12);
      sprintf(buf, "%04.2fppm     .", scd30.CO2);
      M5.Lcd.drawString(buf, 35, 54, GFXFF);
    }

    // co2 level bar
    {
      uint16_t width = (int)(scd30.CO2 / 13.6363);
      uint16_t color;
      if (scd30.CO2 <= 1000.0)
      {
        color = BLUE;
      }
      else if (scd30.CO2 <= 2500.0)
      {
        color = YELLOW;
      }
      else
      {
        color = RED;
      }
      M5.Lcd.fillRect(33, 104, width, 10, color);
      M5.Lcd.drawRect(33, 104, width, 10, WHITE);
      M5.Lcd.fillRect(33 + width, 104, 320 - (33 + 1), 10, BLACK);
    }

    // Tempreture
    {
      char buf[20];
      M5.Lcd.setFreeFont(FSS9);
      sprintf(buf, "%2.2fC ", scd30.temperature);
      M5.Lcd.drawString(buf, 48 - 16, 157, GFXFF);
    }

    // Humidity
    {
      char buf[20];
      M5.Lcd.setFreeFont(FSS9);
      sprintf(buf, "%2.2f%% ", scd30.relative_humidity);
      M5.Lcd.drawString(buf, 177, 157, GFXFF);
    }

    // ambient
    if (WiFi.status() == WL_CONNECTED && scd30.CO2 != 0.0)
    {
      // 30秒に1回だけambientへ送信する
      ambient_send_span_counter++;
      if (ambient_send_span_counter >= AMBIENT_SEND_SPAN)
      {
        ambient_send_span_counter = 0;

        ambient.set(1, scd30.CO2);
        ambient.set(2, scd30.temperature);
        ambient.set(3, scd30.relative_humidity);
        ambient.set(4, (int)time(NULL));

        ambient.send();
      }
    }
  }

  // WiFi接続状態表示
  M5.Lcd.setFreeFont(NULL); // Select the font
  M5.Lcd.setTextColor(WHITE, BLUE);
  M5.Lcd.drawString((WiFi.status() == WL_CONNECTED) ? "WiFi " : "---- ", 260, 0, GFXFF); // Print the string name of the font
  M5.Lcd.setTextColor(WHITE, BLACK);

  //delay(2000);

  t = millis() - t;
  t = (t < PERIOD * 1000) ? (PERIOD * 1000 - t) : 1;

  int span = 200;
  for (int i = 0; i < t; i += span)
  {
    if (M5.BtnA.wasPressed() == 1)
    {
      M5.Lcd.setBrightness(LCD_HIGH_LEVEL);
    }
    if (M5.BtnB.wasPressed() == 1)
    {
      M5.Lcd.setBrightness(LCD_LOW_LEVEL);
    }
    short bBtnC = M5.BtnC.wasPressed();
    M5.update();

    // 再接続リクエスト
    if (bBtnC == 1)
    {
      // もし，WiFi切断中なら
      if (WiFi.status() != WL_CONNECTED) 
      {
        // 接続
        wifiMulti.run();
      }
    }

    if (WiFi.status() == WL_CONNECTED)
    {
        // NTP接続設定初期化していない（起動してからはじめてのWiFi接続時）
        if (bInitTime == false)
        {
          ambient.begin(channelId, writeKey, &client);
          MDNS.begin("co2");
          configTime(9 * 3600L, 0, "ntp.nict.jp", "time.google.com", "ntp.jst.mfeed.ad.jp"); //NTP
          bInitTime = true;
        }
    }


    // 時計表示（NTP設定初期化されている時のみ）
    if (bInitTime)
    {
      struct tm timeInfo;      //時刻を格納するオブジェクト
      char s[25];              //文字格納用
      getLocalTime(&timeInfo); //tmオブジェクトのtimeInfoに現在時刻を入れ込む
      sprintf(s, "%04d/%02d/%02d %02d:%02d:%02d ",
              timeInfo.tm_year + 1900, timeInfo.tm_mon + 1, timeInfo.tm_mday,
              timeInfo.tm_hour, timeInfo.tm_min, timeInfo.tm_sec); //人間が読める形式に変換

      M5.Lcd.setFreeFont(NULL);
      M5.Lcd.drawString(s, 86, 20, GFXFF);
    }

    delay(span);
  }
}
