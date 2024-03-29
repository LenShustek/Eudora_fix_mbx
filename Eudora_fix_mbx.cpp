//file: Eudora_fix_mbx.cpp
/*-----------------------------------------------------------------------------------------------------

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
   skipfilenames         Don't do replacements in MIME specifications with filenames.
   skipdeleted           Don't do replacements in deleted messages
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
    hexadecimal string   hexadecimal character codes, like 20, C2A0, or E282AC
    \r \n \t             escape sequence for return (0D), newline (0A), or tab (09)
    <whitespace>         matches zero or more blank, tab, return, or newline characters

Any single-character item in the searchstring can optionally be preceded by
     ! to mean "not this character", ie "any character but this one"
     * to mean "zero or more occurrences of this character"

The replacementstring can be an arbitrary sequence of these:
   "quotedstring"        any printable characters except "
   'quotedstring'        any printable characters except '
    hexadecimal string   hexadecimal character codes, like 20 or 433A
    \r \n \t             escape sequence for return (0D), newline (0A), or tab (09)
    *                    a request to insert the one character that
                            matched a !xx or !'c' in the searchstring
    <blankpad>           insert zero or more blanks at the current point to make the replacement
                            take the full size of the matched string. This can appear only once.
    <setmatch n>         set "match flag n"
    <clearmatch n>       clear "match flag n"

or the replacementstring can instead be one of these:
    <blanks>             replace the searchstring with all blanks
    <zeros>              replace the searchstring with all zeroes (same as "" or '')
    <nochange>           make no change to the searched string (useful if you are only setting match flags)
    <fixattachment>      a special request to fix and rename attachments whose file names were truncated;
                           for details, see the file fixattachment_explanation.txt, or the comments before
                           do_fixattachment() in the source code. Requires "option skipdeleted".

The substitution rules are:
  - The searchstring may be from 1 to 100 bytes long
  - The replacementstring may not be longer than the string being searched for.
  - If the replacement is shorter than the search string, then
    - In a mailbox, the remainder bytes of the search string are changed to zero,
      which are ignored when Eudora renders the text.
    - In the TOC, the rest of the field is shifted left and zeros are inserted at the right end.
      (We can't change remainder bytes to zeros, because Eudora stops displaying at a zero.)
  - The match flags, numbered from 0 to 31, remember previous matches. They are all independent.
  - Multiple matchflags in a single searchstring must all be set for the match to trigger.
  - A * in the replacement string must match a search string character preceded by !.
  - MIME specifications can be matched with the search string \r\n"mime-title" even
    when  options skipfilenames  has been specified. That allows you, for example, to change
    directory pathnames and file names.
  - The components may be separated by one or more spaces, which are ignored.
  - A ; starts a comment.

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

------------------------------------------------------------------------------------------------------*/
/*----- Change log -------

12 Sep 2021, L. Shustek, V0.1  First version, written in the C subset of C++. I hate C++.
14 Sep 2021, L. Shustek, V0.2  Elaborate to allow NOT searches, string search specifiers,
                               and "same as source" replacements. The syntax of the
                               translation.txt file has changed.
16 Sep 2021, L. Shustek, V0.3  Show a map of places changed. Minor cosmetic fixes.
                               Fix serious bug in ".mbx" extension removal code.
17 Sep 2021, L. Shustek, V0.4  Fix bug when printing extended ascii characters in the report.
                               Increase the translations.txt max linesize to 200.
                               Increase the max translations from 250 to 500.
                               In the status report, only show translations actually used.
                               Don't display the translation.txt file lines as they are read.
                               Check for duplicate search strings.
19 Sep 2021, L. Shustek, V0.5  Add search modifiers: <headers> <body> <html xxx> <ignorecase>
                               Add <blanks> as a replacement option
23 Sep 2021, L. Shustek, V0.6  Add match flags: <ifmatch n>, <setmatch n>, <clearmatch n>
                               Do subsitutions in the subject and address fields of TOC file.
                               Add options: skipheaders, skipbody, skiptoc
                               Add progress dots for big mailboxes; show total time taken.
25 Sep 2021, L. Shustek, V0.7  Allow multiple search restriction clauses to be in any order.
                               Allow modification to non-system mailboxes with Eudora running.
                               Restart progress dots when reading TOC file.
30 Sep 2021, L. Shustek, V0.8  Add logging, and an option that disables it.
                               Compile to be compatible with Windows XP. (Not a source change;
                                 see the build notes below.)
                               Delay closing MBX until after TOC processing, so it stays locked.
                               Clear all matchflags at the start of each message, because we've
                                 seen some cases where the search whose replacement has the
                                 <clearmatch n> was never found.
                               Return errorlevel 4 after giving help when there was no argument,
                                 to distinguish it from errorlevel 8 indicating errors.
                               Don't pause before exiting with an error.
11 Oct 2021, L. Shustek, V0.9  Report the number of messages scanned, and the number we changed.
                               Report on the discrepancy between MBX and TOC number of messages.
                               Save the comments given with replacements, and use them in reporting.
                               Add <nothing> as a replacement option, equivalent to "" or ''.
                               Put a note in translation.txt about how Eudora handles emoticons.
16 Oct 2021, L. Shustek, V1.0  Return errorlevel 1 if there are no errors but no changes were made,
                                 so the calling batch file can decide to discard the backup file.
                               Add option "onlydo 5 MB" or "onlydo 150 KB" to only do the recent
                                 part of big mailboxes.
19 Oct 2021, L. Shustek, V1.1  Give an error if there is more than one command argument supplied.
                               Report the date of the first mailbox message changed.
                               Show <blank> for search string blanks.
10 Feb 2022, L. Shustek, V1.2  Add -t= on the command line to specify the name of the translation file.
                               Add "checksync" option to see if TOC entries point to messages in the MBX.
                               Increase the maximum search/replacement size from 50 to 100 bytes.
                               Fix bug: a search could match beyond the end of the valid data at the
                                 end of the file. (Thanks to Art Maravelis for finding an example.)
                               Complain about "duplicate search string" only if testflags and options also match.
                               Allow <setmatch> after <blanks> or <nothing> in the replacement string.
                               Give an error when translations are missing the search or replacement string.
19 Mar 2022, L. Shustek, V1.3  Add -l= on the command line to specify the name of the log file.
 4 May 2022, L. Shustek, V1.4  Add <nochange> as a replacement option, which is useful for only setting match flags.
                               Add * as a search character prefix to mean "zero or more occurrences" of the next char.
                               Add <fixattachment> as a replacement option to repair truncated attachment filenames.
                               Allow \r \n \t \\ escape sequences as search or replacement items, or in the strings.
                               Show <b> instead of <blank> in strings, to save screen space.
27 May 2022, L. Shustek, V1.5  Make <fixattachment> deal with the filename*0=... filename*1=... continuation method.
                               In <fixattachment>, don't do rename unless the Content-Disposition suggested filename
                                 was split onto multiple lines. (Avoids renaming files that Eudora added digits to.)
                               In <fixattachment>, allow whitespace between Attachment Converted: and the string
                               Don't allow escape sequences like \r in strings: makes it too hard to specify pathnames.
                               Add "option skipdeleted" to not do replacements in deleted messages.
                                 (Requires prereading the TOC and recording where valid message start.)
                               Add <whitespace> as search term: zero or more blank, tab, return, or newline characters.
                               Add <blankpad> to pad the replacement with blanks to the length searched at that point.
                               Use _strerror(NULL) to show the text of I/O errors instead of the number.
                               Check for "replacement larger than search" only when matches are made, because <whitespace>
                                 and *' ' can both allow the search part to be larger than what's in the translation rule.
                               Don't do wide-character file rename, because it might screw up special characters above 0x80.
                               Add "option skipfilenames" to not do replacements inside MIME headers with filenames.
                                 (Required reorganzing how the inner search loop works.
                               Make <fixattachment> collapse multiple spaces at the start of continuation lines into
                                 a single space.
                               Refuse to allow <fixattachment> unless skipdeleted is specified; otherwise too many files
                                in deleted message get renamed, and they may be in use by other messages.

Ideas for future versions:
- Allow the size of the mailbox to expand or contract! Keep a list of the TOC offsets as for "option skipdeleted",
  and create a new .tmp copy of the mailbox with addition or deletions, keeping the TOC offset list updated.
  Make replacements the exact size; dont pad them with zeros.
  At the end, rename .mbx to .tmp, .tmp to .mbx, and read/update/rewrite the TOC.
- Allow no search string if <html xxx> is speciaafied, and the replacement is <nothing> or <blanks>
     That can be used to entirely delete an HTML tag, like those for images, as follows:
          <html img> = <nothing>
     (Requires a new OPT_REPL_ERASETAG", which we implement in the search phase, not the replacement phase.)
     But: a simpler option is just to change "<img " to "<xxx "
- In addition to !xx, allow numeric comparisons for the single-byte wildcard matches:
         >27    <'z'   >=80<=FF   >='a'<='z"
     That would be handy to more precisely specify translations for blocks of Unicode graphics characters.
         E298 >80 = "(O)"
 -Add a limited multi-character wildcard search:
         "<p " * "margin:0px;" * ">" = <blanks> ;remove 0-margin paragraph start
     (Beware: the matched string could be longer than the max for search and replacement strings.
      How should the search be limited?)
     Allow *n in replacements for the nth wildcard search term?
 -An easier more specific solution to the above case:
        <html p> "margin:0px" = <removetag> <setmatch 2>
      (Easier? As of now the start of the tag might be before the start of the buffer!)
 -Add an XX replacement alternative to * that translates the source character into 2 hex digits.
         E298 >80 = "x" XX    ;produces, for example, "x9C"  (but we wouldn't have room to show the E298!)
- Should we open the TOC even if we're not making changes to it, so that it's locked?
     The downside is that we fail early if it doesn't exist, and then we don't fix the MBX.
     It probably doesn't matter, since Eudora is probably not going to make changes unless
     it can open both files.
 - Add a mode where we make a modified copy of the mailbox (and TOC?) file, instead of updating in place?
     But if we change the name, do we need to change it in the TOC?
 - Add a command-line switch for "quiet mode", where only errors are shown?

-------------------------------------------------------------------------------------------------------*/
/* Copyright(c) 2021,2022 Len Shustek
The MIT License(MIT)
Permission is hereby granted, free of charge, to any person obtaining a copy of this software
and associated documentation files(the "Software"), to deal in the Software without
restriction, including without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following conditions :

The above copyright notice and this permission notice shall be included in all copies or
substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT.IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE. */

#define VERSION "1.5"
#define DEBUG 0             // 0=none, 1=little, 2=lots
#define FAKE_RENAME false   // for testing <fixattachment>: suppress doing the file renaming?

#define RECORD_MSG_OFFSETS false    // create a debugging file of message offsets?
#define SHOW_ALL_TRANSLATIONS false // always show final report, with all translations
#define DO_LOCKING true             // do file locking (and read/write) with fileapi.h calls, not fopen/fread/fwrite/fclose

#define TRANSLATION_FILE "translations.txt"   // if not specified with -t=xxx on the command line
#define LOG_FILE "Eudora_fix_mbx.log"         // if not specified with -l=xxx on the command line

