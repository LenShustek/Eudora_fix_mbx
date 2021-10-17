Eudora_fix_mbx: Repair UTF-8 character codes and problematic HTML in Eudora mailboxes

This is a command-line (non-GUI) Windows program that modifies the data of Eudora
mailboxes, and of the to/from and subject fields of table-of-contents (TOC) files,
in ways like this:

  - change UTF-8 characters that aren't rendered correctly into related ASCII
    or extended ASCII (Windows-1252) characters
  - change linefeed characters not adjacent to carriage returns into carriage returns
    so Eudora will move to a new line and not squash everything together
  - change Outlook-generated non-standard HTML into something Eudora deals correctly with

This is freeware, and it lives at  https://github.com/LenShustek/Eudora_fix_mbx.

You don't really have to understand all of the information below to use the program.
If you are ok with the defaults, see the instructions.txt file for a cookbook way
to install and run the program.

When used on system mailboxes (In, Out, Junk, and Trash) this should only be run when
Eudora is not running, and a runtime check ensures that. For any other mailbox, we lock
the file so Eudora isn't accessing it while we are, and vice versa. That said,
IT IS A GOOD IDEA TO *NOT* HAVE THE MAILBOX YOU ARE FIXING BE OPEN IN EUDORA.

All of the modifications are made without changing the size of the messages or of the files.
In addition, the table-of-contents file (.toc) file has its timestamp updated.
As a result, Eudora won't rebuild the table-of-contents file when it examines the mailbox.

The program learns the changes you want to make from a plain text file named
"translations.txt" that has the following kinds of lines:

Lines starting with "options" let you specify one or more keywords
that control the the operation of the program:
   skipheaders     Don't do replacements in the header lines of messages
   skipbody        Don't do replacements in the body of messages
   skiptoc         Don't do replacements in the table-of-contents file
   nologging       Don't do logging to Eudora_fix_mbx.log
   noeudora        Don't allow Eudora to be running even for non-system mailboxes
   eudoraokforsystemmailboxes  A hard-to-type option that lets Eudora run even
                                 for system mailboxes, if you like taking risks
   onlydo xx MB (or xx KB)  Only look at messages within the specified number of bytes at
                              the end of the mailbox, if the mailbox is larger than that.
                              This speeds up processing for very large mailboxes.

Others lines of the file specify replacements to make, in this form:
   searchstring = replacementstring  ;comment

The searchstring can optionally start with clauses restricting the search:
   <headers>        only match in the headers of messages
   <body>           only match in the body of messages
   <html xxx>       only match inside HTML <xxx...> tags
   <ifmatch n>      only match if "match flag n" is set; see <setmatch n>
   <ignorecase>     treat upper and lower case alphabetics as equivalent

Following that is an arbitrary sequence of:
   "quotedstring"           any printable characters except "
   'quotedstring'           any printable characters except '
    hexadecimal string      hexadecimal character codes

Any single-character item can optionally be preceeded by ! to mean "not this character".

The replacementstring can be an arbitrary sequence of:
   "quotedstring"          any printable characters except "
   'quotedstring'          any printable characters except '
    hexadecimal string     hexadecimal character codes
    *                      replace with the character matching !xx in searchstring
    <setmatch n>           set "match flag n"
    <clearmatch n>         clear "match flag n"
or the replacementstring can be one of these:
    <blanks>               replace the searchstring with all blanks
    <nothing>              replace the searchstring with all zeroes (same as "" or '')

The rules are:
  - The searchstring may be 1 to 50 bytes long
  - The replacementstring may not be longer than the string searched for.
  - If the replacement is shorter than the search string, then
    - In a mailbox, the remainder bytes of the search string are changed to zero,
      which are ignored when Eudora renders the text.
    - In the TOC, the rest of the field is shifted left and zeroes are inserted at the end.
      (We can't change remainder bytes to zeroes, because Eudora stops displaying at a zero.)
  - The match flags, numbered from 0 to 31, remember previous matches. They are all independent.
  - Multiple matchflags in a single searchstring must all be set for the match to trigger.
  - A * in the replacement string must match a search string character preceeded by !.
  - The components may be separated by one or more spaces.
  - A ; starts a comment

Here are some example translations.txt lines:

   E28093 = "-"  ;En dash
   E28094 = "--" ;Em dash
   E2809C = '"'  ;left double quote
   E2808B = ""   ;zero-width space
   E282AC = 80   ;Euro Sign in extended ASCII

   ;remove UTF-8 HTML specification so extended ASCII characters are shown
   <body> <ignorecase> <html meta> "charset=utf-8" = <blanks>

   ;fix text blocks from being squashed because they contain only naked linefeeds
   !0D 0A !0D = * 0D *  ; replace isolated linefeeds with carriage returns

   ;fix Outlook-generated messages so Eudora doesn't insert extra blank lines when replying
   <body> "<p class=MsoNormal>" = "" <setmatch 1>     ;remove Outlook 0-margin paragraphs
   <body> <ifmatch 1> "</p>" = "<br>" <clearmatch 1>  ;and replace with one linebreak

See the accompanying "translations.txt" file for suggested translations that use
the Windows-1252 extended ASCII character set, which Eudora displays correctly.
If you want to only use standard ASCII characters instead, use the
"translations_ASCII.txt" file by renaming it to translations.txt.

The program is invoked with a single argument, which is the base filename
(optionally prepended with a path) of both the mailbox and the table-of-contents files:

   Eudora_fix_mbx  In
   Eudora_fix_mbx  Archive.fol\In2008

If the mailbox name contains embedded blanks, enclose it in quotes.

If the mailbox name ends with ".mbx", it is removed. That allows the program to be run
by dragging and dropping the mailbox file onto the program's icon. The downside of
doing that is that the status report at the end will disaappear before you can read it.
The "fix_mbx" batch file recommended in instructions.txt helps with that, and it makes
backup files, so try drag-and-dropping onto the batch file's icon instead.

The translations.txt file is expected to be in the current directory.

A status report about what changes were made is appended to the file Eudora_fix_mbx.log,
unless you have specified "options nologging".

The program returns the following values, which can be tested as %ERRORLEVEL% in a batch file:
   0 no errors, and changes were made to the mailbox or table-of-contents file
   1 no errors, and no changes were made to either file
   8 a serious error occurred which has been described in the log and on the console

I don't guarantee this will work well for you, so be sure to keep backups of the
MBX and TOC files in case you don't like what it did!

Len Shustek, September/October 2021
