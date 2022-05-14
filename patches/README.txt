There are subdirectories here with various patched versions of the eudora.exe executable 
file. Each patch was designed to fix a specific problem with Eudora, and each subdirectory 
contains an explanation of the problem and the fix. 

Each new patched version typically also includes the previous patches. Custom versions
could be created with different subsets of the patches, if there were a need.

To use any of these, rename your existing eudora.exe file (often in C:\Program Files 
(x86)\Qualcomm\Eudora) and use the one you find here instead. 

Patch 1: doubleletter_patch 
   Fix doubled letters that result from Eudora translating multi-byte UTF-8 characters.
   First patched byte: 12464D changes from 8B to E9.

Patch 2: message-id_patch
   Make Eudora send a unique Message-ID with each outgoing message.
   First patched byte: 101102 changes from 0F to 90.
   Also includes Patch 1.
   
Patch 3: POP_filename_patch
  Makes Eudora treat multi-line attachment filenames from POP servers as a single name.
  First patched byte: 554E0 changes from 0F to 90.
  Also includes Patch 1 and Patch 2.
  
I seem to be churning out patches. In order to identify which patches have been made to a 
particular copy of eudora.exe, I've shortened the "This program cannot be run in DOS 
mode." string near the beginning of the exe file, and replaced the end of it with a string 
that says which patches were applied. If you open the file with a text editor (or "type" 
the file from a command window), the second line will be something like this: 

  $Patches:123
  
which indicates that patches 1, 2, and 3 have been applied to that exe file.
