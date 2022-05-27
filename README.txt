Repairs for the venerable last release of Eudora, Version 7.1.0.9 from 2006.
It was the finest email client ever written, and we're trying to keep it alive!

There are two kinds of remediation offered here:

(1) The Eudora_fix_mbx program in this directory. 

   This "fixes" mailbox files to try to make the messages more compatible with what Eudora 
   can process. 

   It operates on the messages in the mailboxes you run it on. It doesn't install anything 
   inside Eudora that runs automatically when new messages are received. You have to run it 
   again for new messages after they have been received and put into a mailbox. 

   What I do is move messages that I want fixed from "In" to a "fixit" mailbox, then I use 
   the fix_mbx batch file to run the program on that mailbox and maintain backups. Then from 
   within Eudora I move the fixed messages to where I want them. Various people have devised 
   semi-automated ways to do that, using a combination of Eudora filters and additional 
   batch files. Some people routinely fix their "In" mailbox after new messages come in.

   For detailed information on what the program does, see details.txt. 
   For information on how to install and run it, see instructions.txt.

(2) Various patches to the Eudora program, Eudora.exe.

   These are modifications to the Eudora binary program itself which change how it 
   processes messages. The changes are permanent as long as you continue to use the
   patched version, and will apply to new incoming or outgoing messages.
   
   For details on the patched version, see the "patches" subdirectory and its
   README file.


Len Shustek, September/October 2021, March 2022, May 2022

