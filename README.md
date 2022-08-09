# JARVIS-TriggerFirmware

If install throws an error related to `Long Path Support` open a command prompt as administrator and run:

     reg add "HKLM\SYSTEM\CurrentControlSet\Control\FileSystem" /v LongPathsEnabled /t REG_DWORD /d 1
