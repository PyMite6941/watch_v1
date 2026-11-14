@echo off
rem Watch Studio setup. Double-click this file.
rem It opens the installer window (files\setup.ps1). Nothing needs administrator rights.
rem If Windows says "Windows protected your PC", choose "More info" then "Run anyway":
rem the files are not signed, which is normal for something home-made.
start "" /min powershell.exe -NoProfile -ExecutionPolicy Bypass -WindowStyle Hidden -File "%~dp0files\setup.ps1" %*
