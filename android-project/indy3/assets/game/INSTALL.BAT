echo off
if "%1" == "" goto nodrive
mkdir %1\indy3
echo --
echo - 
echo - Copying files for Indiana Jones and the Last Crusade.
echo -
echo --
:disk1
if exist disk1.id goto cpdisk1
echo - Insert Disk 1 into the floppy drive, hit any key to continue.
echo --
pause > nul
goto disk1
:cpdisk1
echo - Copying disk 1 to %1
echo --
copy *.* %1\indy3
echo --
:disk2
if exist disk2.id goto cpdisk2
echo - Insert Disk 2 into the floppy drive, hit any key to continue.
echo --
pause > nul
goto disk2
:cpdisk2
echo - Copying disk 2 to %1					    
echo --
copy *.* %1\indy3
echo --
:disk3
if exist disk3.id goto cpdisk3
echo - Insert Disk 3 into the floppy drive, hit any key to continue.
echo --
pause > nul
goto disk3
:cpdisk3
echo - Copying disk 3 to %1
echo --
copy *.* %1\indy3
echo --
:disk4
if exist disk4.id goto cpdisk4
echo - Insert Disk 4 into the floppy drive, hit any key to continue.
echo --
pause > nul
goto disk4
:cpdisk4
echo - Copying disk 4 to %1
echo --
copy *.* %1\indy3
echo --
:disk5
if exist disk5.id goto cpdisk5
echo - Insert Disk 5 into the floppy drive, hit any key to continue.
echo --
pause > nul
goto disk5
:cpdisk5
echo - Copying disk 5 to %1
echo --
copy *.* %1\indy3
echo --
:disk6
if exist disk6.id goto cpdisk6
echo - Insert Disk 6 into the floppy drive, hit any key to continue.
echo --
pause > nul
goto disk6
:cpdisk6
echo - Copying disk 6 to %1
echo --
copy *.* %1\indy3
cls
echo --
echo -
echo - Done installing Indiana Jones on %1\INDY3
echo -
echo - To run, type    CD \INDY3
echo - then type       INDY3
echo -
echo - You can also start INDY3 up with several options including
echo -
echo - CGA      - Use CGA graphics.
echo - EGA      - Use EGA graphics.
echo - MCGA     - Use MCGA graphics.
echo - TANDY    - Use TANDY graphics.
echo - ADLIB    - Use Adlib board for sound and music.
echo - GAMEB    - Use the CMS (Game Blaster) board for sound and music.
echo - INTERNAL - Use the Internal PC speaker for sound and music.
echo - 2DISKS   - If you have two drives and are playing off
echo -            of floppys.
echo -
echo - Example:   INDY3 EGA ADLIB 2DISKS
echo -
echo --
%1
cd \indy3
goto end

:nodrive
echo --
echo -
echo - You must specify hard drive to install files onto.
echo -
echo - install c:
echo -
echo --
goto end

:end

