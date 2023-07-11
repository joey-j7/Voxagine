@ECHO OFF

Echo Making folders if not exists

mkdir ".\..\Compressed\Development\%1\Content"
mkdir ".\..\Compressed\Development\%1\Engine"

Echo Copying files...

Robocopy  /s ".\..\Game\Content"  ".\..\Compressed\Development\%1\Content" /E
Robocopy  /s ".\..\Game\Engine"  ".\..\Compressed\Development\%1\Engine" /E
xcopy  /s ".\..\Voxagine\fmod64.dll"  ".\..\Compressed\Development\%1\" /E /F /Y
xcopy  /s ".\..\Voxagine\fmodL64.dll"  ".\..\Compressed\Development\%1\" /E /F /Y
xcopy  /s ".\..\Voxagine\OptickCore.dll"  ".\..\Compressed\Development\%1\" /E /F /Y
xcopy  /s ".\..\Voxagine\OptickCore_d.dll"  ".\..\Compressed\Development\%1\" /E /F /Y
xcopy  /s ".\..\Game\PlayerPrefs.vgprefs"  ".\..\Compressed\Development\%1\" /E /F /Y
xcopy  /s ".\..\Game\ProjectSettings.vgps"  ".\..\Compressed\Development\%1\" /E /F /Y
xcopy  /s ".\..\Game\Settings.vgs"  ".\..\Compressed\Development\%1\" /E /F /Y
xcopy  /s ".\..\Builds\%CONFIGURATION%\%BUILDPROJECT%\Game\%GAMEFILE%"  ".\..\Compressed\Development\%1" /E /F /Y

Echo Copy Complete

rem ReleaseGame Editor