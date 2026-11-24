#include "EPD_JD79661.h"

EPD_JD79661::EPD_JD79661(int8_t cs, int8_t dc, int8_t rst, int8_t busy,
                         SPIClass &spi, uint32_t spiHz)
    : Adafruit_GFX(NATIVE_W, NATIVE_H),
      _cs(cs), _dc(dc), _rst(rst), _busy(busy),
      _spi(spi), _spiSettings(spiHz, MSBFIRST, SPI_MODE0) {
  clearBuffer(EPD_WHITE);
}

void EPD_JD79661::begin(int8_t sck, int8_t mosi) {
  pinMode(_cs, OUTPUT);
  pinMode(_dc, OUTPUT);
  pinMode(_rst, OUTPUT);
  pinMode(_busy, INPUT_PULLUP);
  digitalWrite(_cs, HIGH);
  digitalWrite(_dc, HIGH);
  digitalWrite(_rst, HIGH);
  // 模組只有 SDA(MOSI)，沒有 MISO
  _spi.begin(sck, -1, mosi, -1);
}

// ---------- 低階 SPI ----------
// 原廠範例每個 byte 都拉一次 CS，這裡照做以確保相容。
void EPD_JD79661::writeCmd(uint8_t c) {
  _spi.beginTransaction(_spiSettings);
  digitalWrite(_dc, LOW);
  digitalWrite(_cs, LOW);
  _spi.transfer(c);
  digitalWrite(_cs, HIGH);
  digitalWrite(_dc, HIGH);
  _spi.endTransaction();
}

void EPD_JD79661::writeData(uint8_t d) {
  _spi.beginTransaction(_spiSettings);
  digitalWrite(_dc, HIGH);
  digitalWrite(_cs, LOW);
  _spi.transfer(d);
  digitalWrite(_cs, HIGH);
  _spi.endTransaction();
}

void EPD_JD79661::writeCmdData(uint8_t c, const uint8_t *d, size_t n) {
  writeCmd(c);
  for (size_t i = 0; i < n; i++) writeData(d[i]);
}

// BUSY 腳：低電位 = 忙碌，高電位 = 空閒
// 送出指令後控制器需要一點時間才會把 BUSY 拉低，先等一下再開始讀，
// 否則 ESP32 太快，會在 BUSY 還沒拉低前就判定「已完成」。
void EPD_JD79661::waitBusy() {
  delay(10);
  uint32_t t0 = millis();
  while (digitalRead(_busy) == LOW) {
    if (busyTimeoutMs && millis() - t0 > busyTimeoutMs) {
      Serial.println(F("[EPD] BUSY timeout - check wiring / power"));
      return;
    }
    if (busyHook) busyHook();
    delay(5);
  }
  if (debugBusy) Serial.printf("[EPD] busy %lu ms\n", millis() - t0);
}

void EPD_JD79661::hwReset() {
  delay(100);
  digitalWrite(_rst, LOW);
  delay(10);
  digitalWrite(_rst, HIGH);
  delay(10);
  waitBusy();
}

// ---------- 控制流程（序列取自原廠 EPD_Init）----------
void EPD_JD79661::init() {
  hwReset();

  static const uint8_t r4D[] = {0x78};
  static const uint8_t r00[] = {0x0F, 0x09};                         // Panel setting
  static const uint8_t r01[] = {0x07, 0x00, 0x22, 0x78, 0x0A, 0x22}; // Power setting
  static const uint8_t r03[] = {0x10, 0x54, 0x44};                   // Power off seq.
  static const uint8_t r06[] = {0x0F, 0x0A, 0x2F, 0x25, 0x22, 0x2E, 0x21}; // Booster
  static const uint8_t r30[] = {0x02};                               // Frame rate (OTP)
  static const uint8_t r41[] = {0x00};
  static const uint8_t r50[] = {0x37};                               // VCOM/data interval
  static const uint8_t r60[] = {0x02, 0x02};
  static const uint8_t r61[] = {SOURCE_BITS >> 8, SOURCE_BITS & 0xFF,
                                (uint8_t)(NATIVE_H >> 8), (uint8_t)(NATIVE_H & 0xFF)}; // Resolution
  static const uint8_t r65[] = {0x00, 0x00, 0x00, 0x00};
  static const uint8_t rE7[] = {0x1C};
  static const uint8_t rE3[] = {0x22};
  static const uint8_t rE0[] = {0x00};
  static const uint8_t rB4[] = {0xD0};
  static const uint8_t rB5[] = {0x03};
  static const uint8_t rE9[] = {0x01};

  writeCmdData(0x4D, r4D, sizeof r4D);
  writeCmdData(0x00, r00, sizeof r00);
  writeCmdData(0x01, r01, sizeof r01);
  writeCmdData(0x03, r03, sizeof r03);
  writeCmdData(0x06, r06, sizeof r06);
  writeCmdData(0x30, r30, sizeof r30);
  writeCmdData(0x41, r41, sizeof r41);
  writeCmdData(0x50, r50, sizeof r50);
  writeCmdData(0x60, r60, sizeof r60);
  writeCmdData(0x61, r61, sizeof r61);
  writeCmdData(0x65, r65, sizeof r65);
  writeCmdData(0xE7, rE7, sizeof rE7);
  writeCmdData(0xE3, rE3, sizeof rE3);
  writeCmdData(0xE0, rE0, sizeof rE0);
  writeCmdData(0xB4, rB4, sizeof rB4);
  writeCmdData(0xB5, rB5, sizeof rB5);
  writeCmdData(0xE9, rE9, sizeof rE9);
}

