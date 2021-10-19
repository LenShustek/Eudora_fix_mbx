@echo off
rem Fix the UTF-8 characters and other problems in one Eudora mailbox, and maintain
rem a series of backup files. If the mailbox name isn't an argument, we ask for it.
rem (this batch file version is from 10/19/2021 at12:08pm)
set numbackups=5
set logfile="Eudora_fix_mbx.log"
set maxlogsize=1000000

set mbxname=%1
if "%~1"=="" set /p mbxname="mailbox name: "
rem change forward slashes in the name to back slashes. Forward slashes will work in
rem   the Eudora_fix_mbx program, but not in batch file commands
set mbxname=%mbxname:/=\%
rem enclose the name in quotes, if it isn't already, in case there are embedded blanks
if "%mbxname:~0,1%%mbxname:~0,1%"=="""" (rem first character is a quote
) else set mbxname="%mbxname%"
rem remove .mbx extension if it is present, so that drag-and-drop works
set last4=%mbxname:~-5,4%
if "%last4:~0,1%%last4:~0,1%"=="""" goto tooshort
if "%last4%"==".mbx" set mbxname=%mbxname:~0,-5%"
if "%last4%"==".MBX" set mbxname=%mbxname:~0,-5%"
:tooshort
rem extract just the filename part (with surrounding quotes) for the later rename commands
call :setfilename %mbxname%

rem temporarily create the newest backups with the number 0 in the name
if exist %mbxname%.mbx copy /b %mbxname%.mbx %mbxname%.mbx.0.bak
if exist %mbxname%.toc copy /b %mbxname%.toc %mbxname%.toc.0.bak

rem fix the mailbox and TOC files, which also adds to the log
echo. >>%logfile%
Eudora_fix_mbx %mbxname%
set /a returnval=%ERRORLEVEL%
rem errorlevel is 0 for "made changes", 1 for "made no changes", 
rem   8 for "fatal error with no changes made", 12 for "fatal error with changes"
IF %returnval% EQU 0 goto keepbackup
IF %returnval% GTR 1 pause
if %returnval% GTR 8 goto keepbackup
rem no changes were made, or there was an error with no changes: delete the new backups
if exist %mbxname%.mbx.0.bak del %mbxname%.mbx.0.bak
if exist %mbxname%.toc.0.bak del %mbxname%.toc.0.bak
echo no changes were made, so the new backup of mailbox %mbxname% was removed
goto :truncatelog

:keepbackup
rem first remove the oldest backup -- the one with the highest number
if exist %mbxname%.mbx.%numbackups%.bak del %mbxname%.mbx.%numbackups%.bak
if exist %mbxname%.toc.%numbackups%.bak del %mbxname%.toc.%numbackups%.bak
if exist %mbxname%.mbx.0.bak echo a backup of mailbox %mbxname% was kept
rem now rename the remaining backups, so the newest is 1 and the oldest has the highest number
SETLOCAL EnableDelayedExpansion
for /L %%x in (%numbackups%,-1,1)do (
 set /a xminus1=%%x-1
 if exist %mbxname%.mbx.!xminus1!.bak ren %mbxname%.mbx.!xminus1!.bak %filename%.mbx.%%x.bak
 if exist %mbxname%.toc.!xminus1!.bak ren %mbxname%.toc.!xminus1!.bak %filename%.toc.%%x.bak
 )
rem make a log entry that lists all the backups
echo mailbox %mbxname% backups as of %date% at %time% >>%logfile%
  rem (the "more" below removes the first 5 useless lines of the directory list)
dir /OD %mbxname%.* | more +5 >>%logfile%

rem The following code truncates the log file file if it has gotten too big,
rem by repeatedly removing the first 100 lines until it isn't too big.
:truncatelog
FOR /F "usebackq" %%A IN ('%logfile%') DO set logsize=%%~zA
echo log file %logfile% uses %logsize% bytes
if %logsize% leq %maxlogsize% goto done
more +100 %logfile% > %logfile%.tmp
del %logfile%
ren %logfile%.tmp %logfile%
goto truncatelog

rem helper subroutine to allow the ~n "file name" modifier to be applied to a variable
:setfilename
set filename="%~n1"
exit /b

:done
