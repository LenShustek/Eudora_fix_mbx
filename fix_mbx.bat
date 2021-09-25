rem fix the UTF-8 characters and other problems in one Eudora mailbox
@echo off
set /p mbxname="mailbox name: "
rem Naming backups with .mbx and .toc extensions might be a bad idea,
rem because Eudora has recorded the file name inside the TOC file.
rem If you restore the backups, it's probably best to use the original name.
@echo on
copy %mbxname%.mbx %mbxname%.mbx.bak
copy %mbxname%.toc %mbxname%.toc.bak
Eudora_fix_mbx %mbxname%
@echo off
IF %ERRORLEVEL% EQU 0 pause