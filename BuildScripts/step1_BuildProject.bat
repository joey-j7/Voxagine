@ECHO OFF

Echo "Building Windows project"

rem call "C:\Program Files (x86)\Microsoft Visual Studio\2017\BuildTools\MSBuild\15.0\Bin\msbuild.exe" %WORKSPACE%\Voxagine\Voxagine.sln /t:Voxagine;game /p:Platform=x64;Configuration="ReleaseGame Editor";verbosity=diagnostic
rem call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\MSBuild\15.0\Bin\msbuild.exe" %WORKSPACE%\Voxagine\Voxagine.sln /t:Voxagine;game /p:Platform=x64;Configuration="ReleaseGame Editor";verbosity=diagnostic
cd "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\MSBuild\15.0\Bin\" 
call "msbuild.exe" "%WORKSPACE%\Voxagine.sln" /t:Voxagine;game /p:Platform=x64;Configuration="ReleaseGame Editor";verbosity=diagnostic

@if ERRORLEVEL 1 goto fail

rem If no error, success
goto success

:fail
echo "ReleaseGameEditor failed." 
exit /B 1

:success
echo "ReleaseGameEditor completed."
exit /B 0

