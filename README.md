<h1 align="center">
    FreeDS
    <br>
    <a href="#">
        <img src="https://github.com/pablozg/freeds/wiki/images/logo.png" width="30%">
    </a>
</h1>

**⚠️ ATTENTION / ADVERTENCIA** The project has evolved through different licensing stages. Please check the version you are using:

* **v1.0.7 rev2:** Open Source (GPL-3.0). Source code available in this repo.
* **Betas (up to v1.0.21):** Distributed under CC BY-ND 4.0.
* **v2.0.0 and above:** Proprietary EULA. Requires **Per-Device Activation**.

You can get more info in the Telegram Channel: [http://t.me/freeds_es](http://t.me/freeds_es) or check the community manual: [Community Manual](https://docs.google.com/document/d/1-XLCqHEbpqEPi4geI4CbANmGxodExC__zDvND89xjIU)

**FreeDS** is a universal surplus manager, totally independent of the energy source (photovoltaic and/or wind), created by **Pablo Zerón**, tested by Aeizoon, and PCB Design by amcalo.

---

## 🔑 License & Activation (v2.0.0+)

Starting from **version 2.0.0**, FreeDS uses a **Proprietary EULA**. 

- **Ownership:** The software is the exclusive property of **Pablo Zerón**.
- **Cloud Services:** Powered and managed by **Ibepower Technologies S.L.**
- **Activation:** A unique **Activation Key** is required for each device (linked to Hardware ID).

### How to get a Key?
Send an email to **derivadorfreeds@gmail.com** with:
1. Your **Device ID** (visible on the screen/web interface).
2. **DIY/Contributors:** Proof of contribution or self-assembly (image of the device) to request a **free key**.
3. **Commercial Use:** Request a quote if you are selling hardware with FreeDS firmware.

---

## 📜 License History

| Version Range | License Type | Description |
| :--- | :--- | :--- |
| **<= v1.0.7 rev2** | [GPL-3.0](https://opensource.org/licenses/GPL-3.0) | Open Source. Original legacy code. |
| **v1.0.8 - v1.0.21** | [CC BY-ND 4.0](https://creativecommons.org/licenses/by-nd/4.0/) | Creative Commons. No derivatives allowed. |
| **>= v2.0.0** | **Proprietary EULA** | Proprietary. Requires activation key per hardware. |

*Full legal terms can be found in the [EULA.md](./EULA.md) file.*

---

## ☕ Donate
If you find this project useful, consider supporting its development:

[![](https://www.paypalobjects.com/en_US/i/btn/btn_donateCC_LG.gif)](https://www.paypal.com/donate/?hosted_button_id=P2KUJFDMRSGTY)

---

## Dependencies (for legacy v1.0.7 compilation)
- **TickerScheduler**: [Toshik/TickerScheduler](https://github.com/Toshik/TickerScheduler)
- **Async MQTT client**: [marvinroger/async-mqtt-client](https://github.com/marvinroger/async-mqtt-client)
- **ArduinoJson**: [bblanchon/ArduinoJson](https://github.com/bblanchon/ArduinoJson)
- **AsyncTCP**: [me-no-dev/AsyncTCP](https://github.com/me-no-dev/AsyncTCP)
- **ESPAsyncWebServer**: [me-no-dev/ESPAsyncWebServer](https://github.com/me-no-dev/ESPAsyncWebServer)
- **ThingPulse OLED SSD1306**: [ThingPulse/esp8266-oled-ssd1306](https://github.com/ThingPulse/esp8266-oled-ssd1306)
- **esp32ModbusTCP**: [bertmelis/esp32ModbusTCP](https://github.com/bertmelis/esp32ModbusTCP)

## Compile (Legacy v1.0.7)
1. **Download:** Clone this repo or download the [PID.zip](https://github.com/pablozg/freeds/archive/refs/heads/PID.zip).
2. **Library Setup:** Extract `lib.zip` into the `/lib` folder.
3. **IDE:** Open with **VSCode** and **PlatformIO**.
4. **Build:** Use PlatformIO to compile and upload to your ESP32.

---
Copyright (c) 2026 Pablo Zerón. Cloud services provided by Ibepower Technologies S.L.
