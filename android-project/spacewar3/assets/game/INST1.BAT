@echo off
:no2
echo .
echo    Insert  Disk  No.2  To  %src%
echo .
pause
if not exist %src%\disk.001  goto no2
echo ...Copying  to %dst%
copy %src%\disk.001 . > nul
:no3
echo .
echo    Insert  Disk  No.3  To  %src%
echo .
pause
if not exist %src%\disk.002 goto no3
echo ...Copying  to %dst%
copy %src%\disk.002 . > nul
:no4
echo .
echo    Insert  Disk  No.4  To  %src%
echo .
pause
if not exist %src%\disk.003 goto no4
echo ...Copying  to %dst%
copy %src%\disk.003 . > nul
echo .
echo .
copy /B disk.000+disk.001+disk.002+disk.003 gin3sp.ccc > nul
fae gin3sp.ccc *.*
del disk.*
del gin3sp.ccc
