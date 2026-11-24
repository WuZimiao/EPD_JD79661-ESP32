# EPD_JD79661

ESP32 / Arduino driver for **2.13" black / white / yellow / red e-paper panels with the JD79661 controller**
(122 × 250, 2 bits per pixel), e.g. the ZJY `ZJYE213S08Y0G01` module.
Drawing is done through [Adafruit GFX](https://github.com/adafruit/Adafruit-GFX-Library), so lines,
shapes, text and fonts all work.

適用 **JD79661 控制晶片的 2.13 吋黑白黃紅四色電子紙**（122 × 250，每像素 2 bit）的 ESP32／Arduino 驅動，
繪圖交給 Adafruit GFX。

## Features
- Full refresh (Display Flow B: reset → init → data → power on → refresh → power off → deep sleep)
- **Partial window refresh** (`displayWindow`) using the controller's PTL register
- Optional DC VCOM for partial refresh (experimental)
- `getPixelColor()` to read back the frame buffer (e.g. to export a preview image)
- BUSY timeout and an optional hook to keep a web server responsive while the panel refreshes

## Wiring (ESP32 DevKit example)

| Panel | ESP32 |
|---|---|
| VCC | 3V3 (**not 5 V**, max 3.7 V) |
| GND | GND |
| SCL | GPIO18 |
| SDA | GPIO23 |
| CS  | GPIO5 |
| DC  | GPIO17 |
| RES | GPIO16 |
| BUSY| GPIO4 |

Any GPIOs can be used; pass them to the constructor.

## Usage

```cpp
#include <EPD_JD79661.h>

EPD_JD79661 epd(/*cs*/5, /*dc*/17, /*rst*/16, /*busy*/4);

void setup() {
  epd.begin(/*sck*/18, /*mosi*/23);
  epd.init();                 // call again after every sleep()
  epd.setRotation(1);         // landscape 250 x 122
  epd.fillScreen(EPD_WHITE);
  epd.setTextColor(EPD_RED);
  epd.setCursor(10, 20);
  epd.print("Hello");
  epd.display();              // full refresh, ~20 s at 25 °C
  epd.sleep();
}
```

Colors: `EPD_BLACK`, `EPD_WHITE`, `EPD_YELLOW`, `EPD_RED`.

### Partial refresh

```cpp
epd.init();
epd.fillRect(158, 0, 92, 38, EPD_WHITE);
// ... draw into that area ...
epd.displayWindow(158, 0, 92, 38);   // only this rectangle is sent and refreshed
epd.sleep();
```

Notes from the JD79661 user guide and testing:
- The waveform is stored in the controller's OTP; there is **no fast/partial waveform**, so a partial
  refresh takes about as long as a full one and the window flashes.
- Gates outside the window are also scanned, so other areas fade slightly over time; do a full refresh
  now and then to clean up ghosting.
- The window snaps to 4 pixels along the panel's source direction (the screen's vertical axis in landscape).
- Re-initialize (`init()`) before every refresh and put the panel to sleep afterwards; keeping the
  controller awake between partial refreshes made it stop updating on real hardware.

## Panel specs (from the module datasheet)
- Refresh time ≈ 26 s (25 °C); lifetime 1,000,000 refreshes or 5 years
- Operating 0 – 50 °C; store at 23 °C in a **white** image
- Supply 2.2 – 3.7 V (3.3 V typical)

## Credits
The initialization register values follow the panel vendor's (ZJY / 中景園) sample code and the
JD79661 customer user guide. This library is an independent implementation; no vendor code is included.

## License
MIT — see [LICENSE](LICENSE).
