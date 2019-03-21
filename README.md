VolCtl is a Windows command line program to get and set audio device volumes. It's intended to provide volume capability to a scripting language, such as python.

```Bash
Usage: VolCtl [options]
Options:
-l                list all active devices, each line has format:
                     'name' 'id' isInput
-I                list default input device for role
-O                list default output device for role
-i                select default input device (otherwise select default output device)
-n deviceName     specify device name
-d deviceId       specify device ID
-v vol            set volume, float between 0 and 1
-V                get volume
-m 0/1            set mute state: 1 == mute, 0 == no mute
-M                get mute state
-r role           device role: 0 = console, 1 = multimedia (default), 2 = communication
```
