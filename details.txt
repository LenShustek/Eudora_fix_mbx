
Eudora_fix_mbx: Repair UTF-8 character codes and problematic HTML in Eudora mailboxes

This is a command-line (non-GUI) Windows program that modifies the data of Eudora
mailbox (MBX) files, and the address and subject fields of table-of-contents (TOC) files,
in ways like this:

  - change UTF-8 characters that aren't rendered correctly by Eudora
    into related ASCII or extended ASCII (ANSI, Windows-1252) characters
  - change linefeed characters not adjacent to carriage returns into carriage returns
    so Eudora will move to a new line and not squash everything together
  - change Outlook-generated non-standard HTML into something Eudora deals correctly with
  - left-justify inline images so they aren't way off the screen
  - fix attachment filenames sent by Outlook that Eudora mistakenly truncated

This operates on the messages in the mailboxes you run it on. It doesn't install anything
inside Eudora that runs automatically when new messages are received. You have to run
it again for new messages after they have been received and put into a mailbox.

What I do is move messages that I want fixed from "In" to a "_fixit" mailbox, then I use the
fix_mbx batch file to run the program on that mailbox and maintain backups. Then from within
Eudora I move the fixed messages to where I want them. Various people have devised semi-
automated ways to do that, using a combination of Eudora filters and additional batch files.

*** This is freeware that lives at https://github.com/LenShustek/Eudora_fix_mbx ****

There is a lot of detailed information below, but you don't really have to understand it all
to use the program. If you are ok with the defaults, see the instructions.txt file for a
cookbook way to install and run the program. In particular, it recommends using the fix_mbx
batch file to maintain a series of backups of the mailbox files you are fixing.

------ the details ------

When used on system mailboxes (In, Out, Junk, and Trash) this should only be run when
Eudora is not running, and a runtime check enforces that. For any other mailbox, we
lock the file so Eudora isn't accessing it while we are, and vice versa. That said,
IT IS STRONGLY SUGGESTED TO *NOT* HAVE THE MAILBOX YOU ARE FIXING BE OPEN IN EUDORA,
because Eudora caches some data from open mailboxes and you will see confusing results.

All of the modifications are made without changing the size of the messages or of the files.
In addition, the table-of-contents file (.toc) file has its timestamp updated.
As a result, Eudora won't rebuild the table-of-contents file when it examines the mailbox.

The program learns the changes you want to make from a plain text file typically named
"translations.txt", and we supply a suggested file to start with. Here are the kind
lines that it can have:

Lines starting with "options" let you specify one or more keywords that control
the overall operation of the program:

   skipheaders           Don't do replacements in the header lines of messages
   skipbody              Don't do replacements in the body of messages
   skiptoc               Don't do replacements in the table-of-contents file
   nologging             Don't do logging to Eudora_fix_mbx.log (or the file specified by -l=)
   onlydo xx MB          Only look at messages within the specified number of bytes at
   onlydo xx KB            the end of the mailbox, if the mailbox is larger than that.
                           This speeds up processing for very large mailboxes.
   noeudora              Don't allow Eudora to be running even for processing
                           non-system mailboxes
   checksync             Check that TOC entries point to valid message starts in MBX

   eudoraokforsystemmailboxes  A hard-to-type option that lets Eudora run even when
                               processing system mailboxes, if you like taking risks.

The remaining lines of the file specify the substitutions to made, like this:

     searchstring = replacementstring  ;comment

The searchstring can optionally start with clauses that restrict the search:
   <headers>             only match in the headers of messages
   <body>                only match in the body of messages
   <html xxx>            only match inside HTML <xxx...> tags
   <ifmatch n>           only match if "match flag n" is set; see <setmatch n>
   <ignorecase>          treat upper and lower case alphabetics as equivalent

Following that is an arbitrary sequence of these:
   "quotedstring"        any printable characters except "
   'quotedstring'        any printable characters except '
    hexadecimal string   hexadecimal character codes, like C2A0 or E282AC
    \r \n \t  \\         escape sequence for return (0D), newline (0A), tab (09), or backslash (5C)

