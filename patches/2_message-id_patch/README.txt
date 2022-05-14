The last released version 7.1.0.9 of Eudora in 2006 included a change that 
suppressed sending the unique Message-Id header, apparently because there was a 
bug that generated two Message-Id headers when Send Again was used. Since the 
email server should add the header if it isn't already there, suppressing it was 
an easy temporary fix. Sadly, there were was never another version after 
7.1.0.9, so they never had the chance to fix it properly. 

Unfortunately some servers -- in particular some mailing list systems, and Apple 
Mail -- add the Message-Id header incorrectly, which causes confusion at the 
receiving end. For those experiencing that problem, this directory has a patched 
version of eudora.exe that reinstates the generation of a unique Message-Id 
header for every outgoing message. For the gory details of what I did, see 
message-id_buxfix.txt. 

The patched executable file also contains the previous fix for doubled letters 
on incoming messages. See the doubleletter_patch directory in this repository 
for information about that. 

To use this patched version, just save your existing eudora.exe file by renaming 
it something else, then click on the eudora.exe in this directory and use the 
"download" button to put it where your original was. 

As always, be cautious when using a patched version of any software. Backup, 
backup, backup! 

Len Shustek, 12 April 2022 
