@ECHO OFF

Echo Making folders if not exists

mkdir ".\Compressed\Development\VoxagineGame_Development\Content"
mkdir ".\Compressed\Development\VoxagineGame_Development\Engine"

Echo Copying files...

xcopy /s ".\Game\Content"  ".\Compressed\Development\VoxagineGame_Development\Content" /E /Y /F
xcopy /s ".\Game\Engine"  ".\Compressed\Development\VoxagineGame_Development\Engine" /E /Y /F
xcopy /s %1  ".\Compressed\Development\VoxagineGame_Development\" /E /Y /F
Echo Copy Complete