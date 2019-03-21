VolCtl is a Windows command line program to get and set audio device volumes. It's intended to provide volume capability to a scripting language, such as python.

```
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
-s sec            sleep, to test caller timeouts
```

Some simple examples follow.

List devices:
```
c:\>VolCtl -l
'Microphone (Realtek High Definition Audio)' '{0.0.1.00000000}.{90b810d0-2380-4297-8563-409b13ef7763}' 1
'Speaker/HP (Realtek High Definition Audio)' '{0.0.0.00000000}.{2c8c39cf-532d-4da5-9d15-27408337c05f}' 0
```

Set default speaker volume to 50%:
```
c:\>VolCtl -v 0.5
```

Get default speaker volume:
```
c:\>VolCtl -V
0.500000
```

Get default microphone volume:
```
c:\>VolCtl -i -V
0.400000
```

Set microphone volume to 80%, specifying microphone by name:
```
c:\>VolCtl -n "Microphone (Realtek High Definition Audio)" -i -v 0.8
```

Get default microphone volume:
```
c:\>VolCtl -i -V
0.800000
```
