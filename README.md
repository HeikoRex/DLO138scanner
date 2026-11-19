# DLO-138
An open source firmware for DSO-138 Oscilloscope. 
![Photo](https://github.com/ardyesp/DLO-138/blob/master/pics/pic4.png)

DSO-138 is an excellent piece of hardware based on ARM Cortex M3 core STM32F103 processor and sufficient for most beginner users. The stock firmware, while quite responsive, can use a few improvements. The main shortcoming which prompted the development of DLO-138 firmware is the inability to get waveform data into a computer for further analysis and the lack of a second channel. Engineers troubleshooting hardware issues need to mark reference points on waveform so having another analog or digital channel can greatly improve analysis. This firmware hopes to improve on these issues.

## Features
- Two analog channels
- Two digital logic channels (SWDIO and SWDIO pins (PA13 and PA14) on board)
- Serial port interface for captured waveform data
- Trigger source selectable from Analog Channel 1 or Digital Channel
- Option to use rotary encoder instead of + - and SEL switches
- 2K sample depth

This firmware can be used on stock DSO-138 hardware as well. Select one of the pre-compiled binaries to suit the board. Follow the firmware upgrade instructions for DSO-138. At any time, you can reflash DSO-138 with JYE Tech provided firmware.

# Cost
Extra features come at an additional cost. In the case of DLO-138, it is the loss of lowest timebase. Maximum sampling rate in DLO-138 is 20 µs/div instead of 10 µs/div. In the 20 µs/div range, firmware under-samples ADC channels, often reading same data twice. To use the second analog channel, analog front end has to be duplicated on a daughter board. On a stock hardware, this firmware can be used to provide two digital logic channels.

# Build
The build environment uses Arduino. For help with setting up IDE visit http://www.stm32duino.com

For graphics output, this project depends on the Adafruit GFX Library v1.1.4.
Install it via the Library Manager of your Arduino IDE.
Other/newer versions of this library are likely to not compile or will give you a black screen.

# Hardware
Following changes can be applied selectively, to get maximum functionality from board. The firmware can be run on unmodified hardware as well.
![Mod Schematic](https://github.com/ardyesp/DLO-138/blob/master/pics/HardwareMod.png)

# Usage:
	Push button in encoder (SEL if using switches) moves focus to next parameter
	Left/Right turn in encoder (+/- if using switches) changes the parameter which is in focus
	Short press OK to HOLD the waveform and output it on serial port
	Long press OK button:
	
		Focus				Action
		Trigger Level		Zero the trigger level to Analog channel 1
		Wave X scrollbar	Center waveform on screen (at trigger point)
		Wave Y cursor		Zero the cursor. If Analog CH1 coupling is GND, waveform reference base is set 
		Other				Toggle on screen Analog CH1 statistics display

	Press and hold OK button at power up to reset settings to default

## PC serial commands

The firmware accepts newline terminated commands at 115200 baud. It returns plain text
and CSV data over the USB serial connection.

| Command | Result |
| --- | --- |
| `HELP` | List supported commands |
| `STATUS` | Report timebase, sample count and operating state |
| `SAMPLES` | Report the waveform sample depth |
| `CAPTURE` | Start REC mode and reset the recorded pulse list |
| `DUMP` | Output only the data collected in REC mode as CSV |
| `AUTO` / `NORMAL` / `SINGLE` | Select trigger mode |
| `HOLD ON` / `HOLD OFF` | Freeze or resume display capture |
| `TIMEBASE 0..11` | Select the configured timebase |
| `TRIGGER RISING` / `TRIGGER FALLING` | Select trigger edge |

The existing `R`, `D`, `C` and `H` commands remain available for compatibility.

### CSV output

REC mode uses PA5 as its gate; PA10 is not used for this recording. While PA5 is HIGH,
the firmware samples CH1 and calculates its mean and slope (`delta CH1 / delta time`)
over the HIGH interval. The most recent `REC_BUF_SIZE` records remain in the ring
buffer. `DUMP` reports those records in mV:

On the device, short-press the OK/BTN4 button to cycle **RUN → HOLD → REC → RUN**.
The REC indicator is shown on screen; gate recording runs only in REC mode.

```csv
mean_mV,slope_mV_per_us
12.34,-5.6
...
```

For captures longer than the ring buffer, stream results directly to a CSV file on
Windows. This avoids terminal scrollback limits and works without Python packages:

```powershell
powershell -ExecutionPolicy Bypass -File .\tools\capture_com.ps1 -PortName COM3 -DurationSeconds 5 -OutputPath .\capture.csv
```

The script starts and stops REC mode automatically. Increase `DurationSeconds` for a
longer capture. It writes only the mean and slope columns.

The included Python helper can also stream a timed capture. Install `pyserial` first
with `python -m pip install pyserial`:

```text
python tools/dlo138_serial.py COM3 STATUS
python tools/dlo138_serial.py COM3 CAPTURE --csv capture.csv --duration 5
python tools/dlo138_serial.py COM3 DUMP --csv dump.csv
```
	
# Flash binaries directly via serial interface

When using Windows you can follow the guide from jyetech:
https://jyetech.com/wp-content/uploads/2018/07/dso138-firmware-upgrade.pdf

The guide uses the graphical programming tool provided by ST:
https://www.st.com/en/development-tools/flasher-stm32.html

---

When using Linux, you can use the open source command line tool stm32flash:

Install stm32flash:
```
sudo apt-get install stm32flash
```

Connect your TTL-UART-to-USB converter to the DSO138 and bridge jumpers J1 and J2 on the back of the PCB just like in the above manual.

Unlock the flash of the STM32:
```
sudo stm32flash /dev/ttyUSB0 -k -b 115200

sudo stm32flash /dev/ttyUSB0 -u -b 115200
```

Flash new firmware:
```
sudo stm32flash /dev/ttyUSB0 -w binaries/DLO-138_switches_1.0.bin -b 115200
```

Remove the solder bridges on J1 and J2 and enjoy the alternative firmware on your DSO138.
 		

# References
DSO-138 - http://www.jyetech.com/Products/LcdScope/e138.php

STM32Duino - http://www.stm32duino.com

STM32F103 - http://www.st.com/en/microcontrollers/stm32f103.html

Adafruint Graphics Library - https://github.com/adafruit/Adafruit-GFX-Library

Parallel 8 bit ILI9341 library - https://github.com/stevstrong/Adafruit_TFTLCD_8bit_STM32
