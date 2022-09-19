# Motogymkhana Timer

An Arduino and ultrasonic sensor based tool to measure lap times and expose them via webpage over WiFi. 
It stores a timestamp when the motorcycle first crosses the starting gate, and once the motorcycle crosses the gate again will calculate & send the lap time.

## Development Environment Setup

- PlatformIO & your favorite text editor/IDE. I used VSCode.

PlatformIO should automatically fetch the various dependencies. One exception is the library code for the Grove Pi ranging sensor which is checked into the repo.

## Hardware Setup

Required things:

- ESP8266 module
- Grove Pi ultrasonic ranger (you must modify the [code](src\main.cpp) if using some other sensor!)
- Battery bank to power the device in the field
- Some kind of case

Assemble the above, flash the logic using PlatformIO, then set up your favorite motogymkhana course and place your sensor at the starting gate, the ultrasonic sensor facing across the gate so it may detect a passing motorcycle. Try not to run over the device!

## Controls Overview

Once running, connect to the ESP8266's WiFi network:
- SSID: `ESP8266 Timer`
- Password: `motogymkhana`

Then, navigate to `http://http://192.168.4.1/` where you can see the lap time and change settings.  

![Screenshot of the configuration & display webpage](Docs\Images\webpage.png)

### Lap Timer

The `Last Trigger <...> sec` title shows the last recorded lap time.

The `Start` button will start the detection process. Note that the lap timer will only start once a motorcycle has been detected going through the gate!. Hit the button again to stop and reset the detection circuit.

### Settings 

These will most likely need tweaking based on your environment. Lost on reboot.

#### Ranging Interval

Default 100 msec. How often the ultrasonic sensor sends out pulses.

#### Window Size

The logic attempts to calibrate itself when started. This is the number of ranging values it stores in a rolling window in order to smooth out potential outlier measurements. Should not need tweaking if the device is static and not knocked around by wind or other external factors.

#### Detection Size

Rolling window size of how many measurements it takes before it considers an event a "trigger" (something crossing the gate). Meant to allow discarding of any outliers. Tweak this if the sensor trips when nothing has crossed the gate. A too high detection size combined with a high ranging interval may cause fast travelling motorcycles to not trigger the sensor.

#### Percent Difference Trigger

The sensor considers a range reading from the sensor a potential "trigger" when the difference from the average measurement (from the rolling window) exceeds this percentage. Once this happens the detection window goes into effect and if that is passed, the initial even is considered a "trigger". Two of these triggers is a completed lap.

#### After Detection Delay

The amount of time (in milliseconds) the device will sleep after a completed lap.

#### Log Level

Log level to filter the log output in the Log text area. Set to `DEBUG` to see verbose output.

## Limitations

An ultrasonic sensor isn't ideal, but it's what I had. A better approach might be some kind of IR/Laser light barrier that will be more accurate than this approach but require some kind of reflective plate and more careful alignment.

Another limitation is that only one motorcycle may perform the course at one time, else the second motorcycle will trigger the lap time to stop as it goes through the gate.

## Future Work

As of 9/18/2022 this whole concept has not yet been validated on an actual motogymkhana course - it's still in development. Things will surely change. 

- Validate on an actual course
- API improvements (use JSON instead of form-data everywhere?)
- Try different sensor like IR light barrier?
- Design & print a case