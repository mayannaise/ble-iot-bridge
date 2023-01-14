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

## Reverse Engineering BLE Smartbulb
In order to determine the protocol used to control the smart bulb, the bluetooth signal needs to be intercepted
to capture the commands.

This can be achieved using a MITM attack. A PC is used to created a Bluetooth bridge using 2 Bluetooth adapters.
The smartphone (with the smartbulb control app) connects to the bridge which then relays the signal to the smartbulb
thus allowing the signal to be intercepted.

This can be achieved using a tool called [BtleJuice](https://github.com/DigitalSecurity/btlejuice)

### Installation

1. Intall correct version of `Node`

```bash
sudo apt-get purge nodejs && sudo apt-get autoremove && sudo apt-get autoclean
curl -o- https://raw.githubusercontent.com/creationix/nvm/v0.35.2/install.sh | bash
nvm install 6.17.1
nvm use 6.17.1
nvm alias default 6.17.1
source ~/.bashrc
nvm install --latest-npm
```

2. Install dependencies

```bash
npm install -g n
n 8.9.0
npm install bluetooth-hci-socket --unsafe-perm
```

3. Install BtlJuice

```bash
npm install -g btlejuice --unsafe-perm
echo fs.inotify.max_user_watches=524288 | sudo tee -a /etc/sysctl.conf && sudo sysctl -p
```

4. Setup MITM Attack
This requires 2 PCs to run the following components:
* `interception proxy`.
* `interception core`.


**Interception Proxy**

I ran this on a Raspberry PI. This is the end which will connect to the bluetooth smartbulb.

```bash
sudo setcap cap_net_raw+eip $(eval readlink -f `which node`)
sudo service bluetooth start
sudo hciconfig hci0 up
btlejuice-proxy -i hci0
```

**Interception Core**

I ran this on my Ubuntu20 laptop. Instead of connecting my phone to the bulb I will instead connect it to the proxied bluetooth connection on my laptop.

```bash
sudo service bluetooth stop
sudo hciconfig hci0 up
btlejuice -u 192.168.1.62 -i hci0 -w
```

**Running the Attack**
1. Visit the BtlJuice control page [localhost:8080](http://localhost:8080) from the "interception core" machine
1. Click the Bluetooth icon ("Select target device") to view all the BLE devices detected by the "interception core"
1. Double click the BLE target (e.g. smart bulb that you want to reverse engineer)
1. On the smartphone app, instead of connecting directly to the bulb, connect to the dummy smart bulb which has been emulated by the "interception proxy"


The MAC address of the BLE smartbulb I wan't to control is: `F0:C7:7F:93:E8:8D`
The RaspberryPI will connect to this.

The new proxied smartbulb will now be accessible from my laptop's bluetooth connection: `8C:88:2B:01:75:31`

```
wizwoz@ubuntu22:~$ hciconfig
hci0:	Type: Primary  Bus: USB
	BD Address: 8C:88:2B:01:75:31  ACL MTU: 1021:6  SCO MTU: 255:12
	UP RUNNING 
	RX bytes:2175 acl:0 sco:0 events:192 errors:0
	TX bytes:29073 acl:0 sco:0 commands:192 errors:0
```

**Results**

Service | Characteristic | Data | Function
---|---|---|---
FFB0 | FFB2 | D0 00 00 00 | Off
FFB0 | FFB2 | D0 FF 00 00 | Red
FFB0 | FFB2 | D0 00 FF 00 | Green
FFB0 | FFB2 | D0 00 00 FF | Blue
FFB0 | FFB2 | D0 FF FF FF | White

**Replaying Results**
Using `nRF Connect` Android app, you can connect to a Bluetooth device, navigate to a service and read from or write to characteristics.
This was used to verify that I had captured all the data required to control the bulb.
