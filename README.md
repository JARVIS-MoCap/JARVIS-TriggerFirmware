# JARVIS-TriggerFirmware

## Quick Upload instructions

### Linux

- Clone the repository with:

      git clone --recursive https://github.com/JARVIS-MoCap/JARVIS-TriggerFirmware.git

- Make sure you are in the `JARVIS-TriggerFirmware` Directory and run:

           sh install_arduino_uno.sh

### Windows

- Make sure you have a recent version of Python installed. You can either get it directly from the Microsoft Store or download it from [here](https://www.python.org/downloads/).

- Clone the repository with:

      git clone --recursive https://github.com/JARVIS-MoCap/JARVIS-TriggerFirmware.git

- Make sure you are in the `JARVIS-TriggerFirmware` Directory and run:          
      
      .\install_arduino_uno.bat

 - If the install throws an error related to `Long Path Support` first remove the `JARVIS-TriggerFirmware\PlatformIO\install` directory and then open a command prompt as administrator and run:

       reg add "HKLM\SYSTEM\CurrentControlSet\Control\FileSystem" /v LongPathsEnabled /t REG_DWORD /d 1


## Raspberry Pi Pico:
If you encounter a error regarding missing `libhidapi-hidraw0` install it with:

     sudo apt install -y libhidapi-hidraw0


# Contact
JARVIS was developed at the **Neurobiology Lab of the German Primate Center ([DPZ](https://www.dpz.eu/de/startseite.html))**.
If you have any questions or other inquiries related to JARVIS please contact:

Timo Hüser - [@hueser_timo](https://mobile.twitter.com/hueser_timo) - timo.hueser@gmail.com
