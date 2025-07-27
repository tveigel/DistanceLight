I will make this guide cleaner and more comprehensive in the next weeks, I just wanted to share the project.

# Background
My girlfriend lives in another city and I often forget to check my phone, so I built this light which she can press at anytime. This will light up on my desk, reminding me of taking a break and checking my phone :-)

This project can be modified in any way. The way I built this was the mosst efficient for me with stuff that I had laying around. You don't have to use a breadboard, you can include some other functionalities or make the electronics a bit safer. I'm simply providing a minimal idea here.

# ESP32‑S3 Nano Ping – Beginner Edition

https://www.youtube.com/shorts/T2wOaKLyKXQ

A friendly step‑by‑step project that lets two **Seeed XIAO ESP32‑S3 Nano** boards “ping” each other over Wi‑Fi/MQTT and answer with colourful LED effects. Press the button on unit A → unit B glows, and vice‑versa.

---

## 1 · What You’ll Need

**NOTE**: The links I attached are just examples with quick delivery. You can also get all these parts at AliExpress for probably 1/3 of the price, but it will take longer to arrive.
> **Tip for absolute beginners:** If you don’t have any of these parts, get a *starter kit* that includes a breadboard, jumper wires, resistors and capacitors. You’ll need those for many other projects as well.

| Qty | Part                                   | Notes                             |
| :-: | -------------------------------------- | --------------------------------- |
|  1  | **Seeed XIAO ESP32‑S3 Nano**           | [I used this one](https://www.seeedstudio.com/XIAO-ESP32S3-p-5627.html?srsltid=AfmBOorbtmUUk94OIPrtpH53kLbEPYzA-IHbMap9jMzgo7Rm_LBCjfvc)             |
|  1  | NeoPixel ring (12 × WS2812 B)          | [I used this one](https://www.amazon.de/dp/B09YTGFSPN?psc=1&ref=ppx_pop_dt_b_asin_title)                        |
|  1  | Momentary push‑button                  | [I used this one](https://www.amazon.de/dp/B0CJ6GXM2T?ref=ppx_pop_dt_b_asin_title&th=1) |
|  1  | 220 Ω resistor (¼ W)                   | If you don't have it laying around, get some kind of Arduino Starter Kit that has some jumper wires and capacitors as well  |
|  1  | 1000 µF electrolytic capacitor ≥ 6.3 V | [I used this one](https://www.amazon.de/dp/B09KV8M72F?psc=1&ref=ppx_pop_dt_b_asin_image)                       |
|  1  | 100 nF ceramic capacitor               | If you don't have it laying around, get some kind of Arduino Starter Kit that has some jumper wires and capacitors as well                     |
|  —  | Half‑size breadboard + jumper wires    | If you don't have it laying around, get some kind of Arduino Starter Kit that has some jumper wires and capacitors as well ([For lazyness I used this board](https://www.amazon.de/dp/B01M9CHKO4?psc=1&ref=ppx_pop_dt_b_asin_title))    |
|  1  | 5 V USB power supply (≥ 500 mA)        | Phone charger works               |

> **Tip for absolute beginners:** Jumper‑wire colours are only conventions; electrons don’t care – but *humans* do! Stick to red = 5 V, black = GND, yellow = data, green = button‑signal and everything stays readable.

---

## 2 · Cloud Broker
For this project we will need to set up a server in the cloud that will relay messages between the two XIAO boards, so they can communicate no matter where they are, as long as they are connected to WIFI.
You can use any MQTT broker you like, but I recommend using [CloudMQTT](https://www.cloudmqtt.com/) because they have generous free tiers, which you will never hit
with this project. Also I found it easy to set up and use.

## 3 · Wiring – The Easy Way


|  Step | Do this                                                                                                                                                                                   | Why                                                 |
| :---: | ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | --------------------------------------------------- |
| **A** | **Feed the rails.** Connect **5 V** on the XIAO to the breadboard’s red rail and **GND** to the blue rail.                                                                                | Power everywhere.                                   |
| **B** | **Power the ring.** Red wire from ring +5 V to red rail, black wire from ring GND to blue rail.                                                                                           | Ring gets juice.                                    |
| **C** | **Add capacitors.** Stick the **1000 µF** cap across red ↔ blue (long leg +), and the **100 nF** cap right next to it.                                                                    | Smoother voltage for the picky LEDs.                |
| **D** | **Data line.** Place the **220 Ω** resistor across a spare row (e.g. A10 → E10). Yellow wire from NeoPixel DIN into that row; then a jumper from the resistor’s other end to **XIAO D7**. | Protects first LED.                                 |
| **E** | **Button.** Bridge one button leg to the blue rail (GND) with a black wire. Put the other leg in a free row (e.g. A20) and run a green jumper from that row to **XIAO D4**.               | Internal pull‑up makes the pin go LOW when pressed. |

Here is an example picture on how to wire this:
![Wiring Example](ExampleWiring.png)

---

### ASCII Quick‑View (plug‑and‑play map)

```text
Breadboard (top view) – red rail at the very top, blue rail at the very bottom

+5 V rail  ============================================================

                     .------.   (yellow: data)
XIAO D7 ── 220 Ω ──▶ |  DIN |  NeoPixel ring
                     '------'

               +5 V (red) ──▶ Ring VCC
               GND (black) ─▶ Ring GND

XIAO D4 ──▶ Push‑button ──▶ GND (blue rail)    (green wire)

Capacitors on the rails:
  • 1000 µF electrolytic:  + → +5 V,  – → GND
  • 100 nF ceramic:        one leg → +5 V, other leg → GND

GND rail  ============================================================
```

Read it like this: **red** = power, **black** = ground, **yellow** carries LED data *through the resistor* to **D7**, and **green** is the button signal to **D4**.

### Pin‑by‑Pin Reference

| Function/Part          | XIAO pin    | Wire colour (suggested) | Breadboard destination                  |
| ---------------------- | ----------- | ----------------------- | --------------------------------------- |
| 5 V supply             | 5 V         | Red                     | Red rail & NeoPixel VCC                 |
| Ground                 | GND         | Black                   | Blue rail, NeoPixel GND, one button leg |
| NeoPixel DIN           | D7 (GPIO 7) | Yellow                  | Via 220 Ω resistor to NeoPixel DIN      |
| Push‑button signal     | D4 (GPIO 4) | Green                   | Other button leg                        |
| Bulk capacitor 1000 µF | —           | —                       | + leg → red rail, – leg → blue rail     |
| Decoupling cap 100 nF  | —           | —                       | Across red ↔ blue rails                 |




## 4 · Firmware Setup 

1. Connect your XIAO boards to your computer and open the Arduino IDE.
2. Open the *nanoA.ino* or *nanoB.ino* sketch in the Arduino IDE, depending on which board you set up as **A** or **B**.
3. Fill in your Wi‑Fi and MQTT (or any other Cloud Brokers) credentials in the sketch:

   ```cpp

   * `WIFI_SSID`, `WIFI_PASS`
   * `MQTT_BROKER`, `MQTT_USER`, `MQTT_PASS`
3. Decide which of your two boards is **A** and set `#define THIS_IS_A 1` (leave 0 for the other board).
4. Install the required libraries in Arduino IDE:

   * *PubSubClient*
   * *Adafruit NeoPixel*

5. Select **Seeed XIAO ESP32S3 → 2 MB Flash (no PSRAM)** and hit **Upload**.



---

## 5 · First Test

1. Power both boards from USB chargers.
2. Watch the Serial Monitor (115200 baud): you should see Wi‑Fi and MQTT connect messages.
3. Press the button – your ring shows a blue spinner. Within 3 s the other ring should glow.

> ⚠️ If the ring stays dark, double‑check *power* and the **yellow DIN wire** – 90 % of first‑time issues live there.

Happy hacking – and enjoy your first MQTT‑powered light show! 🔆
