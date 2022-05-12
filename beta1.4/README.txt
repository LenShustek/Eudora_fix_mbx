This directory has beta test versions of two approaches to fixing 
Eudora's truncation of long filenames sent by Outlook. 

1. A change to Eudora_fix_mbx to repair it after the fact in the mailbox. 
To use this you need to uncomment the rule in translations.txt that 
invokes <fixattachment>. 

2. A patched version of the eudora.exe file that fixes the problem in the 
code so that the mailbox is correct. It also includes the previous two patches,
for the double letter problem, and for the message ID problem.

For more details about both approaches, see fixattachment_explanation.txt. 

L. Shustek, 5/11/2022 

