# BLE IoT Bridge using ESP32
This project is a BLE/IoT bridge to facilitate connecting an old fashioned Bluetooth smartbulb to a WiFi smart hub (such as `Alexa`).
This is achieved using an [ESP32](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/hw-reference/esp32/get-started-pico-kit-1.html)
to emulate a WiFi smartbulb (which can be automatically discovered by Alex) and then relaying the commands to the Bluetooth smartbulb.

The ESP32 is used since it is a low power micrcontroller which has both WiFi and Bluetooth enabling easy bridging between a WiFi network
and a Bluetooth device (the smartbulb).

## Discovery
The first stage of emulating a smartbulb is to allow the ESP32 to be discoverable by the IoT hub (Alexa) and respond in a way which makes
itsef look like a smartbulb.

Discovery of a TP-Link Kasa SmartBulb is performed as shown below:

**Request:**

```json
{"system":{"get_sysinfo":null}}
```

**Response:**

```json
{
  "system":
  {
    "get_sysinfo":
    {
      "sw_ver":"1.0.13 Build 211217 Rel.170501",
      "hw_ver":"2.0",
      "model":"KL130B(UN)",
      "deviceId":"80121C1874CF2DEA94DF3127F8DDF7D71DD7112E",
      "oemId":"E45F76AD3AF13E60B58D6F68739CD7E4",
      "hwId":"1E97141B9F0E939BD8F9679F0B6167C8",
      "rssi":-71,
      "latitude_i":0,
      "longitude_i":0,
      "alias":"Front Light",
      "status":"new",
      "description":"Smart Wi-Fi LED Bulb with Color Changing",
      "mic_type":"IOT.SMARTBULB",
      "mic_mac":"C0C9E3AD7C1C",
      "dev_state":"normal",
      "is_factory":false,
      "disco_ver":"1.0",
      "ctrl_protocols": 
      {
        "name":"Linkie",
        "version":"1.0"
      },
      "active_mode":"none",
      "is_dimmable":1,
      "is_color":1,
      "is_variable_color_temp":1,
      "light_state":
      {
        "on_off":0,
        "dft_on_state":
        {
          "mode":"normal",
          "hue":0,
          "saturation":0,
          "color_temp":2700,
          "brightness":75
        }
      },
      "preferred_state":[
        {
          "index":0,
          "hue":0,
          "saturation":0,
          "color_temp":2700,
          "brightness":50
        },
        {
          "index":1,
          "hue":0,
          "saturation":100,
          "color_temp":0,
          "brightness":100
        },
        {
          "index":2,
          "hue":120,
          "saturation":100,
          "color_temp":0,
          "brightness":100
        },
        {
          "index":3,
          "hue":240,
          "saturation":100,
          "color_temp":0,
          "brightness":100
        }
      ],
      "err_code":0
    }
  }
}
```

## SmartBulb Emulator

In order to emulate a SmartBulb and allow it to be discovered by Alexa, the following things should be emulated:

* `mic_mac` (MAC address): C0:C9:E3:AD:7C:1C
  * OUI (Organisationally Unique Identifier): `C0:C9:E3` (Tp-link Technologies Co.,ltd.) [lookup tool](https://dnschecker.org/mac-lookup.php?query=c0c9e3ad7c1c)
* `alias`
