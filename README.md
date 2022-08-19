# JARVIS-TriggerFirmware

## Quick Upload instructions

- Clone the repository with:

      git clone --recursive https://github.com/JARVIS-MoCap/JARVIS-TriggerFirmware.git

- Make sure you are in the `JARVIS-TriggerFirmware` Directory and run:
     * Linux:
     
           sh install_arduino_uno.sh
     
     * Windows:
     
           .\install_arduino_uno.bat

 - If the install throws an error related to `Long Path Support` first remove the `JARVIS-TriggerFirmware\PlatformIO\install` directory and then open a command prompt as administrator and run:

     reg add "HKLM\SYSTEM\CurrentControlSet\Control\FileSystem" /v LongPathsEnabled /t REG_DWORD /d 1


## Raspberry Pi Pico:
If you encounter a error regarding missing `libhidapi-hidraw0` install it with:

     sudo apt install -y libhidapi-hidraw0