void EPD_JD79661::displayRaw(const uint8_t *image) {
  writeCmd(0x10);                       // Data start transmission
  for (uint16_t i = 0; i < BUF_SIZE; i++) writeData(image[i]);
  refresh();
}

void EPD_JD79661::refresh(bool dcVcom) {
  writeCmd(0x04);                       // Power ON
  waitBusy();
  const uint8_t r12[] = {(uint8_t)(dcVcom ? 0x01 : 0x00)};   // bit0: 0 = AC VCOM, 1 = DC VCOM
  writeCmdData(0x12, r12, 1);           // Display refresh
  waitBusy();
}

void EPD_JD79661::display() {
  static const uint8_t off[] = {0, 0, 0, 0, 0, 0, 0, 0, 0};  // PMODE=0：關閉區段模式
  writeCmdData(0x83, off, sizeof off);
  displayRaw(_buf);
}

// 目前旋轉方向的座標 -> 面板原生座標（同 drawPixel 的轉換）
void EPD_JD79661::toNative(int16_t x, int16_t y, int16_t &nx, int16_t &ny) {
  int16_t t;
  switch (getRotation()) {
    case 1: t = x; x = WIDTH - 1 - y; y = t; break;
    case 2: x = WIDTH - 1 - x; y = HEIGHT - 1 - y; break;
    case 3: t = x; x = y; y = HEIGHT - 1 - t; break;
  }
  nx = x; ny = y;
}

void EPD_JD79661::displayWindow(int16_t x, int16_t y, int16_t w, int16_t h, bool dcVcom) {
  if (w <= 0 || h <= 0) return;
  int16_t ax, ay, bx, by;
  toNative(x, y, ax, ay);
  toNative(x + w - 1, y + h - 1, bx, by);
  int16_t x0 = min(ax, bx), x1 = max(ax, bx);
  int16_t y0 = min(ay, by), y1 = max(ay, by);
  x0 = constrain(x0, 0, (int16_t)SOURCE_BITS - 1) & ~3;   // 水平起點對齊 4
  x1 = constrain(x1, 0, (int16_t)SOURCE_BITS - 1) | 3;    // 水平終點 = 4n+3
  y0 = constrain(y0, 0, NATIVE_H - 1);
  y1 = constrain(y1, 0, NATIVE_H - 1);
  if (y1 <= y0) y1 = min<int16_t>(y0 + 1, NATIVE_H - 1);

  const uint8_t ptl[] = {
    (uint8_t)(x0 >> 8), (uint8_t)(x0 & 0xFF),
    (uint8_t)(x1 >> 8), (uint8_t)(x1 & 0xFF),
    (uint8_t)(y0 >> 8), (uint8_t)(y0 & 0xFF),
    (uint8_t)(y1 >> 8), (uint8_t)(y1 & 0xFF),
    0x01,                                  // PMODE=1
  };
  writeCmdData(0x83, ptl, sizeof ptl);

  writeCmd(0x10);                          // 只送視窗內的資料
  for (int16_t yy = y0; yy <= y1; yy++)
    for (int16_t bx2 = x0 >> 2; bx2 <= (x1 >> 2); bx2++)
      writeData(_buf[bx2 + yy * ROW_BYTES]);
  refresh(dcVcom);
}

void EPD_JD79661::powerOff() {
  static const uint8_t r02[] = {0x00};
  writeCmdData(0x02, r02, 1);           // Power OFF
  waitBusy();
}

void EPD_JD79661::sleep() {
  static const uint8_t r02[] = {0x00};
  writeCmdData(0x02, r02, 1);           // Power OFF
  waitBusy();
  delay(20);                            // 手冊 Display Flow B：Power OFF 後等 20ms 再進深度睡眠
  static const uint8_t r07[] = {0xA5};
  writeCmdData(0x07, r07, 1);           // Deep sleep
}

// ---------- 緩衝區 ----------
void EPD_JD79661::clearBuffer(uint16_t color) {
  uint8_t c = color & 0x03;
  memset(_buf, (c << 6) | (c << 4) | (c << 2) | c, BUF_SIZE);
}

void EPD_JD79661::drawPixel(int16_t x, int16_t y, uint16_t color) {
  if (x < 0 || y < 0 || x >= width() || y >= height()) return;

  // 依 GFX 的 rotation 轉回面板原生座標 (122 x 250)
  int16_t t;
  switch (getRotation()) {
    case 1: t = x; x = WIDTH - 1 - y; y = t; break;
    case 2: x = WIDTH - 1 - x; y = HEIGHT - 1 - y; break;
    case 3: t = x; x = y; y = HEIGHT - 1 - t; break;
  }

  uint16_t addr  = (x >> 2) + y * ROW_BYTES;
  uint8_t  shift = 6 - ((x & 3) << 1);   // MSB 為最左邊的像素
  _buf[addr] = (_buf[addr] & ~(0x03 << shift)) | ((color & 0x03) << shift);
}

uint8_t EPD_JD79661::getPixelColor(int16_t x, int16_t y) {
  if (x < 0 || y < 0 || x >= width() || y >= height()) return EPD_WHITE;
  int16_t nx, ny;
  toNative(x, y, nx, ny);
  uint16_t addr  = (nx >> 2) + ny * ROW_BYTES;
  uint8_t  shift = 6 - ((nx & 3) << 1);
  return (_buf[addr] >> shift) & 0x03;
}