Any single-character item in the searchstring can optionally be preceded by
     ! to mean "not this character", ie "any character but this one"
     * to mean "zero or more occurrences of this character"

The replacementstring can be an arbitrary sequence of these:
   "quotedstring"        any printable characters except "
   'quotedstring'        any printable characters except '
    hexadecimal string   hexadecimal character codes, like 20 or 433A
    \r \n \t  \\         escape sequence for return (0D), newline (0A), tab (09), or backslash (5C)
    *                    a request to insert the one character that
                            matched a !xx or !'c' in the searchstring
    <setmatch n>         set "match flag n"
    <clearmatch n>       clear "match flag n"

or the replacementstring can instead be one of these:
    <blanks>             replace the searchstring with all blanks
    <nothing>            replace the searchstring with all zeroes (same as "" or '')
    <nochange>           make no change to the searched string (useful if you are only setting match flags)
    <fixattachment>      a special request to fix and rename attachments whose file names were truncated;
                           for details, see the file truncated_filename_fixes.txt, or the comments before
                           fix_attachment_filename() in the source code. This is probably obsolete given
                           the patched version of eudora.exe at patches\3_POP_filename_patch.

The substitution rules are:
  - The searchstring may be from 1 to 100 bytes long
  - The replacementstring may not be longer than the string being searched for.
  - Quoted strings may contain \r \n \t \\ escape sequences for those special characters.
  - If the replacement is shorter than the search string, then
    - In a mailbox, the remainder bytes of the search string are changed to zero,
      which are ignored when Eudora renders the text.
    - In the TOC, the rest of the field is shifted left and zeros are inserted at the right end.
      (We can't change remainder bytes to zeros, because Eudora stops displaying at a zero.)
  - The match flags, numbered from 0 to 31, remember previous matches. They are all independent.
  - Multiple matchflags in a single searchstring must all be set for the match to trigger.
  - A * in the replacement string must match a search string character preceded by !.
  - The components may be separated by one or more spaces, which are ignored.
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

See the accompanying "translations.txt" file for suggested translations that use the
Windows-1252 extended ASCII (ANSI) character set, which Eudora displays correctly.

If you want to only use standard ASCII characters instead, use the
"translations_ASCII.txt" file either by renaming it to translations.txt, or by
giving (see below) the  -t=translations_ASCII.txt  command line option.

The program is normally invoked with a single argument, which is the base filename
(optionally prepended with a path) of both the mailbox and the table-of-contents files:

   Eudora_fix_mbx  In
   Eudora_fix_mbx  Archive.fol\In2008

If the mailbox or path name contains embedded blanks, enclose it in quotes.

If the mailbox name ends with ".mbx", it is removed. That allows the program to be run
by dragging and dropping the mailbox file onto the program's icon. The downside of
doing that is that an error reported at the end will disappear before you can read it.
The "fix_mbx" batch file recommended in instructions.txt helps with that, and it
maintains backup files, so try drag-and-dropping onto that batch file's icon instead.

The translations.txt file is normally expected to be in the current directory. If you wish
to use a different name and/or location for the translations file, specify it as follows:
  Eudora_fix_mbx     -t=filename.txt     mailboxname
  Eudora_fix_mbx  -t=path\filename.txt   mailboxname

A status report about what changes were made (or what errors were found) is normally appended
to the file Eudora_fix_mbx.log, unless you have specified "options nologging". If you wish
to use a different name and/or location for the log file, specify it using the
  -l=filename.txt  or  -l=path\filename.txt  command line option.

The program returns the following values, which can be tested as %ERRORLEVEL% in a batch file:
   0 no errors, and changes were made to the mailbox and/or the table-of-contents file
   1 no errors, and no changes were made to either file
   8 a serious error occurred but no changes were made to either file
   12 a serious error occurred and changes might have been made to one of the files

I don't guarantee this will work well for you, so be sure to keep backups of the
MBX and TOC files in case you don't like what it did! I have compiled it so that
it should run on any any version of Windows starting with XP from 2001.

Len Shustek, September/October 2021, March 2022, May 2022