/*  Build notes.......

 This is written in the C subset of C++. I dislike object-oriented programming in general,
 and C++ in particular.

 I compile using Microsoft Visual Studio Community 2017, with the following project properties:
 Disable precompiled headers: in Project/Properties/Configuration Properties/C++/Command Line,
    in the "Additional Options" box at the bottom, add /Y-.
    Check on the top to see that you are making the change for "All Configurations".
 Don't check for deprecated usage: in Project/Properties/Configuration Properties/C++/Preprocessor,
    click on Preprocessor Definitions, use the downarrow on the right to select Edit,
    and add a line in the top box saying _CRT_SECURE_NO_WARNINGS.
    Again, check that you're doing it for All Configurations.

 In order to compile a version that will work on Windows XP, you need to install deprecated toolset
 V141_xp into VS 2017: "Desktop Development with C++", check "Windows XP support for C++" in right panel,
 which also will cause the v141 toolset to be selected in the Individual Components list on the left.
 Then choose V141_xp in Configuration Properties/General property/Platform Toolset and apply;
 it should switch to Windows SDK version 7.0. (!)  Also: in Properties/Confg/C++/Language choose
 Conformance mode "No". Then spin around, touch your head three times, and it might work.
 https://docs.microsoft.com/en-us/cpp/build/configuring-programs-for-windows-xp?view=msvc-160c
 https://stackoverflow.com/questions/58417992/how-to-compile-code-for-windows-xp-in-visual-studio-2017
 Apparently VS 2019 and later versions cannot compile for XP using that toolset, which means that
 they will probably discontinue VS 2017 soon to prevent us from compiling for XP at all.

 To switch back to the later non-XP version, choose toolset V141 and SDK version 10.0.xxx.

 I use AStyle formatting, which maximizes the protein you see on one screen and deemphasizes the
 display of syntactic sugar. The block structure is evident from the nesting. We don't store tabs.
 https://marketplace.visualstudio.com/items?itemName=Lukamicoder.AStyleExtension
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <Share.h>
#include <Windows.h>
#include <stdarg.h>
#include <time.h>
#if DO_LOCKING
//#include <fileapi.h>  // apparently included in Windows.h
#endif
#include "Eudora_TOC.h"

#define max(x, y) (((x) > (y)) ? (x) : (y))
#define min(x, y) (((x) < (y)) ? (x) : (y))
typedef unsigned char byte;

#define BLKSIZE 8192          // how much we read or write at a time, for efficiency
#define MAX_STR 100           // max size of search and replacement strings
#define MAX_TRANSLATIONS 500
#define MAXFILENAME 300
#define LINESIZE 200
#define COMMENTSIZE 60        // the longest search-replace comment we save
#define MAPWIDTH 72           // the width of the "changed areas" map we show at the end
#define MAX_MATCHFLAG 31      // match flags are numbered from 0 to this and fit into an unsigned long

/* Some assumptions:
  - the mailbox is at most 2GB, so 32-bit signed or unsigned integer offsets are enough
  - fpos_t is actually just a 64-bit byte offset value
  - int is 32 bits
  */
// The buffer is divided into two halves so that we can match and replace over the buffer
// boundary. When we start looking in the second half, we slide the second half to the
// first half, and read the next data from the file into the second half.
// This works the same way for TOC files as for mailbox files.
byte bufferin[2 * BLKSIZE];   // the two halves of the buffer with the original data
byte bufferout[2 * BLKSIZE];  // the two halves of the buffer with the changes we have made
// We need two buffers so that changes don't mess up later searches. Consider this case:
//    !0D 0A !0D = * 0D *
// If we have only one buffer, the second 0A in the sequence xx 0A 0A xx won't be matched
// after the first 0A gets turned into 0D. So we use another buffer to retain the original data.
int bufferlen = 0;                 // how much is in the buffer
int bufferpos = 0;                 // where we're looking in the buffer, 0..BLKSIZE-1
fpos_t filepos1st;               // the file position of the first half of the buffer's data
fpos_t filepos2nd;               // the file position of the second half of the buffer's data
fpos_t fileposnext;              // the next file position to read
bool dirty1st = false;             // do we need to write the first half of the buffer?
bool dirty2nd = false;             // do we need to write the second half of the buffer?
unsigned long mbxsize;           // total size of the mailbox file in bytes
unsigned long bytes_to_do = MAXINT32;   // how many bytes of it to do
unsigned long bytes_to_skip = 0; // how many bytes of it to skip, therefore
unsigned long matchflags = 0;    // bitmap of which matchflags are currently set
int total_changes = 0;           // various stats...
int mailbox_changes;
int num_messages = 0;
int MBX_messages_changed = 0;
int TOC_messages_changed = 0;
int renamed_file_attachments = 0;
int deleted_msgs_skipped = 0;
char msg_date[LINESIZE] = { 0 };
char translation_filename[MAXFILENAME] = TRANSLATION_FILE;
char log_filename[MAXFILENAME] = LOG_FILE;
char map[MAPWIDTH + 1];          // '_' for untouched sections, '*' for modified sections
bool neednewline = false;        //we've got something (like progress dots) being displayed
int last_megabyte;               // how many megabytes of the file we've read so far (for dots)
FILE *logfile = NULL;
FILE *debugfile = NULL;

byte skip_areas;              // flags of which areas to skip
#define AREA_HEADERS 0x01
#define AREA_BODY 0x02
#define AREA_FILENAMES 0x04
#define AREA_DELETED 0x08
#define AREA_TOC 0x10

bool noeudora = false;           // other option flags
bool okeudoraalways = false;     //
bool logging = true;             //
bool checksync = false;          //

struct string_t {     // a searchstring or a replacementstring
   byte str[MAX_STR];         // the characters
   byte flag[MAX_STR];        // character flags
   int len; };                // how long the string is
#define FL_NOT 0x01           // flag for "not this char" in search
#define FL_ANYNUMBER 0x02     // flag for "any number" (zero or more) of this char in search
#define FL_WHITESPACE 0x04    // flag for "whitespace" (zero or more blank/tab/return/newline) in search 
#define WHITESPACE " \t\r\n"  //       (define whitespace)
#define FL_ORIG 0x08          // flag for "use original byte" in replacement
#define FL_BLANKPADBEFORE 0x10// flag for "pad with blanks before this character"
#define FL_BLANKPADAFTER 0x20 // flag for "pad with blanks after this character"

struct translation_t { // the table of translations
   struct string_t srch;      // the search string
   struct string_t repl;      // the replacement string
   char htmltag[MAX_STR];     // optional HTML tag name to match, like "<meta"
   byte srch_options;         // option bits for search, OPTSRCH_xxx
   byte repl_options;         // option bits for replace, OPTREPL_xxx
   byte status;               // status bits, STAT_xxx
   unsigned long testflags;   // bitmap of which match flags should be tested on search
   unsigned long setflags;    // bitmap of which match flags should be set on replace
   unsigned long clrflags;    // bitmap of which match flags should be cleared on replace
   int usecount;              // how often this translation was used
   char comment[COMMENTSIZE]; // the saved comment from the translations.txt file
} translation[MAX_TRANSLATIONS] = { 0 };
int num_translations = 0;     // number of translations parsed and stored

#define OPTSRCH_HEADERS 0x01      // option for "only search in headers"
#define OPTSRCH_BODY 0x02         // option for "only search in body"
#define OPTSRCH_IGNORECASE 0x04   // option for "ignore case in search"
#define OPTSRCH_HTMLTAGMATCH 0x08 // option for "only search in HTML matching 'htmltag'"

#define OPTREPL_BLANKS 0x01       // option for "replace with all blanks"
#define OPTREPL_ZEROS 0x02        // option for "replace with all zeros" (replacement is just null)
#define OPTREPL_NOCHANGE 0x04     // option for "make no replacement"
#define OPTREPL_BLANKPAD 0x08     // there is a place to insert blank padding (only for detected multiples, an error)
#define OPTREPL_FIXATTACH 0x10    // option for "fix truncated atttachment filename"

#define STAT_TAGMATCH 0x01    // status: "we are inside a matching HTML tag"

//
//    display routines
//
int log_printf(char const* const format, ...) { // printf, with logging
   va_list args;
   va_start(args, format);
   int nchars = vprintf(format, args);
   if (logging && logfile) vfprintf(logfile, format, args);
   va_end(args);
   return nchars; }

#define MAX_ERRLINE 200
char errline[MAX_ERRLINE];

void stopdots(void) {
   if (neednewline) {
      printf("\n");
      neednewline = false; } }

void assert(bool test, const char *msg, ...) { // check for a fatal error
   if (!test) {
      va_list args;
      stopdots();
      log_printf("ERROR: ");
      va_start(args, msg);
      vprintf(msg, args);
      if (logging && logfile) vfprintf(logfile, msg, args);
      va_end(args);
      log_printf("\n");
      //system("pause");  let the caller check ERRORLEVEL instead
      exit(total_changes == 0 ? 8 : 12); } }

char const *add_s(int value) { // make plurals have good grammar
   return value == 1 ? "" : "s"; }

void showhex(const char *msg, const char *string) {
   printf(msg);
   for (const char *ptr = string; *ptr; ++ptr)
      printf("%c %02X  ", *ptr & 0xff, *ptr & 0xff);
   printf("\n"); }

int show_matchflags(const char *name, unsigned long flags) {
   int nchars = 0;
   for (int flagnum = 0; flags; flags >>= 1, ++flagnum)
      if (flags & 1)
         nchars += log_printf("<%s %d> ", name, flagnum);
   return nchars; }

void show_stats(void) { // it's all over: display ending statistics
   int toc_changes = total_changes - mailbox_changes;
   log_printf("from %d message%s examined, we made %d MBX change%s in %d message%s, and %d TOC change%s",
              num_messages - deleted_msgs_skipped, add_s(num_messages - deleted_msgs_skipped),
              mailbox_changes, add_s(mailbox_changes),
              MBX_messages_changed, add_s(MBX_messages_changed),
              toc_changes, add_s(toc_changes));
   if (TOC_messages_changed) log_printf(" in %d message%s", TOC_messages_changed, add_s(TOC_messages_changed));
   log_printf(":\n");
   for (int ndx = 0; ndx < num_translations; ++ndx) {
      struct translation_t *t = &translation[ndx];
      if (SHOW_ALL_TRANSLATIONS || t->usecount > 0) { // maybe only show translations actually used
         log_printf("  "); int nchars = 0;
         if (t->srch_options & OPTSRCH_HEADERS) nchars += log_printf("<headers> ");
         if (t->srch_options & OPTSRCH_BODY) nchars += log_printf("<body> ");
         if (t->srch_options & OPTSRCH_IGNORECASE) nchars += log_printf("<ignorecase> ");
         if (t->srch_options & OPTSRCH_HTMLTAGMATCH) nchars += log_printf("<html %s> ", t->htmltag + 1);
         nchars += show_matchflags("ifmatch", t->testflags);
         for (int byt = 0; byt < t->srch.len; ++byt) {
            byte ch = t->srch.str[byt];
            if (t->srch.flag[byt] & FL_NOT) nchars += log_printf("!");
            if (t->srch.flag[byt] & FL_ANYNUMBER) nchars += log_printf("*");
            if (t->srch.flag[byt] & FL_WHITESPACE) nchars += log_printf("<whitespace>");
            else nchars += log_printf(ch == ' ' ? "<b>" : ch > 31 && ch < 127 ? "%c" : "%02X ", ch); }
         while (nchars < 11) nchars += log_printf(" ");
         nchars = 0;
         if (t->repl_options & OPTREPL_FIXATTACH) nchars += log_printf(" invoked <fixattachment>");
         else {
            log_printf(" changed to ");
            if (t->repl.len == 0) nchars += log_printf("<zeros>");
            else if (t->repl_options & OPTREPL_BLANKS) nchars += log_printf("<blanks>");
            else if (t->repl_options & OPTREPL_NOCHANGE) nchars += log_printf("<nochange>");
            else for (int byt = 0; byt < t->repl.len; ++byt) {
                  if (t->repl.flag[byt] & FL_BLANKPADBEFORE) nchars += log_printf(" <blankpad> ");
                  byte ch = t->repl.str[byt];
                  if (t->repl.flag[byt] & FL_ORIG) nchars += log_printf(" * ");
                  else nchars += log_printf(ch == ' ' ? "<b>" : ch > 31 && ch < 127 ? "%c" : "%02X ", ch);
                  //if (ch == '*') nchars += log_printf(" "); //what was this for??
                  if (t->repl.flag[byt] & FL_BLANKPADAFTER) nchars += log_printf(" <blankpad> "); } }
         nchars += log_printf(" ");
         nchars += show_matchflags("setmatch", t->setflags);
         nchars += show_matchflags("clearmatch", t->clrflags);
         while (nchars < 10) nchars += log_printf(" ");
         nchars = log_printf(" %d time%s", t->usecount, add_s(t->usecount));
         while (nchars < 14) nchars += log_printf(" ");
         if (t->comment[0]) log_printf("\"%s\"", t->comment);
         log_printf("\n"); } }
   if (renamed_file_attachments)
      log_printf("We %s %d file attachment%s\n",
                 FAKE_RENAME ? "faked renaming" : "renamed",
                 renamed_file_attachments, add_s(renamed_file_attachments)); }

