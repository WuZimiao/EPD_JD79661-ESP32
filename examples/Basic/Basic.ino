// ESP32 驅動 2.13" 黑白黃紅四色電子紙 (JD79661) 範例
// 需要函式庫：Adafruit GFX Library（Arduino IDE 函式庫管理員搜尋 "Adafruit GFX"）
//
// 接線（ESP32 DevKit / WROOM，使用 VSPI 硬體 SPI）：
//   模組   ESP32
//   VCC -> 3V3   （不要接 5V）
//   GND -> GND
//   SCL -> GPIO18 (SCK)
//   SDA -> GPIO23 (MOSI)
//   CS  -> GPIO5
//   DC  -> GPIO17
//   RES -> GPIO16
//   BUSY-> GPIO4
// 其他板子（S3 / C3 ...）改下面的腳位即可，任意 GPIO 都能用。

#include <EPD_JD79661.h>
#include <Fonts/FreeSansBold12pt7b.h>

#define PIN_SCK   18
#define PIN_MOSI  23
#define PIN_CS     5
#define PIN_DC    17
#define PIN_RST   16
#define PIN_BUSY   4

EPD_JD79661 epd(PIN_CS, PIN_DC, PIN_RST, PIN_BUSY);

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("EPD 2.13 4-color demo");

  epd.begin(PIN_SCK, PIN_MOSI);
  epd.init();

  epd.setRotation(1);           // 橫向 250 x 122（同原廠預設方向）
  epd.fillScreen(EPD_WHITE);

  // 外框
  epd.drawRect(0, 0, epd.width(), epd.height(), EPD_BLACK);

  // 標題（GFX 外掛字型）
  epd.setFont(&FreeSansBold12pt7b);
  epd.setTextColor(EPD_RED);
  epd.setCursor(8, 26);
  epd.print("Hello ESP32");

  // 內建 5x7 字型
  epd.setFont(nullptr);
  epd.setTextSize(1);
  epd.setTextColor(EPD_BLACK);
  epd.setCursor(8, 40);
  epd.print("2.13\" JD79661  250x122  4-color");

  // 四色色塊
  const uint16_t colors[] = {EPD_BLACK, EPD_WHITE, EPD_YELLOW, EPD_RED};
  const char *names[]     = {"BLACK", "WHITE", "YELLOW", "RED"};
  for (int i = 0; i < 4; i++) {
    int x = 8 + i * 60;
    epd.fillRect(x, 56, 52, 36, colors[i]);
    epd.drawRect(x, 56, 52, 36, EPD_BLACK);
    epd.setTextColor(EPD_BLACK);
    epd.setCursor(x + 4, 98);
    epd.print(names[i]);
  }

  // 一些圖形
  epd.fillCircle(232, 22, 12, EPD_YELLOW);
  epd.drawCircle(232, 22, 12, EPD_BLACK);
  epd.drawLine(8, 110, 242, 110, EPD_RED);

  Serial.println("Refreshing (about 26 s)...");
  uint32_t t0 = millis();
  epd.display();
  Serial.printf("Done in %lu ms\n", millis() - t0);

  epd.sleep();                  // 電子紙斷電後畫面仍會保留

  // 一分鐘後刷成全白（適合長時間存放的狀態）
  Serial.println("Clearing to white in 60 s...");
  for (int s = 60; s > 0; s -= 10) {
    Serial.printf("  %d s\n", s);
    delay(10000);
  }

  // 要再更新畫面：epd.init() -> 畫圖 -> epd.display() -> epd.sleep()
  epd.init();                   // sleep 之後必須重新 init 喚醒
  epd.fillScreen(EPD_WHITE);
  Serial.println("Refreshing to white...");
  t0 = millis();
  epd.display();
  Serial.printf("Done in %lu ms\n", millis() - t0);
  epd.sleep();
  Serial.println("Test finished.");
}

void loop() {
  // 測試只跑一次；按 EN/RST 重新開始
  // 注意：四色屏只有全刷，沒有局部刷新；不要太頻繁地刷新
}
