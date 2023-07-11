@ECHO OFF

Echo Compressing files

7z a ".\..\Compressed\Development\%1" ".\..\Compressed\Development\%2"

@if ERRORLEVEL 1 goto fail

rem If no error, success
goto success

:fail
echo "Compressing failed." 
exit /B 1

:success
echo "Compressing completed."
exit /B 0

rem "D:\Program Files\WinRAR\Rar.exe" a -ep1 -idq -r -y ".\..\Compressed\Development\VoxagineGame_Development" ".\..\Compressed\Development\VoxagineGame_Development\"


rem timeout 10 >NUL