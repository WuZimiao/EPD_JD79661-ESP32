// 2.13" 黑白黃紅四色電子紙 (ZJYE213S08Y0G01, 控制晶片 JD79661) ESP32 驅動
// 由原廠 STM32 範例 (中景園 EPD.c / EPD_GUI.c) 移植，繪圖功能交給 Adafruit_GFX。
//
// 面板：122 x 250，每像素 2 bit，緩衝區 32 bytes/列 x 250 列 = 8000 bytes
//   00 = 黑  01 = 白  10 = 黃  11 = 紅
#pragma once
#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_GFX.h>

enum : uint16_t {
  EPD_BLACK  = 0,
  EPD_WHITE  = 1,
  EPD_YELLOW = 2,
  EPD_RED    = 3,
};

class EPD_JD79661 : public Adafruit_GFX {
public:
  static const int16_t  NATIVE_W   = 122;   // 可見寬度 (source)
  static const int16_t  NATIVE_H   = 250;   // 可見高度 (gate)
  static const uint16_t SOURCE_BITS = 128;  // 控制器 source 寬度 (含補齊)
  static const uint16_t ROW_BYTES  = SOURCE_BITS / 4;     // 32
  static const uint16_t BUF_SIZE   = ROW_BYTES * NATIVE_H; // 8000

  EPD_JD79661(int8_t cs, int8_t dc, int8_t rst, int8_t busy,
              SPIClass &spi = SPI, uint32_t spiHz = 4000000);

  // 設定腳位與 SPI（只需呼叫一次）。sck/mosi 傳 -1 用板子預設腳位。
  void begin(int8_t sck = -1, int8_t mosi = -1);

  // 喚醒並初始化控制器。每次從 sleep() 回來都要再呼叫一次。
  void init();

  // 把緩衝區送進螢幕並刷新（四色全刷規格書：25°C 時約 26 秒）
  void display();

  // 區段更新：只把 (x, y, w, h) 這塊（目前旋轉方向的座標）送進螢幕並刷新。
  // 面板原生的水平方向會對齊到 4 像素，所以實際更新範圍可能比指定的大一點。
  // dcVcom = true：刷新時 VCOM 用直流（手冊 R12H 參數 bit0），可能減少視窗外受影響，需實機驗證
  void displayWindow(int16_t x, int16_t y, int16_t w, int16_t h, bool dcVcom = false);

  // 關電源 + 深度睡眠；之後要再刷新必須重新 init()（控制器 RAM 會清掉）
  void sleep();

  // 只關高壓電源，控制器保持喚醒、RAM 保留（連續區段更新用）
  void powerOff();

  // 緩衝區操作
  void clearBuffer(uint16_t color = EPD_WHITE);
  void drawPixel(int16_t x, int16_t y, uint16_t color) override;
  void fillScreen(uint16_t color) override { clearBuffer(color); }
  uint8_t *getBuffer() { return _buf; }
  // 讀回緩衝區裡某一點的顏色（目前旋轉方向的座標）
  uint8_t getPixelColor(int16_t x, int16_t y);

  // 直接顯示一張原廠取模工具產生的四色圖 (8000 bytes，格式同原廠 Pic.h)
  void displayRaw(const uint8_t *image);

  // BUSY 等待逾時 (ms)，0 = 不限
  uint32_t busyTimeoutMs = 30000;

  // true = 每次等待 BUSY 後印出等了多久（除錯用）
  bool debugBusy = true;

  // 等待 BUSY 期間反覆呼叫（例如處理網頁伺服器），可為 nullptr
  void (*busyHook)() = nullptr;

private:
  void hwReset();
  void waitBusy();
  void writeCmd(uint8_t c);
  void writeData(uint8_t d);
  void writeCmdData(uint8_t c, const uint8_t *d, size_t n);
  void toNative(int16_t x, int16_t y, int16_t &nx, int16_t &ny);
  void refresh(bool dcVcom = false);

  int8_t _cs, _dc, _rst, _busy;
  SPIClass &_spi;
  SPISettings _spiSettings;
  uint8_t _buf[BUF_SIZE];
};
