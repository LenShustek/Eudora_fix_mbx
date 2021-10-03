@echo off
rem Fix the UTF-8 characters and other problems in one Eudora mailbox, and maintain
rem a series of backup files. If the mailbox name isn't an argument, we ask for it.
set numbackups=5
set logfile="Eudora_fix_mbx.log"
set maxlogsize=1000000

set mbxname=%1
if "%1"=="" set /p mbxname="mailbox name: "
rem remove .mbx extension if it is present, so that drag-and-drop works
set last4=%mbxname:~-4%
if "%last4%"==".mbx" set mbxname=%mbxname:~0,-4%
if "%last4%"==".MBX" set mbxname=%mbxname:~0,-4%
rem remove the oldest backup
rem   Note that naming backups with .mbx and .toc extensions is not a good
rem   idea, because Eudora has recorded the file name inside the TOC file.
rem   If you restore a backup, it's probably best to use the original name.
if exist %mbxname%.mbx.%numbackups%.bak del %mbxname%.mbx.%numbackups%.bak
if exist %mbxname%.toc.%numbackups%.bak del %mbxname%.toc.%numbackups%.bak
rem rename all the other existing backups, with the oldest having the highest number
SETLOCAL EnableDelayedExpansion
for /L %%x in (%numbackups%,-1,2)do (
 set /a xminus1=%%x-1
 if exist %mbxname%.mbx.!xminus1!.bak ren %mbxname%.mbx.!xminus1!.bak %mbxname%.mbx.%%x.bak
 if exist %mbxname%.toc.!xminus1!.bak ren %mbxname%.toc.!xminus1!.bak %mbxname%.toc.%%x.bak
 )
rem create the newest backups with the number 1 in the name
if exist %mbxname%.mbx copy %mbxname%.mbx %mbxname%.mbx.1.bak
if exist %mbxname%.toc copy %mbxname%.toc %mbxname%.toc.1.bak
if exist %mbxname%.mbx echo a backup of mailbox "%mbxname%" was made
rem make a log entry about the backups
echo. >>%logfile%
echo *** mailbox "%mbxname%" backups as of %date% at %time% >>%logfile%
  rem (the "more" below removes the first 5 useless lines of the directory list)
dir /OD %mbxname%.* | more +5 >>%logfile%
rem Finally, fix the mailbox and TOC files, which also adds to the log
Eudora_fix_mbx %mbxname%
IF %ERRORLEVEL% NEQ 0 pause
rem The following code truncates the log file file if it has gotten too big,
rem by repeatedly removing the first 100 lines until it isn't too big.
:truncate
FOR /F "usebackq" %%A IN ('%logfile%') DO set logsize=%%~zA
echo log file %logfile% uses %logsize% bytes
if %logsize% leq %maxlogsize% goto done
more +100 %logfile% > %logfile%.tmp
del %logfile%
ren %logfile%.tmp %logfile%
goto truncate
:done
