Eudora_fix_mbx: Repair UTF-8 character codes and problematic HTML in Eudora mailboxes

This is a command-line (non-GUI) Windows program that can modify the data of a
Eudora mailbox in ways like this:

  - change UTF-8 characters that aren't rendered correctly into related ASCII
    or extended ASCII (Windows-1252) characters
  - change linefeed characters not adjacent to carriage returns into carriage returns
    so Eudora will move to a new line and not squash everything together
  - change Outlook-generated non-standard HTML into something Eudora deals correctly with

This should only be run when Eudora is not running, and there is a runtime check to ensure that.

All of the modifications are made to mailbox data without changing the size of the messages
or the mailbox.  In addition, the corresponding table-of-contents file (.toc) file has its
timestamp updated. As a result, Eudora won't rebuild the table-of-contents file when it restarts.

The program learns the changes you want to make from a file named "translations.txt".
Each line of that file is of this form:

     searchstring = replacementstring  ;comment

 The searchstring can optionally start with clauses modifying the search:
     <headers>     only match in the headers of the message
     <body>        only match in the body of the message
     <html xxx>    only match in HTML <xxx...> tags
     <ignorecase>  treat upper and lower case alphabetics as equivalent
 Following that is an arbitrary sequence of:
     "quotedstring"
     'quotedstring'
      hexadecimal string
Any single-character item can optionally preceeded by ! to mean "not this character".

The replacementstring is an arbitrary sequence of:
        "quotedstring"
        'quotedstring'
         hexadecimal string
         *
         <blanks>
The * means "use one character from the source", and the corresponding
  search character needs to be one that was preceeded by !.
<blanks> means replace the searched-for string with all blanks.

The rules are:
  - The searchstring may be 1 to 50 bytes long
  - The replacement may not be longer than the string searched for.
  - If the replacement is shorter than the search string, the remaining bytes are
    changed to zero in the mailbox, which are ignored when Eudora renders the text.
  - The components may be separated by one or more spaces.
  - A ; starts a comment

  Here are some example translations.txt lines:
      E28093 = "-"  ;En dash
      E28094 = "--" ;Em dash
      E2809C = '"'  ;left double quote
      E2808B = ""   ;zero-width space
      E282AC = 80   ;Euro Sign in extended ASCII
      <body> <ignorecase> <html meta> "charset=utf-8" = <blanks>
      !0D 0A !0D = * 0D *  ; replace isolated linefeeds with carriage returns
      "<o:p>" = "<p>"      ; replace Outlook namespace tag with paragraph tag

  See the accompanying "translations.txt" file for suggested translations that use
  the Windows-1252 extended ASCII character set, which Eudora displays correctly.
  If you want to only use standard ASCII characters instead, use the
  "translations_ASCII.txt" file by renaming it to translations.txt.

The program is invoked with a single argument, which is the base
filename of both the mailbox and the table-of-contents files:

      Eudora_fix_mbx  In

If the argument ends with ".mbx", it is removed. That allows the program to be run
by dragging and dropping the mailbox file onto the program's icon. The downside of
doing that is that the status report at the end will disaappear before you can read it.
But if there is an error, the program will pause until you acknowledge it.

The translations.txt file is expected to be in the current directory.

I don't guarantee this will work well for you, so keep a backup
of the mailbox file in case you don't like what it did!

Len Shustek, September 2021