//
// I/O routines: either old-style fopen/fread/fwrite/fclose, which don't allow locking,
// or the new fileapi.h routines like CreateFile/Lockfile/ReadFile/WriteFile, which do.
//
enum fileid { MBX, TOC };
#if DO_LOCKING // new-style fileapi.h routines
HANDLE mbx_handle = NULL, toc_handle = NULL;
void fileopen(fileid whichfile, char *path) {
   HANDLE *pHandle = whichfile == MBX ? &mbx_handle : &toc_handle;
   if ((*pHandle = CreateFile(path, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL))
         == INVALID_HANDLE_VALUE) {
      int nchars = snprintf(errline, MAX_ERRLINE, "can't open %s\n  ", path);
      FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(), 0, errline + nchars, MAX_ERRLINE - nchars - 1, NULL);
      assert(false, errline); }
   assert(LockFile(*pHandle, 0, 0, MAXINT32, 0), "can't lock file %s", path); } // lock 2 GB
void fileclose(fileid whichfile) {
   HANDLE *pHandle = whichfile == MBX ? &mbx_handle : &toc_handle;
   const char *filetype = (whichfile == MBX ? "MBX" : "TOC");
   assert(UnlockFile(*pHandle, 0, 0, MAXINT32, 0), "can't unlock %s file", filetype);
   assert(CloseHandle(*pHandle), "can't close %s file", filetype); }
int fileread(fileid whichfile, void * ptr, int numbytes) {
   HANDLE *pHandle = whichfile == MBX ? &mbx_handle : &toc_handle;
   unsigned long numread;
   assert(ReadFile(*pHandle, ptr, numbytes, &numread, NULL), "can't read %d bytes", numbytes);
   return numread; }
void filewrite(fileid whichfile, void * ptr, int numbytes) {
   HANDLE *pHandle = whichfile == MBX ? &mbx_handle : &toc_handle;
   unsigned long numwritten;
   assert(WriteFile(*pHandle, ptr, numbytes, &numwritten, NULL)
          && numwritten == numbytes, "can't write %d bytes", numbytes); }
void fileseek(fileid whichfile, int64_t position) {
   HANDLE *pHandle = whichfile == MBX ? &mbx_handle : &toc_handle;
   assert(SetFilePointer(*pHandle, (unsigned long)position, 0, FILE_BEGIN) != INVALID_SET_FILE_POINTER,
          "can't seek to position %ld", position); }
int64_t fileposition(fileid whichfile) { // get the current position in the file
   HANDLE *pHandle = whichfile == MBX ? &mbx_handle : &toc_handle;
   fpos_t position;
   assert((position = SetFilePointer(*pHandle, 0, 0, FILE_CURRENT)) != INVALID_SET_FILE_POINTER,
          "can't find current position %ld", position);
   return position; }
int64_t filesize(fileid whichfile) { // get the size of the file
   HANDLE *pHandle = whichfile == MBX ? &mbx_handle : &toc_handle;
   fpos_t position = fileposition(whichfile); // save current position
   fpos_t endfile;
   assert((endfile = SetFilePointer(*pHandle, 0, 0, FILE_END)) != INVALID_SET_FILE_POINTER,
          "can't find end-of-file position");
   fileseek(whichfile, position);
   return endfile; }

#else // old-style C routines
FILE *mbx_fid, *toc_fid;
void fileopen(fileid whichfile, char *path) {
   assert((whichfile == MBX ? mbx_fid : toc_fid) = fopen(path, "rb+"),
          "can't open file %s", path); }
void fileclose(fileid whichfile) {
   fclose(whichfile == MBX ? mbx_fid : toc_fid); }
int fileread(fileid whichfile, void * ptr, int numbytes) {
   return (int)fread(ptr, 1, numbytes, whichfile == MBX ? mbx_fid : toc_fid); }
void filewrite(fileid whichfile, void * ptr, int numbytes) {
   assert(fwrite(ptr, 1, numbytes, whichfile == MBX ? mbx_fid : toc_fid) == numbytes,
          "can't write %d bytes, errno %d", numbytes, errno); }
void fileseek(fileid whichfile, int64_t position) {
   assert(fsetpos(whichfile == MBX ? mbx_fid : toc_fid, &position) == 0,
          "can't seek to position %ld, errno %d", position, errno); }
int64_t fileposition(fileid whichfile) { // get the current position in the file
   fpos_t position;
   assert(fgetpos(whichfile == MBX ? mbx_fid : toc_fid, &position) == 0,
          "can't get file position, errno %d", errno);
   return position; }
int64_t filesize(fileid whichfile) { // get the size of the file
   fpos_t position;
   FILE *fid = whichfile == MBX ? mbx_fid : toc_fid;
   assert(fgetpos(fid, &position) == 0,  // save current position
          "can't save current position for filesize, errno=%d", errno);
   assert(_fseeki64(fid, 0, SEEK_END) == 0, "can't seek to end of file, errno %d", errno);
   fpos_t endfile = ftell(fid);
   assert(_fseeki64(fid, position, SEEK_SET) == 0, "can't seek back to start of file, errno %d", errno);
   return endfile; }
#endif
//
//    buffer management routines
//
void init_buffer(fileid whichfile) { // initialize both halves of both buffers
   filepos1st = fileposition(whichfile);
   bufferlen = fileread(whichfile, bufferin, BLKSIZE);
   memcpy(bufferout, bufferin, BLKSIZE);
   filepos2nd = fileposition(whichfile);
   bufferlen += fileread(whichfile, bufferin + BLKSIZE, BLKSIZE);
   memcpy(bufferout + BLKSIZE, bufferin + BLKSIZE, BLKSIZE);
   fileposnext = fileposition(whichfile);
   bufferpos = 0;
   last_megabyte = 3 + (int)(fileposnext >> 20); // show more progress dots after this many megabytes
   matchflags = 0;
   if (DEBUG) printf("\nbuffer primed with %d bytes, filepos1st %lld, filepos2nd %lld\n", bufferlen, filepos1st, filepos2nd); }

void chk_buffer(fileid whichfile, bool forceend) { // keep the pointer in the first half of the buffer
   if (forceend || bufferpos >= BLKSIZE + 1) { //+1 is so we always have one previous byte to check
      if (dirty1st) {
         if (DEBUG) printf("writing first %d bytes of buffer to file position %lld\n", min(BLKSIZE, bufferlen), filepos1st);
         fileseek(whichfile, filepos1st); // write out first half if it was changed
         filewrite(whichfile, bufferout, min(BLKSIZE, bufferlen)); }
      memcpy(bufferin, bufferin + BLKSIZE, BLKSIZE);  // slide second half to first half
      memcpy(bufferout, bufferout + BLKSIZE, BLKSIZE);
      filepos1st = filepos2nd;
      filepos2nd = fileposnext;
      fileseek(whichfile, fileposnext);
      int new_megabyte = (int)(fileposnext >> 20);
      if (DEBUG < 2 && new_megabyte > last_megabyte) {
         printf("."); // print a progress dot after every megabyte, except when we're generating a lot of debugging
         neednewline = true;
         last_megabyte = new_megabyte; }
      int nbytes = fileread(whichfile, bufferin + BLKSIZE, BLKSIZE); // read (up to) another half
      memcpy(bufferout + BLKSIZE, bufferin + BLKSIZE, BLKSIZE);
      fileposnext = fileposition(whichfile);
      if (DEBUG>1) printf("buffer starts at file pos %lld, read %d bytes at file pos %lld, next file pos %lld\n", filepos1st, nbytes, filepos2nd, fileposnext);
      dirty1st = dirty2nd;
      dirty2nd = false;
      bufferlen = bufferlen - BLKSIZE + nbytes;
      bufferpos -= BLKSIZE; } }
//
// scanning routines for the translations.txt file and command-line options
//
char line[LINESIZE];

char lowercase(char ch) {
   if (ch >= 'A' && ch <= 'Z') ch += 'a' - 'A';
   return ch; }

char uppercase(char ch) {
   if (ch >= 'a' && ch <= 'z') ch -= 'a' - 'A';
   return ch; }

bool skip_blanks(char **pptr) { // returns true at end-of-line or start of comment
   while (**pptr == ' ' || **pptr == '\t')++*pptr;
   return **pptr == 0 || **pptr == '\n' || **pptr == ';'; }

bool scan_keyword(char **pptr, const char *keyword) {
   skip_blanks(pptr);
   char *ptr = *pptr;
   do if (lowercase(*ptr++) != *keyword++) return false;
   while (*keyword);
   *pptr = ptr;
   skip_blanks(pptr);
   return true; }

bool scan_number(char **pptr, int *pnum) {
   int num, nch;
   if (sscanf(*pptr, "%d%n", &num, &nch) != 1) return false;
   *pnum = num;
   *pptr += nch;
   skip_blanks(pptr);
   return true; }
//
// parse command-line options
//
void show_help(void) {
   printf("\nRepair UTF-8 character codes and problematic HTML in Eudora mailboxes.\n");
   printf("It reads translation.txt, which has lines like:\n");
   printf("   E28098 = \"'\"         ; left single quote\n");
   printf("   C2A1 =  '\"'          ; double quote\n");
   printf("   '<o:p>' = '<p>'      ; repair Outlook HTML\n");
   printf("   !0D 0A !0D = * 0D *  ; change isolated LF to CR \n");
   printf("   <body> '<p class=MsoNormal>' = <blanks> <setmatch 1>  ;remove Outlook 0-margin paragraphs\n");
   printf("invoke as: Eudora_fix_mbx filename\n");
   printf("The mailbox filename.mbx is changed in place, so keep a backup!\n");
   printf("It also updates the timestamp of filename.toc so Eudora doesn't rebuild it.\n");
   printf("For more information see https://github.com/LenShustek/Eudora_fix_mbx\n"); }

bool switch_keyword(const char *arg, const char* keyword) {
   do { // check for a keyword option and nothing after it
      if (toupper(*arg++) != *keyword++) return false; }
   while (*keyword);
   return *arg == '\0'; }

bool switch_string(const char *arg, const char *keyword, const char * *str) {
   do { // check for a "keyword=string" option
      if (toupper(*arg++) != *keyword++) return false; }
   while (*keyword);
   *str = arg; // return ptr to "string" part, which could be null
   return true; }

