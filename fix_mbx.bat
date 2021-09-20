rem fix the UTF-8 characters and other problems in one Eudora mailbox
@echo off
set /p mbxname="mailbox name: "
rem Renaming backups as .mbx and .toc is a bad idea because
rem   Eudora has recorded the file name inside the TOC file.
rem If you restore the backups, use the original name.
@echo on
copy %mbxname%.mbx %mbxname%.mbx.bak
copy %mbxname%.toc %mbxname%.toc.bak
Eudora_fix_mbx %mbxname%
@echo off
IF %ERRORLEVEL% EQU 0 pause