This patched version of Eudora.exe fixes the problem with incoming 
Outlook-generated email whose suggested filename for attachments is long 
and split into multiple lines. Without the patch, Eudora only uses the 
first part of the name. 

This patch changes the way Eudora processes incoming messages, so the
filenames will be correct in the mailbox and in the attachment directory.
For details, see truncated_filename_fixes.txt.

This only fixes the easy case, where the Content-Disposition MIME header
contains a single filename= parameter that is split over multiple lines.
If the message uses the more modern formulation where there are multiple 
filename*n= subparts, it doesn't help. For that case, you can fix the
mailbox after the fact by using Eudora_fix_mbx and enabling the rule in
the translations.txt file that invokes <fixattachment>.

This patched executable file also contains the previous two patches that
fix doubled letters after UTF-8 substitutions and missing message-IDs.
See the other directories for details about those.

As always, be cautious when using a patched version of any software. Backup, 
backup, backup! 

Len Shustek, 13 May 2022; updated 22 Jun 2022