bool switch_integer(const char* arg, const char* keyword, int *pval, int min, int max) {
   do { // check for a "keyword=integer" option and nothing after it
      // the integer can be decimal, 0x8A (hex), 0b101 (binary), or 0123 (octal)
      if (toupper(*arg++) != *keyword++)
         return false; }
   while (*keyword);
   int num = 0, nch = 0;
   if (*arg == '0') { // leading 0: alternate number base specified
      ++arg;
      if (toupper(*arg) == 'X') { // hex
         if (sscanf(++arg, "%x%n", &num, &nch) != 1) return false; }
      else if (toupper(*arg) == 'B') { // binary
         char ch; // (why is there no %b conversion?)
         while (ch = *++arg) {
            if (ch == '0') num <<= 1;
            else if (ch == '1') num = (num << 1) + 1;
            else return false; } }
      else { // octal, or just a single zero
         if (*arg != 0 && sscanf(arg, "%o%n", &num, &nch) != 1) return false; } }
   else if (sscanf(arg, "%d%n", &num, &nch) != 1) return false; // decimal
   if (num < min || num > max || arg[nch] != '\0') return false;
   *pval = num;
   return true; }

bool switch_filename(const char* arg, const char* keyword, char* path) {
   //check for keyword=filename
   const char *str;
   if (switch_string(arg, keyword, &str)) {
      strncpy(path, str, MAXFILENAME); path[MAXFILENAME - 1] = '\0';
      return true; }
   return false; }

bool parse_cmdline_switch(char *option) {
   if (option[0] != '-' && option[0] != '/') return false;
   char *arg = option + 1;
   if (switch_filename(arg, "T=", translation_filename)) {}
   else if (switch_filename(arg, "L=", log_filename)) {}
   // could do: else if (switch_keyword(arg, "KEYWORD")) something;
   // could do: else if (switch_integer(arg, "KEYWORD=", &number, min, max)) something;
   else if (option[2] == '\0') // single-character switches
      switch (toupper(option[1])) {
      case 'H':
      case '?': show_help(); exit(1);
      default: goto switcherr; }
   else {
switcherr: assert(false, "bad option: %s\n", option); }
   return true; }

int parse_cmdline_switches(int argc, char *argv[]) {
   /* returns the index of the first argument that is not an option;
   or 0 if there isn't one */
   int i, firstnonoption = 0;
   //for (i = 0; i < argc; ++i) printf("arg %d: \"%s\"\n", i, argv[i]);
   for (i = 1; i < argc; i++) {
      if (!parse_cmdline_switch(argv[i])) { // end of switches
         firstnonoption = i;
         break; } }
   return firstnonoption; }


//
//  Parsing routines for translations.txt commands
//
void parse_tagname(char **pptr, char *dst) {
   int strlen = 0;
   while (**pptr != '>') {
      assert(**pptr, "HTML tagname not terminated: %s\n%s", *pptr, line);
      *dst++ = *(*pptr)++;
      assert(++strlen < MAX_STR - 2, "HTML tagname too big: %s\n%s", *pptr, line); }
   *dst = 0;
   ++*pptr; skip_blanks(pptr); }

char parse_char(char **p) { // get one character, possibly \r \n \t
   char next = *(*p)++;
   if (next == '\\') {
      next = *(*p)++; // skip \ and get following char
      if (next == 'r') next = 0x0d; // carriage return
      else if (next == 'n') next = 0x0a; // linefeed
      else if (next == 't') next = 0x09; // tab
      else if (next == '\\') next = 0x5c; // backslash
      else assert(false, "bad escape character: %s\n", *p); }
   return next; }

void parse_string(char **p, byte *dst, int *dstndx) {
   // parse: "abc", 'abc', \n, \r, \t, \\, or xxxx in hex
   char delim = **p;
   if (delim == '"' || delim == '\'') { // accumulate a character string
      ++*p;
      while (**p != delim) { // for each character inside the string
         assert(*dstndx < MAX_STR, "string too long: %s\n%s", *p, line);
         dst[(*dstndx)++] = *(*p)++; // or parse_char(p) to allow escape chars
         assert(**p, "unterminated string: %s\n%s", *p, line); }
      ++*p; }
   else if (delim == '\\') // escaped single character
      dst[(*dstndx)++] = parse_char(p);
   else { // accmulate a hex string
      bool firstnibble = true;
      while (1) { // for each nibble
         char ch = *(*p)++;
         if (ch == ' ' || ch == '\n' || ch == ';') break;
         if (ch >= '0' && ch <= '9') ch -= '0';
         else if (ch >= 'A' && ch <= 'F') ch -= 'A' - 10;
         else if (ch >= 'a' && ch <= 'f') ch -= 'a' - 10;
         else assert(false, "unknown search or replacment term: %s\n%s", *p - 1, line);
         assert(*dstndx < MAX_STR, "string too long: %s\n%s", *p - 1, line);
         if (firstnibble) dst[*dstndx] = ch;
         else {
            dst[*dstndx] = (dst[*dstndx] << 4) + ch;
            ++*dstndx; }
         firstnibble = !firstnibble; }
      assert(firstnibble, "odd number of hex chars: %s\n%s", *p - 1, line); }
   skip_blanks(p); }

unsigned int parse_flagnum(char **pptr) { // parse: nn>
   // returns a mask marking the bit position of the flag in a 32-bit word
   int flagnum;
   assert(scan_number(pptr, &flagnum) && flagnum >= 0 && flagnum <= MAX_MATCHFLAG,
          "bad match flag number: %s\n%s", *pptr, line);
   assert(scan_keyword(pptr, ">"), "missing > after flag number: %s\n%s", *pptr, line);
   return 1 << flagnum; }

void parse_options(char **pptr) {
   while (!skip_blanks(pptr))
      if (scan_keyword(pptr, "skipheaders")) skip_areas |= AREA_HEADERS;
      else if (scan_keyword(pptr, "skipbody")) skip_areas |= AREA_BODY;
      else if (scan_keyword(pptr, "skiptoc")) skip_areas |= AREA_TOC;
      else if (scan_keyword(pptr, "skipdeleted")) skip_areas |= AREA_DELETED;
      else if (scan_keyword(pptr, "skipfilenames")) skip_areas |= AREA_FILENAMES;
      else if (scan_keyword(pptr, "noeudora")) noeudora = true;
      else if (scan_keyword(pptr, "nologging")) logging = false;
      else if (scan_keyword(pptr, "eudoraokforsystemmailboxes")) okeudoraalways = true;
      else if (scan_keyword(pptr, "checksync")) checksync = true;
      else if (scan_keyword(pptr, "onlydo") ||
               (scan_keyword(pptr, "only") && scan_keyword(pptr, "do"))) {
         int kmbytes;
         assert(scan_number(pptr, &kmbytes) && kmbytes > 0 && kmbytes < 2048,
                "bad number after \"onlydo\": %s", line);
         if (scan_keyword(pptr, "kb"))
            bytes_to_do = ((unsigned long)kmbytes) << 10;
         else if (scan_keyword(pptr, "mb"))
            bytes_to_do = ((unsigned long)kmbytes) << 20;
         else assert(false, "missing KB or MB: %s", line); }
      else assert(false, "invalid option: %s\n%s", *pptr, line); }

void read_translations(void) { // read and parse all lines in the translations file
   FILE *text_fid;
   printf("reading translations from %s...\n", translation_filename);
   assert(text_fid = fopen(translation_filename, "r"), "Can't open %s ", translation_filename);
   while (fgets(line, LINESIZE, text_fid)) {
      //printf("%s", line);
      //if (line[strlen(line) - 1] != '\n') printf("\n");
      char *ptr = line;
      if (skip_blanks(&ptr)) continue; // skip blank or comment line
      if (scan_keyword(&ptr, "options")) parse_options(&ptr);
      else if (scan_keyword(&ptr, "option"))  parse_options(&ptr);
      else { // must be a translation specification
         struct translation_t *tp = &translation[num_translations];
         char *startptr;
         do {  // process search string modifiers in any order
            startptr = ptr;
            if (scan_keyword(&ptr, "<headers>")) tp->srch_options |= OPTSRCH_HEADERS;
            if (scan_keyword(&ptr, "<body>")) tp->srch_options |= OPTSRCH_BODY;
            if (scan_keyword(&ptr, "<ignorecase>")) tp->srch_options |= OPTSRCH_IGNORECASE;
            if (scan_keyword(&ptr, "<html ")) {
               tp->htmltag[0] = '<';
               parse_tagname(&ptr, tp->htmltag + 1);
               tp->srch_options |= OPTSRCH_HTMLTAGMATCH; }
            if (scan_keyword(&ptr, "<ifmatch ")) tp->testflags |= parse_flagnum(&ptr); }
         while (ptr > startptr);
         while (*ptr != '=') { // parse all parts of the search string
            if (*ptr == '!') {
               tp->srch.flag[tp->srch.len] |= FL_NOT; // flag the next char as "not this char"
               skip_blanks(&++ptr); } // skip ! and following blanks
            if (*ptr == '*') {
               tp->srch.flag[tp->srch.len] |= FL_ANYNUMBER; // flag the next char as "zero or more occurrences"
               skip_blanks(&++ptr); } // skip * and following blanks
            if (scan_keyword(&ptr, "<whitespace>")) {
               assert(tp->srch.flag[tp->srch.len] == 0, "! or  * is not allowed with <whitespace>: %s\n", line);
               tp->srch.flag[tp->srch.len] |= FL_WHITESPACE;
               tp->srch.str[tp->srch.len++] = ' '; } // (the character is irrelevant)
            else parse_string(&ptr, tp->srch.str, &tp->srch.len); }
         assert(tp->srch.len > 0, "missing search string in %s", line);
         for (int ndx = 0; ndx < num_translations; ++ndx)  // check for an exact duplicate search string
            assert(translation[ndx].srch.len != tp->srch.len
                   || memcmp(translation[ndx].srch.str, tp->srch.str, tp->srch.len) != 0
                   || memcmp(translation[ndx].srch.flag, tp->srch.flag, tp->srch.len) != 0
                   || translation[ndx].testflags != tp->testflags
                   || translation[ndx].srch_options != tp->srch_options,
                   "duplicate search string: %s", line);
         ++ptr; // skip =
         while (!skip_blanks(&ptr)) { // parse all parts of the replacement string
            if (scan_keyword(&ptr, "*")) {
               assert(tp->srch.flag[tp->repl.len] & FL_NOT, "* in replacement not matched by ! in search string: %s", line);
               tp->repl.flag[tp->repl.len] |= FL_ORIG;
               tp->repl.str[tp->repl.len++] = '*'; }
            else if (scan_keyword(&ptr, "<setmatch ")) tp->setflags |= parse_flagnum(&ptr);
            else if (scan_keyword(&ptr, "<clearmatch ")) tp->clrflags |= parse_flagnum(&ptr);
            else if (scan_keyword(&ptr, "<blanks>")) {
               assert(tp->repl.len == 0, "<blanks> in replacement isn't by itself: %s", line);
               tp->repl_options |= OPTREPL_BLANKS;
               tp->repl.len = tp->srch.len; }
            else if (scan_keyword(&ptr, "<zeros>") || scan_keyword(&ptr, "<nothing>")) {
               assert(tp->repl.len == 0, "<zeros> in replacement isn't by itself: %s", line);
               tp->repl_options |= OPTREPL_ZEROS; }
            else if (scan_keyword(&ptr, "<nochange>")) {
               assert(tp->repl.len == 0, "<nochange> in replacement isn't by itself: %s", line);
               tp->repl_options |= OPTREPL_NOCHANGE;
               memcpy(tp->repl.str, tp->srch.str, tp->repl.len = tp->srch.len); }
            else if (scan_keyword(&ptr, "<fixattachment>")) {
               if (!DEBUG && !FAKE_RENAME)  // if production version
                  assert(skip_areas & AREA_DELETED, "cannot use <fixattachment> unless \"option skipdeleted\" has been given");
               tp->repl_options |= OPTREPL_FIXATTACH; }
            else if (scan_keyword(&ptr, "<blankpad>")) {
               assert((tp->repl_options & OPTREPL_BLANKPAD) == 0, "<blankpad> can only appear once: %s", line);
               tp->repl_options |= OPTREPL_BLANKPAD;
               if (tp->repl.len == 0) tp->repl.flag[0] |= FL_BLANKPADBEFORE;  // before first replacement char
               else tp->repl.flag[tp->repl.len-1] |= FL_BLANKPADAFTER;  // after the previous replacement char
            }
            else { // must be a string or hex
               assert((tp->repl_options & ~OPTREPL_BLANKPAD) == 0, "can't also specify string: %s", line);
               parse_string(&ptr, tp->repl.str, &tp->repl.len);
               if (tp->repl.len == 0) tp->repl_options |= OPTREPL_ZEROS; } }
         assert(tp->repl.len > 0 || tp->repl_options != 0, "missing replacement in %s", line);
         if (scan_keyword(&ptr, ";")) { // start of a comment?
            int commentlength = strlen(ptr);
            if (commentlength > 0) { // record the comment for later reporting
               if (ptr[commentlength - 1] == '\n') --commentlength;
               strncpy(tp->comment, ptr, min(commentlength, COMMENTSIZE - 1)); } }
         else assert(*ptr == 0 || *ptr == '\n', "unknown extra text: %s\n%s", ptr, line);
         // We used to check "replacement too long" here, but <whitespace> and *' ' can
         // both allow the search part to be larger than what's in the translation rule.
         //assert(tp->repl.len <= tp->srch.len, "replacement longer than search string: %s", line);
         assert(++num_translations < MAX_TRANSLATIONS, "too many translations: %d", num_translations); } }
   fclose(text_fid);
   printf("processed and stored %d translations\n", num_translations); }
//
//    helper routines
//
bool eudora_running(void) {
   char buf[LINESIZE];
#define EUDORA_NAME "Eudora.exe"
   snprintf(buf, LINESIZE, "tasklist /FI \"IMAGENAME eq " EUDORA_NAME"\"");
   if (DEBUG)printf("%s\n", buf);
   bool running = false;
   FILE *fp = _popen(buf, "r");
   assert(fp, "can't start tasklist\n");
   while (fgets(buf, LINESIZE, fp)) {
      if (DEBUG) printf("tasklist: %s", buf);
      // see if any line starts with the Eudora executable file name
      if (strncmp(buf, EUDORA_NAME, sizeof(EUDORA_NAME) - 1) == 0)
         running = true; }
   assert(_pclose(fp) == 0, "can't find 'tasklist' command; check path\n");
   return running; }

void update_filetime(const char *filepath) {
#if 0  // this technique doesn't work if a path was specified and we're not running in the same directory
   char stringbuf[MAXFILENAME];
   snprintf(stringbuf, MAXFILENAME, "copy /b %s+,,", filepath);
   if (DEBUG) printf("%s\n", stringbuf);
   int rc = system(stringbuf); // update timestamp of the corresponding TOC file
   printf("%s timestamp %s\n", argv[1], rc == 0 ? "updated" : "could not be updated");
#endif
   // this technique seems to work in more cases
   SYSTEMTIME thesystemtime;
   GetSystemTime(&thesystemtime); // get time now
   FILETIME thefiletime;
   SystemTimeToFileTime(&thesystemtime, &thefiletime); // convert to file time format
   printf("updating timestamp of %s...\n", filepath);
   HANDLE filehandle = // get a handle that allow atttributes to be written
      CreateFile(filepath, FILE_WRITE_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
   assert(filehandle != INVALID_HANDLE_VALUE, "can't open handle to update timestamp");
   assert(SetFileTime(filehandle, (LPFILETIME)NULL, (LPFILETIME)NULL, &thefiletime), "can't update timestamp");
   CloseHandle(filehandle); }

char *format_filesize(unsigned long val, bool plural) {
   static char buf[30]; // WARNING: returns pointer to static string
   if (val < 1024) snprintf(buf, 30, "%ld byte%s", val, plural ? "s" : "");
   else if (val < 1024 * 1024) snprintf(buf, 30, "%ld KB", (val + 512) >> 10);
   else snprintf(buf, 30, "%ld MB", (val + 512 * 1024) >> 20);
   return buf; }

bool compare_names(char *ptr, const char *keyword) {
   do if (tolower(*ptr++) != tolower(*keyword++)) return false;
   while (*keyword && *ptr);
   return *keyword == *ptr; }

/*  This is a special kludge to fix attachment filenames that were truncated
    by Eudora because the name was split by Outlook onto multiple lines and
    Eudora only uses the first line.

   This is a problem that happens as Eudora reads the message from the server
   and processes the MIME-encoded file attachment. By the time the message is put
   in the Eudora mbx file, the damage has already be done. But there is enough
   evidence left in other header lines to enable us to repair the damage.

   We expect (and verify) that we were triggered by this rule in translations.txt:

   <body> <body> \r\n 'Content-Disposition: attachment;' <whitespace> 'filename' = <fixattachment>

   What should be in the message at the current position, somewhere after the
   HTML body of the message text, is something like this:

        \r\n
        Content-Disposition: attachment; \r\n
        \tfilename="filename_pt1"\r\n
          filename_pt2\r\n
          filename_pt3.ext"; somejunk\r\n
          morejunk\r\n
       Attachment Converted: "truncatedpathname"

  where there may be zero or more blanks after "attachment;".

  An alternative form, using the =*n multiple parameter "obvious solution"
  to breaking up long parameters described in RFC 2184, looks like this:

        \r\n
        Content-Disposition: attachment; \r\n
        \tfilename*0="filename_pt1"\r\n
          filename_pt2"; filename*1="pt3.ext"; somejunk\r\n
          morejunk\r\n
       Attachment Converted: "truncatedpathname"

  The full filename originally was all of the parts concatenated, but
  Eudora only uses filename_pt1. It prepends that with the path to its
  attachment directory to create the truncatedpathname that it uses in
  the new Attachment Converted header that it inserts into the mailbox.

  Note the extra (bogus) terminating " that Eudora inserted at the end of
  filename_pt1, which isn't in the data that it received from the mail server.

  Of course sometimes the whole filename is on only one line. The only way we
  can tell is if the next line does NOT start with a blank or a tab, in which case
  it is probably another header keyword. So the heuristic that seems to work is
  that the end of the original full filename is either  ";  or  "\r\nx, where x
  is not blank or tab. No doubt we'll discover other cases that have to be added.

  What we do is rename truncatedpathname to fullpathname, which we create by using
  the directory path taken from the Attachment Converted truncatedpathname, followed
  by the full pathname we reassembled. If the rename fails because the file exists,
  we try adding extra digits 1, 2, ..., 9 just before the extension.

  If the rename succeeds, we then replace all the text shown above with this:
       Attachment Converted: <40 blanks> "fullpathname"
       <blank padding>

  Why the 40 blanks after "Attachment Converted:"? So that if you want to also apply a
  translation rule that changes the attachment directory, that new directory can be as
  much as 40 characters longer than the old one. We know we have 40 spaces available in
  the mailbox because we have removed at least "Content-Disposition: attachment filename".

  Note this strange effect: If you have put a copy of the message in another mailbox using
  shift-Transfer, there is only one copy of any attachments they refer to. So if you use this
  on one of the message copies to fix an attachment filename, the other copy's reference to
  the attachment will be wrong because it isn't also updated to reflect the new name.

  There was also a problem with it renaming files that are pointed to by deleted messages
  still lurking in the mailbox because it hasn't been compacted. To avoid the havoc that
  that can cause, we now prohibit <fixattachment> from being specified unless you also
  specify "options skipdeleted", which causes deleted messages not to be processed.
*/

#include <wchar.h>
int do_rename(const char *oldname, const char *newname) {
   if (DEBUG>1) {
      showhex("old: ", oldname);
      showhex("new: ", newname); }
#if 0
   // rename, allowing long filenames
   // This was an attempt to get around the 260-character filename limit of "components" between \...\, but it doesn't work.
   // Apparently that limit, lpMaximumComponentLength, is never other than 260, or maybe 255, or maybe less.
   // This approach *may*, however, allow the total path length to be longer, so we leave it in.
   // FLASH: it may have bad side-effects for special characters above 0x80, so we don't do it anymore!
   
   // in a futile attempt to handle UTF-8, try prepending with "\\?\"
   //char oldname_plus[MAXFILENAME + 4], newname_plus[MAXFILENAME + 4];
   //strcpy(oldname_plus, "\\\\?\\"); strcpy(oldname_plus + 4, oldname);
   //strcpy(newname_plus, "\\\\?\\"); strcpy(newname_plus + 4, newname);
   //if (DEBUG > 1) {
     // showhex(oldname_plus);
     // showhex(newname_plus); }
   wchar_t wide_oldname[MAXFILENAME * 2 + 2], wide_newname[MAXFILENAME * 2 + 2]; // wide character versions of the filenames
   size_t charcount;
   errno_t err;
   assert((err = mbstowcs_s(&charcount, wide_oldname, oldname, MAXFILENAME)) == 0, "wide-character conversion of oldname failed:\n  %s\n", _strerror(NULL));
   assert((err = mbstowcs_s(&charcount, wide_newname, newname, MAXFILENAME)) == 0, "wide-character conversion of newname failed:\n  %s\n", _strerror(NULL));
   return _wrename(wide_oldname, wide_newname);
#else
   //printf("chcp returns %d\n", system("chcp"));
   //printf("chcp 1252 returns %d\n", system("chcp 1252"));
   return rename(oldname, newname);  // just do the old-fashioned 8-bit character rename
#endif
}

bool do_fixattachment(struct translation_t *t, int buf_ndx) {
   char fullfilename[MAXFILENAME];
   char truncatedpathname[MAXFILENAME];
   int fullfilename_ndx = 0, truncatedpathname_ndx = 0;
   enum { // the states we go through as we scan the buffer
      FIXA_ACCUM_FULLNAME, FIXA_ACCUM_SKIPBLANKS, FIXA_LOOKFOR_NEXTPART, // get the real filename from Content-Disposition:
      FIXA_LOOKFOR_TRUNCATEDNAME, FIXA_WAITFOR_QUOTE, FIXA_ACCUM_TRUNCATEDNAME,  // get Eudora's filename from Attachment Converted:
      FIXA_DOIT, // we have all the pieces: do it!
      FIXA_SEARCH_FAILED, FIXA_RENAME_UNNECESSARY, FIXA_RENAME_FAILED, FIXA_RENAME_DONE  // possible ending status
   } state = FIXA_ACCUM_FULLNAME;
   if (DEBUG) printf("\n***starting <fixattachment>\n");
   static const char searchstring_begin[] = "\r\nContent-Disposition: attachment;";
   static const char searchstring_final[] = "filename"; // only allow a little flexibility in the rule's search string
   assert(memcmp(t->srch.str, searchstring_begin, sizeof(searchstring_begin) - 1) == 0
          && memcmp(t->srch.str+t->srch.len-(sizeof(searchstring_final)-1), searchstring_final, sizeof(searchstring_final) - 1) == 0,
          "<fixattachment> has wrong search string");
   int section_start = bufferpos; // start of "\r\nContent-Disposition", which is where we eventually put our replacement
   bufferpos += buf_ndx; // point to after the translation rule search string, ie just after "filename"
   bool multiple_parts = false;  // were there actually multiple parts to the filename?
   bool multiple_filenames = false; // do we expect multiple filename*n="xxx" pieces?
   if (memcmp(bufferin + bufferpos, "*0", 2) == 0) { // we do if we see filename*0
      multiple_filenames = multiple_parts = true;
      bufferpos += 2;
      if (DEBUG) printf("expecting multiple filename*n=\"...\"\n"); }
   if (memcmp(bufferin + bufferpos, "=\"", 2) != 0)  // in either case we then expect ="
      state = FIXA_SEARCH_FAILED;
   else bufferpos += 2;  // skip the ="
   bool linestart = false;
   for (; bufferpos < bufferlen - 1; ++bufferpos) { // look for the various pieces we need
      byte nextchar = bufferin[bufferpos];
      if (bufferpos - section_start > 2000) // impose an arbitrary limit on how far we'll look
         state = FIXA_SEARCH_FAILED;
      if (linestart && memcmp(bufferin + bufferpos, "From ???@???", 12) == 0) { // stop looking if a new message starts
         bufferpos -= 2; // and backup so the new message start is seen by the main loop
         if (DEBUG) printf("ran into next message at offset %d\n", (int32_t)filepos1st + bufferpos);
         state = FIXA_SEARCH_FAILED; }
      switch (state) { // process one character according to our current state

      case FIXA_ACCUM_SKIPBLANKS: // skip excess blanks at the start of a continuation line
         if (nextchar == ' ' || nextchar == '\t') break;
         state = FIXA_ACCUM_FULLNAME; // end of blanks: treat as the next filename char
      // fall through to FIXA_ACCUM_FULLNAME

      case FIXA_ACCUM_FULLNAME: //accumulate a string of the original suggested full file name
         if (nextchar == '"') { // " ends this segment
            nextchar = bufferin[++bufferpos]; // ignore the " and get the next char
            if (nextchar == ';') { // "; ends the whole filename, or at least this part
               if (multiple_filenames) { // if we expect multiple parts
                  state = FIXA_LOOKFOR_NEXTPART; // start looking for next filename*n="xxx"
                  if (DEBUG) {
                     fullfilename[fullfilename_ndx] = 0;
                     printf("looking for next filename*n, so far got %s\n", fullfilename); }
                  break; }
               else goto gotfullname; } } // otherwise we're done
         if (nextchar == '\r' && bufferin[bufferpos + 1] == '\n') { // the line is ending
            if (bufferin[bufferpos + 2] != ' ' && bufferin[bufferpos + 2] != '\t')
               goto gotfullname; // next line doesn't start with whitespace, so we're done
            else { // this filename continues on the next line
               multiple_parts = true;
               fullfilename[fullfilename_ndx++] = ' '; // record only a single blank
               ++bufferpos; // skip CR, for(...) will skip LF
               state = FIXA_ACCUM_SKIPBLANKS; // and then skip all subsequent whitespace
            } }
         else if (fullfilename_ndx < MAXFILENAME - 1)
            fullfilename[fullfilename_ndx++] = nextchar; // accumulate another character
         break;

      case FIXA_LOOKFOR_NEXTPART: // look for the next filename*n="xxx" part
         if (strchr(WHITESPACE, nextchar) == NULL) {  // ignore whitespace
            if (memcmp(bufferin + bufferpos, "filename*", 9) == 0) {
               multiple_parts = true;
               if (DEBUG) printf("next filename segment: %.20s\n", bufferin + bufferpos);
               bufferpos += 10;  // skip filename*n (we ignore the numbers)
               if (memcmp(bufferin + bufferpos, "=\"", 2) != 0)  // we then expect ="
                  state = FIXA_SEARCH_FAILED;
               else state = FIXA_ACCUM_FULLNAME; } // start accumulating next part
            else {
               if (DEBUG) printf("ending search for next filename*n at %.20s\n", bufferin + bufferpos);
               goto gotfullname; } } // was something else: we're done
         break;
gotfullname:
         fullfilename[fullfilename_ndx] = 0;
         if (strlen(fullfilename) > 240) { // Windows lpMaximumComponentLength is 255, but the real limit for rename is something like this
            stopdots();
            log_printf("original file name too big: %s\n", fullfilename);
            state = FIXA_RENAME_FAILED;
            break; }
         if (DEBUG) printf("attachment fullfilename: %s\n", fullfilename);
         if (multiple_parts)
            state = FIXA_LOOKFOR_TRUNCATEDNAME;
         else {
            state = FIXA_RENAME_UNNECESSARY;
            if (DEBUG) printf("rename unnecessary since filename was in one part\n"); }
         break;

      case FIXA_LOOKFOR_TRUNCATEDNAME: {  //look for "Attachment Converted:"
         static const char attachment_tag[] = "\r\nAttachment Converted:";
         if (bufferpos < bufferlen - (int)sizeof(attachment_tag)
               && memcmp(bufferin + bufferpos, attachment_tag, sizeof(attachment_tag) - 1) == 0) {
            state = FIXA_WAITFOR_QUOTE;
            bufferpos += sizeof(attachment_tag) - 2; } // -1 to ignore null, -1 because the for loop will increment bufferpos
         break; }

      case FIXA_WAITFOR_QUOTE:  // skip spaces waiting for the quoted truncated name after "Attachment Converted:"
         if (nextchar == '"') state = FIXA_ACCUM_TRUNCATEDNAME;
         else if (nextchar != ' ') state = FIXA_SEARCH_FAILED;
         break;

      case FIXA_ACCUM_TRUNCATEDNAME: //accmulate the truncated attachment name
         if (nextchar == '"') {
            state = FIXA_DOIT;
            truncatedpathname[truncatedpathname_ndx] = 0;
            if (DEBUG) printf("attachment truncated name: %s\n", truncatedpathname); }
         else if (truncatedpathname_ndx < MAXFILENAME-1) {
            truncatedpathname[truncatedpathname_ndx++] = nextchar; }
         break;

      case FIXA_DOIT: //found all the pieces: try to make our changes
         char directoryname[MAXFILENAME]; // the directory part of the truncated path name
         char *truncatedfilename = strrchr(truncatedpathname, '\\'); // last backslash
         assert(truncatedfilename, "missing path for converted attachment: %s\n", truncatedpathname);
         int directorylength = ++truncatedfilename - truncatedpathname;
         assert(directorylength < MAXFILENAME-1, "path too big in converted attachment: %s\n", truncatedpathname);
         memcpy(directoryname, truncatedpathname, directorylength);
         directoryname[directorylength] = 0;
         if (DEBUG) printf("directory: %s\n", directoryname);
         if (strcmp(truncatedfilename, fullfilename) == 0) {
            if (DEBUG) printf("names match; no need to rename\n");
            state = FIXA_RENAME_UNNECESSARY; }
         else {
            char extension[MAXFILENAME]; // separate the extension
            char *filename_end = strrchr(fullfilename, '.'); // last .
            if (filename_end) { // there is an extension
               strcpy(extension, filename_end); // copy the extension, with the dot
               *filename_end = 0; } // and remove it from the fullfilename
            else extension[0] = 0;  // no extension
            if (DEBUG) printf("extension: \"%s\"\n", extension);
            char newfilename[MAXFILENAME];
            for (int tries = 1; ; ++tries) { // try renaming a few times
               if (tries == 1)
                  sprintf(newfilename, "%s%s%s", directoryname, fullfilename, extension);
               else
                  sprintf(newfilename, "%s%s%d%s", directoryname, fullfilename, tries, extension);
               if (DEBUG>1) printf("renaming \"%s\"\n      to \"%s\"\n", truncatedpathname, newfilename);
               if (FAKE_RENAME || do_rename(truncatedpathname, newfilename) == 0) {
                  stopdots();
                  log_printf(FAKE_RENAME ? "fake-renamed %s\n          to %s\n" : "renamed %s\n     to %s\n",
                             truncatedpathname, newfilename);
                  state = FIXA_RENAME_DONE;
                  break; }
               else {
                  if (errno != EEXIST /*file exists*/ || tries >= 10) {
                     stopdots();
                     log_printf("failed renaming %s\n             to %s\n  %s\n", truncatedpathname, newfilename, _strerror(NULL));
                     state = FIXA_RENAME_FAILED;
                     break; /*for tries*/
                  } } }
            if (state == FIXA_RENAME_DONE) {
               int nchars = sprintf((char *)bufferout + section_start, // put in our new restructed filename
                                    "\r\nAttachment Converted:"
                                    "                                        " // see comments in the prologue for why these blanks
                                    "\"%s\"\r\n", newfilename);
               assert(bufferpos - section_start > nchars, "not enough room to rewrite attachment name: %d\n", nchars);
               memset(bufferout + section_start + nchars, ' ', bufferpos - section_start - nchars); // blank pad
               if (section_start < BLKSIZE) dirty1st = true; else dirty2nd = true; //mark the start as dirty
               if (bufferpos >= BLKSIZE) dirty2nd = true; // if the end is in the second buffer
               // We copy our changes to bufferin so another rule that changes the attachment directory works.
               memcpy(bufferin + section_start, bufferout + section_start, bufferpos - section_start);
               bufferpos = section_start; // rescan starting with what we changed
               ++renamed_file_attachments; } }
         break; /*case FIXA_DOIT*/
      } //switch
      if (state == FIXA_RENAME_DONE || state == FIXA_RENAME_FAILED || state == FIXA_RENAME_UNNECESSARY || state == FIXA_SEARCH_FAILED)
         break; /*for (;bufferpos...) */
      linestart = nextchar == '\r' || nextchar == '\n'; // set up for detecting end of message
   } //for
   return state == FIXA_RENAME_DONE; }

//
//    search-and-replace routines
//

bool check_translation(struct translation_t *t, int fieldlen) {
   // See if this one translation matches at the current position in the buffer.
   // Do the replacement and return TRUE if so.
   int srch_ndx, buf_ndx;
   for (srch_ndx = buf_ndx = 0; srch_ndx < t->srch.len; ) { // check each search character
      byte thischar = bufferin[bufferpos + buf_ndx];
      bool match = t->srch_options & OPTSRCH_IGNORECASE
                   ? (lowercase(thischar) == lowercase(t->srch.str[srch_ndx]))
                   : (thischar == t->srch.str[srch_ndx]);
      if (t->srch.flag[srch_ndx] & FL_ANYNUMBER) { // "zero or more occurrences"?
         if (match) { // if it matches, just advance in the buffer
            if (bufferpos + ++buf_ndx >= bufferlen) {
               if (DEBUG) printf("***ran off end of buffer\n");
               break; }
            continue; }
         else { // if it (finally, or at first) doesn't match, just advance in the search string
            ++srch_ndx;
            continue; } }
      if (t->srch.flag[srch_ndx] & FL_WHITESPACE) { // "whitespace"?
         for (++srch_ndx; bufferpos + buf_ndx < bufferlen; ++buf_ndx) { // skip any whitespace
            if (strchr(WHITESPACE, bufferin[bufferpos + buf_ndx]) == NULL)
               break; } }
      else { // regular single-character match
         if (t->srch.flag[srch_ndx] & FL_NOT ? match : !match) break;
         ++srch_ndx; ++buf_ndx; } }
   if (srch_ndx == t->srch.len) { // all chars matched: do replacement
      bool made_change;
      if (DEBUG) printf("match at buffer pos %d file pos %lld: %s\n  result: ", bufferpos, filepos1st + bufferpos, t->comment);
      if (fieldlen == 0   // no field length means we're doing the mailbox
            && !(t->repl_options & OPTREPL_NOCHANGE)) { // and if we're reallly going to make a change
         // show in the map that we modified something in this area
         int mapndx = (int)(((filepos1st + bufferpos) * MAPWIDTH) / mbxsize); //64-bit arithmetic!
         if (mapndx < 0) mapndx = 0; if (mapndx >= MAPWIDTH) mapndx = MAPWIDTH - 1;
         map[mapndx] = '*'; }
      if (t->repl_options & OPTREPL_FIXATTACH)
         made_change = do_fixattachment(t, buf_ndx); // special kludge to fix truncated attachment filenames
      else if (t->repl_options & OPTREPL_NOCHANGE)
         made_change = false;
      else { // normal replacement
         made_change = true;
         int deficit = buf_ndx - t->repl.len; // replacement will be this many bytes too small
         assert(deficit >= 0, "replacement too long: %.50s\n at %.50s", t->repl.str, bufferin + bufferpos);
         for (int repl_ndx = buf_ndx = 0; repl_ndx < t->repl.len; ++repl_ndx) { // for each replacement char
            byte original = bufferout[bufferpos + buf_ndx]; // remember the original character, in case *
            if (t->repl.flag[repl_ndx] & FL_BLANKPADBEFORE)  // <blankpad> to the deficit before this character
               for (; deficit > 0; --deficit) bufferout[bufferpos + buf_ndx++] = ' ';
            bufferout[bufferpos + buf_ndx++] = // output one character:
               t->repl.flag[repl_ndx] & FL_ORIG ? original // FL_ORIG means original character (or a replacement from a previous match!)
               : t->repl_options & OPTREPL_BLANKS ? ' '    // we're replacing with all blanks
               : t->repl.str[repl_ndx];                    // we're replacing from the replacement string
            if (DEBUG) {
               byte ch = bufferout[bufferpos + buf_ndx - 1];
               printf(ch == ' ' ? "<b>" : ch > 31 && ch < 127 ? "%c" : "%02X ", ch); }
            if (t->repl.flag[repl_ndx] & FL_BLANKPADAFTER)  // <blankpad> to the deficit after this character
               for (; deficit > 0; --deficit) bufferout[bufferpos + buf_ndx++] = ' '; }
         if (DEBUG) printf("\n");
         if (bufferpos < BLKSIZE) dirty1st = true; else dirty2nd = true; //mark the start as dirty
         if (deficit > 0) { // still more deficit to do?
            if (fieldlen == 0) { // in mailbox: pad with zeros
               for (; deficit > 0; --deficit) bufferout[bufferpos + buf_ndx++] = 0; }
            else { // in TOC: slide the remaining field to the left
               for (int cnt = fieldlen -(buf_ndx + deficit); cnt > 0; --cnt) { // move chars left
                  bufferout[bufferpos + buf_ndx] = bufferin[bufferpos + buf_ndx] = bufferin[bufferpos + buf_ndx + deficit];
                  ++buf_ndx; }
               for (; buf_ndx < fieldlen; ++buf_ndx) // fill vacated spots at the right of the field with zeros
                  bufferout[bufferpos + buf_ndx] = bufferin[bufferpos + buf_ndx] = 0; } }
         if (bufferpos + buf_ndx >= BLKSIZE) dirty2nd = true; // ended in the second half, so it is dirty
      }
      matchflags |= t->setflags;  // set and clear global match flags as requested
      matchflags &= ~t->clrflags;
      ++t->usecount;
      if (made_change) ++total_changes;
      return true; }
   return false; }

bool check_TOC_translations(byte *field, int size) {
   // check for translations at all positions in a field of the TOC; return TRUE if we made a change
   bufferpos = field - bufferin;  // offset of the start of the message's field in the buffer
   assert(bufferlen - bufferpos >= size, "TOC file too small! " // not enough data left
          "need %d bufferlen %d bufferpos %d", size, bufferlen, bufferpos);
   bool made_change = false;
   while (size > 0) { // for each character in the field
      for (int xlate = 0; xlate < num_translations; ++xlate) { // try all translations at this position
         struct translation_t *t = &translation[xlate];
         if (size >= t->srch.len) // if there are enough characters left for this translation
            made_change |= check_translation(t, size); } // maybe do replacements, sliding instead of zero-padding
      ++bufferpos; --size; }
   return made_change; }

void save_msg_date(void) { // save the message date after "From ???@??? " in the buffer
   int ndx = 0;
   for (int pos = bufferpos + 13; pos < bufferlen && ndx < LINESIZE - 1; ++pos) {
      char ch = bufferin[pos];
      if (ch == '\n' || ch == '\r') break;
      msg_date[ndx++] = ch; }
   msg_date[ndx] = 0; }

bool check_mbx_msg(struct Eudora_TOC_message_t *msg, int msgnum) {
   // check that the TOC message descriptor at msg points to
   // a place in the MBX where there is the start of a message
   char msgstart[12];
   fileseek(MBX, msg->offset);  // go to the reputed start of the message
   fileread(MBX, msgstart, 12);
   if (memcmp(msgstart, "From ???@???", 12) == 0) return true;
   stopdots();
   log_printf("warning: message %d in the TOC doesn't point to a message in the MBX\n", msgnum);
   log_printf("  date: %s\n", msg->date_time);
   log_printf("  address: %s\n", msg->sender_recipient);
   log_printf("  subject: %s\n", msg->subject);
   log_printf("  mbx data: ");
   for (int i = 0; i < 12; ++i)
      log_printf(msgstart[i] > 31 && msgstart[i] < 127 ? "%c" : "[%02X]", msgstart[i]);
   log_printf("\n");
   return false; }

// if we are not to process deleted message, read the TOC file
//  and record where the non-deleted messages are located

int32_t *valid_msg_offsets = NULL;  // array of offsets into the mailbox for valid messages in the TOC
int TOC_nummsgs = 0;
LONGLONG msgcheck_totaltime;

void preread_TOC(void) {
   struct Eudora_TOC_header_t TOC_header;
   struct Eudora_TOC_message_t TOC_msg;
   assert(fileread(TOC, &TOC_header, sizeof(struct Eudora_TOC_header_t)) == sizeof(struct Eudora_TOC_header_t),
          "can't read TOC header");
   TOC_nummsgs = TOC_header.SumCount;
   assert(valid_msg_offsets = (int32_t *)malloc(TOC_nummsgs * sizeof(int32_t)),
          "can't allocate msg offset array for %d messages", TOC_nummsgs);
   if (RECORD_MSG_OFFSETS) {
      assert(debugfile = fopen("debug.txt", "w"), "Can't open debug.txt");
      fprintf(debugfile, "TOC message offsets\n"); }
   for (int msgnum = 0; msgnum < TOC_nummsgs; ++msgnum) { // for each message
      assert(fileread(TOC, &TOC_msg, sizeof(struct Eudora_TOC_message_t)) == sizeof(struct Eudora_TOC_message_t),
             "can't read TOC entry %d", msgnum+1);
      if (RECORD_MSG_OFFSETS) fprintf(debugfile, "%d,%d\n", msgnum + 1, TOC_msg.offset);
      valid_msg_offsets[msgnum] = TOC_msg.offset; } // record the offset in the mailbox
   if (RECORD_MSG_OFFSETS) fprintf(debugfile, "MBX message offsets\n");
   fileseek(TOC, 0); } // reset to beginning so we can read it again

bool msg_deleted(int32_t offset) { // is the specified offset that of a deleted message?
   // The commented out code was used to prove that this N**2 array lookup
   // takes negligible time, even for large mailboxes.
   //LARGE_INTEGER msgcheck_starttime;
   //LARGE_INTEGER msgcheck_endtime;
   //QueryPerformanceCounter(&msgcheck_starttime);
   bool answer;
   for (int msgnum = 0; msgnum < TOC_nummsgs; ++msgnum) {
      if (valid_msg_offsets[msgnum] == offset) { // is it in the TOC?
         answer = false; // yes, so it's not deleted
         goto gettime; } }
   answer = true; // it is missing from the TOC, so it must have been deleted
gettime:
   //QueryPerformanceCounter(&msgcheck_endtime);
   //msgcheck_totaltime += msgcheck_endtime.QuadPart - msgcheck_starttime.QuadPart;
   return answer; }

//
//    main routine
//
int main(int argc, char **argv) {
   clock_t start_time = clock();

   //********** process command line parameters

   printf("Eudora_fix_mbx, version %s, (c)L.Shustek 2021-2022\n", VERSION);
   if (argc <= 1) {
      show_help();
      exit(4); }
   int argno = parse_cmdline_switches(argc, argv);
   assert(argno != 0, "no mailbox name given\n");
   assert(argc <= argno + 1, "extra argument: %s  (If the mailbox name contains blanks, enclose it in quotes.)", argv[argno + 1]);
   if (DEBUG) printf("translation array uses %d bytes, and buffers use %d bytes\n",
                        sizeof(translation), sizeof(bufferin) + sizeof(bufferout));

   //********** open the log file and read the translations file

   char basefile[MAXFILENAME], mbxpath[MAXFILENAME], tocpath[MAXFILENAME];
   strncpy(basefile, argv[argno], MAXFILENAME); basefile[MAXFILENAME - 1] = 0;
   int baselength = strlen(basefile); // remove .mbx extension to allow drag-and-drop of mailbox file onto the icon
   if (baselength > 4 &&
         (strcmp(basefile + baselength - 4, ".mbx") == 0 ||
          strcmp(basefile + baselength - 4, ".MBX") == 0)) {
      basefile[baselength - 4] = 0; }

   char *mbxname = basefile; // find pointer to mailbox name: after the last \, if any
   for (char *ptr = basefile; *ptr; ++ptr)
      if (*ptr == '\\') mbxname = ptr + 1;

   if (logging) {
      time_t rawtime;
      time(&rawtime);
      assert(logfile = fopen(log_filename, "a"), "Can't open %s ", log_filename);
      fprintf(logfile, "Eudora_fix_mbx version %s run for mailbox \"%s\" on %s",
              VERSION, basefile, asctime(localtime(&rawtime))); }
   read_translations();
   //show_stats();

   //******** open the mailbox and table-of-contents files

   if (!okeudoraalways) { // check the rules for whether Eudora can be running
      if (noeudora) assert(!eudora_running(), "Eudora is running; stop it first");
      else if (compare_names(mbxname, "In") || compare_names(mbxname, "Out")
               || compare_names(mbxname, "Trash") || compare_names(mbxname, "Junk"))
         assert(!eudora_running(), "Fixing system mailbox, but Eudora is running; stop it first"); }

   snprintf(mbxpath, MAXFILENAME, "%s.mbx", basefile);
   fileopen(MBX, mbxpath);
   snprintf(tocpath, MAXFILENAME, "%s.toc", basefile);
   if (!(skip_areas & AREA_TOC) || skip_areas & AREA_DELETED) fileopen(TOC, tocpath);
   if (skip_areas & AREA_DELETED) {
      printf("pre-reading table-of-contents %s...\n", tocpath);
      preread_TOC(); }

   //******* process messages in the mailbox file

   mbxsize = (unsigned long)filesize(MBX);
   if (mbxsize > bytes_to_do) {  // we were asked to only do the end of the mailbox
      bytes_to_skip = mbxsize - bytes_to_do;
      log_printf("skipping the first %s of mailbox %s\n", format_filesize(bytes_to_skip, true), mbxpath);
      fileseek(MBX, bytes_to_skip); }
   memset(map, '_', MAPWIDTH);  // initialize the mailbox map
   map[MAPWIDTH] = 0;
   init_buffer(MBX);   // initialize the buffer
   if (bytes_to_skip) { // if we skipped bytes, keep skipping to the start of a message
      bool linestart = false; // assume we start at an arbitrary place in text
      do {
         chk_buffer(MBX, false); // keep bufferpos in the first half of the buffer
         if (linestart && memcmp(bufferin + bufferpos, "From ???@???", 12) == 0)
            break;
         linestart = bufferin[bufferpos] == '\r' || bufferin[bufferpos] == '\n';
         ++bufferpos; }
      while (bufferpos < bufferlen);
      assert(bufferpos < bufferlen, "didn't find a message after skipping");
      save_msg_date();
      log_printf("starting with the message from %s\n", msg_date); }
   printf("reading mailbox %s...", mbxpath);  neednewline = true; // note that we didn't print \n so we can keep adding dots
   if (DEBUG > 1) stopdots(); // (well, maybe)
   bool message_changed = false;
   char prevprev_char = '\r', prev_char = '\n'; // for detecting whether we're after a CRLF, which we assume initially
   byte area = 0;  // which area we're in: AREA_xxxx
   do { // read and search the mailbox for changes to be made
      chk_buffer(MBX, false); // keep bufferpos in the first half of the buffer
      // keep track of what area of the message we are in

      if (prevprev_char == '\r' && prev_char == '\n') { // we're just after a CRLF
         if (memcmp(bufferin + bufferpos, "From ???@???", 12) == 0) { // start of a new message
            if (DEBUG > 1 && area & AREA_FILENAMES) printf("auto-exiting filename area at next message\n");
            area = AREA_HEADERS;  // we're now in the headers and nothing else
            if (RECORD_MSG_OFFSETS) fprintf(debugfile, "%d,%d\n", num_messages + 1, (int32_t)filepos1st + bufferpos);
            if (skip_areas & AREA_DELETED && msg_deleted((int32_t)filepos1st + bufferpos)) {
               area |= AREA_DELETED; // we were asked not to do deleted messages, so we're in the "deleted" area
               ++deleted_msgs_skipped;
               if (DEBUG) printf("skipping deleted message from %.26s\n", bufferin + bufferpos + 12); }
            matchflags = 0; // in case a <setmatch n> didn't find its matching <clearmatch n> in the previous message
            ++num_messages;
            if (total_changes == 0) save_msg_date(); // keep saving the message date until we make the first change
            if (message_changed) { // did we make any changes to the previous message?
               ++MBX_messages_changed;
               message_changed = false; } }
         else if (area & AREA_HEADERS && memcmp(bufferin + bufferpos, "\r\n", 2) == 0) { // CRLFCRLF exits from headers to body
            area &= ~AREA_HEADERS;
            area |= AREA_BODY; }
         if (area & AREA_BODY && skip_areas & AREA_FILENAMES) { //do filename detection
            /* With "option skipfilenames" specified, we protect the following MIME "headers" from having translations made:
                Content-Disposition:
                Content-Description:
                Content-Type:
                Attachment Converted:   (Eudora's pseudo-MIME header) */
            if (memcmp(bufferin + bufferpos, "Attachment Converted:", 21) == 0) {// check for entering a filename area
               area |= AREA_FILENAMES;
               if (DEBUG > 1) printf("entering filename area at %.20s\n", bufferin + bufferpos); }
            else if (memcmp(bufferin + bufferpos, "Content-", 8) == 0) { // check for Content-xxx headers that have filenames
               if (memcmp(bufferin + bufferpos + 8, "Disposition:", 12) == 0
                     || memcmp(bufferin + bufferpos + 8, "Description:", 12) == 0
                     || memcmp(bufferin + bufferpos + 8, "Type:", 5) == 0) {
                  area |= AREA_FILENAMES;
                  if (DEBUG > 1) printf("entering filename area at %.20s\n", bufferin + bufferpos); } } } }

      if (area & AREA_FILENAMES  // exit filename header area at CRLF not followed by blank or tab
            && bufferin[bufferpos] == '\r' && bufferin[bufferpos+1] == '\n'
            && bufferin[bufferpos+2] != ' ' && bufferin[bufferpos+2] != '\t') { // exit filename header area at CRLF not followed by blank or tab
         area &= ~AREA_FILENAMES;
         if (DEBUG > 1) printf("exiting filename area at %.20s\n", bufferin + bufferpos); }

      if (!(area & skip_areas)) { // check if we're in the right area to do replacements, and also screen deleted msgs
         // try each translation at this position
         for (int xlate = 0; xlate < num_translations; ++xlate) {
            struct translation_t *t = &translation[xlate];
            // Check for entering an HTML tag we are asked to match. Unlike the "inheader" boolean,
            // STAT_TAGMATCH isn't a global because it only applies to specific translations.
            if (t->srch_options & OPTSRCH_HTMLTAGMATCH
                  && bufferlen - bufferpos >= (int)strlen(t->htmltag) // enough data left to check
                  && memcmp(bufferin + bufferpos, t->htmltag, strlen(t->htmltag)) == 0) // check for match
               t->status |= STAT_TAGMATCH; // entering the HTML tag
            if (t->status & STAT_TAGMATCH && bufferin[bufferpos] == '>') // exiting the HTML tag
               t->status &= ~STAT_TAGMATCH;
            //see if search modifiers are met
            if (t->srch_options & OPTSRCH_HEADERS && !(area & AREA_HEADERS)) continue; // not in header: go to next translation
            if (t->srch_options & OPTSRCH_BODY && !(area & AREA_BODY)) continue;     // not in body: same
            if (t->srch_options & OPTSRCH_HTMLTAGMATCH && !(t->status & STAT_TAGMATCH)) continue; // not in matching HTML tag: same
            if (t->testflags && (t->testflags & matchflags) != t->testflags) continue;   // not all specified test flags match: same
            if (bufferlen - bufferpos < t->srch.len) continue; // not enough data left to check the search string: same
            message_changed |= check_translation(t, 0); // finally: check for searchstring match and do replacement if so
         } // continue looking for more matches even after doing a replacement
      }
      prevprev_char = prev_char;
      prev_char = bufferin[bufferpos];
      ++bufferpos; } // next position in the mailbox
   while (bufferpos < bufferlen);
   // cleanup
   if (message_changed) ++MBX_messages_changed; // if last message was changed
   while (bufferlen > 0) chk_buffer(MBX, true); // write out any modified parts of the last buffer
   //delay until later, so file stays locked:  fileclose(MBX);
   stopdots();
   if (total_changes > 0) log_printf("the first changed message is from %s\n", msg_date);
   mailbox_changes = total_changes; // everything so far has been a mailbox change
   if (RECORD_MSG_OFFSETS) fclose(debugfile);

   //********* process the subject and address fields in the table of contents file

   if (!(skip_areas & AREA_TOC)) { // unless "options skiptoc"
      printf("reading table-of-contents %s...", tocpath); neednewline = true; // note that we didn't print \n so we can keep adding dots
      if (DEBUG > 1) stopdots(); // (well, maybe)
      assert(fileread(TOC, bufferin, sizeof(struct Eudora_TOC_header_t)) == sizeof(struct Eudora_TOC_header_t),
             "can't read TOC header");
      struct Eudora_TOC_header_t *hdr = (struct Eudora_TOC_header_t *) bufferin;
      int TOC_nummsgs = hdr->SumCount;
      int numbadsyncs = 0;
      //printf("%d messages", hdr->SumCount);
      init_buffer(TOC);   // initialize the buffer with TOC message descriptors
      for (int msgsleft = TOC_nummsgs; msgsleft > 0; --msgsleft) { // for each message
         chk_buffer(TOC, false); // beware: this might change bufferpos!
         struct Eudora_TOC_message_t *msg = // ptr to the message descriptor
            (struct Eudora_TOC_message_t *) (bufferin + bufferpos);
         //printf("msgcnt %d at bufferpos %d, bufferlen %d, datetime %s\n",
         //       TOC_nummsgs, bufferpos, bufferlen, msg->date_time);
         if (checksync) // maybe check if MBX has a message in the right place
            if (!check_mbx_msg(msg, TOC_nummsgs - msgsleft + 1)) ++numbadsyncs;
         if ( // use | not || so both checks are always made
            check_TOC_translations(  // check for translations in the sender or recipient namelist
               (byte*)&(msg->sender_recipient), sizeof(msg->sender_recipient))
            | check_TOC_translations(  // check for translations in the subject field
               (byte*)&(msg->subject), sizeof(msg->subject)))
            ++TOC_messages_changed; // one or both of the fields were changed
         ++msg; // point to next message descriptor in the buffer
         bufferpos = (byte *)msg - bufferin; // move the buffer pointer there
      } // process the next message descriptor
      // cleanup
      while (bufferlen > 0) chk_buffer(TOC, true); // write out any modified parts of the last buffer
      fileclose(TOC);
      stopdots();
      if (checksync && TOC_nummsgs > 0 && numbadsyncs == 0)
         log_printf(TOC_nummsgs == 1 ? "the single TOC entry correctly points to a message header in the MBX\n"
                    : "all %d TOC entries correctly point to message headers in the MBX\n", TOC_nummsgs);
      if (bytes_to_skip == 0) { // can only compare MBX and TOC number of messages if we read the whole MBX
         if (TOC_nummsgs < num_messages)
            log_printf("the MBX has %d messages and the TOC has %d, so %d must have been deleted\n", num_messages, TOC_nummsgs, num_messages - TOC_nummsgs);
         else if (TOC_nummsgs > num_messages)
            log_printf("something's wrong: the MBX has %d messages, but the TOC has %d\n", num_messages, TOC_nummsgs); } }

   fileclose(MBX); // delayed closing, so it stayed locked to Eudora
   if (valid_msg_offsets) {
      free(valid_msg_offsets);
      valid_msg_offsets = NULL; }
//
//      final reporting
//
   if (0 && DEBUG) { // It turns out that checking for deleted messages take negligible time...
      LARGE_INTEGER ticks_per_second;
      QueryPerformanceFrequency(&ticks_per_second);
      printf("**time taken to check for deleted messages: %.4f seconds\n",
             (float)msgcheck_totaltime / float(ticks_per_second.QuadPart)); }

   if (deleted_msgs_skipped)
      log_printf("%d deleted message%s skipped\n",
                 deleted_msgs_skipped, deleted_msgs_skipped == 1 ? " was" : "s were");
   if (total_changes == 0) {
      log_printf("no changes were made in %d message%s of the %s mailbox\n",
                 num_messages, add_s(num_messages), format_filesize(mbxsize, false));
      if (SHOW_ALL_TRANSLATIONS) show_stats(); }
   else {
      show_stats();
      if (mailbox_changes > 0) {
         log_printf("In the %s used by the mailbox, here's a map of the areas we changed:\n",
                    format_filesize(mbxsize, true));
         log_printf("%s\n", map); }
      // update the TOC file's timestamp so Eudora won't rebuild it
      update_filetime(tocpath); }
   log_printf("done in %.2f seconds\n", float(clock() - start_time) / CLOCKS_PER_SEC);
   if (logging && logfile) fclose(logfile);
   return total_changes == 0 ? 1 : 0; // normal end, but let the batch file know if changes were made
}

//*