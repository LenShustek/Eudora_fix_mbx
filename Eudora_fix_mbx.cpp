//file: Eudora_fix_mbx.cpp
/*-----------------------------------------------------------------------------------------------------

Eudora_fix_mbx: Repair UTF-8 character codes and problematic HTML in Eudora mailboxes

This is a command-line (non-GUI) Windows program that modifies the data of Eudora
mailboxes -- and the to/from and subject fields of table-of-contents (TOC) files --
in ways like this:

  - change UTF-8 characters that aren't rendered correctly into related ASCII
    or extended ASCII (Windows-1252) characters
  - change linefeed characters not adjacent to carriage returns into carriage returns
    so Eudora will move to a new line and not squash everything together
  - change Outlook-generated non-standard HTML into something Eudora deals correctly with

When used on system mailboxes (In, Out, Junk, and Trash) this should only be run when
Eudora is not running, and a runtime check ensures that. For any other mailbox, we lock
the file so Eudora isn't accessing it while we are, and vice versa. That said, 
IT IS A GOOD IDEA TO NOT HAVE THE MAILBOX YOU ARE FIXING BE OPEN IN EUDORA.

All of the modifications are made without changing the size of the messages or of the files.
In addition, the table-of-contents file (.toc) file has its timestamp updated.
As a result, Eudora won't rebuild the table-of-contents file when it restarts.

The program learns the changes you want to make from a plain text file named
"translations.txt" that has the following kinds of lines:

Lines starting with "options" let you specify one or more keywords
that control the the operation of the program:
   skipheaders     don't do replacements in the header lines of messages
   skipbody        don't do replacements in the body of messages
   skiptoc         don't do replacements in the table-of-contents file
   noeudora        don't allow Eudora to be running even for non-system mailboxes
   eudoraokforsystemmailboxes  a hard-to-type option that lets Eudora run
                               even for system mailboxes, if you like risks

Others lines of the file specify replacements to make, in this form:
     searchstring = replacementstring  ;comment

 The searchstring can optionally start with clauses restricting the search:
     <headers>        only match in the headers of messages
     <body>           only match in the body of messages
     <html xxx>       only match in HTML <xxx...> tags
     <ifmatch n>      only match if "match flag n" is set; see <setmatch n>
     <ignorecase>     treat upper and lower case alphabetics as equivalent

 Following that is an arbitrary sequence of:
     "quotedstring"        any printable characters except "
     'quotedstring'        any printable characters except '
      hexadecimal string   hexadecimal character codes

Any single-character item can optionally preceeded by ! to mean "not this character".

The replacementstring is an arbitrary sequence of:
        "quotedstring"          any printable characters except "
        'quotedstring'          any printable characters except '
         hexadecimal string     hexadecimal character codes
         *                      use the character that matched !xx in searchstring
         <blanks>               replace the searchstring with all blanks
         <setmatch n>           set "match flag n"
         <clearmatch n>         clear "match flag n"

The rules are:
  - The searchstring may be 1 to 50 bytes long
  - The replacementstring may not be longer than the string searched for.
  - If the replacement is shorter than the search string, then
    - In a mailbox, the remaining bytes are changed to zero,
      which are ignored when Eudora renders the text.
    - In the TOC, the following bytes in the field are shifted left; zeroes are inserted at the end.
      (We can't change remaining bytes to zeroes, because Eudora will stop displaying at a zero byte.)
  - The match flags, numbered from 0 to 31, remember previous matches. They are all independent.
  - Multiple matchflags in a single searchstring must all be set for the match to trigger.
  - A * in the replacementstring must match a searchstring character that was preceeded by !.
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
  But if there is an error, the program will pause until you acknowledge it.

The translations.txt file is expected to be in the current directory.

See instructions.txt for one way to install and run the program.

I don't guarantee this will work well for you, so keep a backup of the mailbox
and TOC files in case you don't like what it did!

Len Shustek, September 2021
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

Ideas for future versions:
 - Pause after the status report so that it can be seen when we're run by drag-and-drop onto
     the icon? Provide a command-line switch to disable it (and the error pauses?) in case
     we're run from a batch file?  Maybe: /pausereport /pauseerror /nopausereport /nopauseerror
 - A command-line switch for "quiet mode", where only errors are shown? Maybe: /quiet
 - A command line switch to specify the translations.txt file location? Maybe: /xlate=filepathname
 - A mode where we make a modified copy of the mailbox (and TOC?) file, instead of updating in place?
     But if we change the name, we need to change it in the TOC.

-------------------------------------------------------------------------------------------------------*/
/* Copyright(c) 2021, Len Shustek
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

#define VERSION "0.7"
#define DEBUG false
#define SHOW_ALL_TRANSLATIONS false // always show final report, with all translations
#define DO_LOCKING true  // do file locking (and read/write) with fileapi.h calls, not fopen/fread/fwrite/fclose

/* I compile using Microsoft Visual Studio Community 2017, with the following project properties:
 Disable precompiled headers: in Project/Properties/Configuration Properties/C++/Command Line,
    in the "Additional Options" box at the bottom, add /Y-.
    Check on the top to see that you are making the change for "All Configurations".
 Don't check for deprecated usage: in Project/Properties/Configuration Properties/C++/Preprocessor,
    click on Preprocessor Definitions, use the downarrow on the right to select Edit,
    and add a line in the top box saying _CRT_SECURE_NO_WARNINGS.
    Again, check that you're doing it for All Configurations.
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <Share.h>
#include <Windows.h>
#include <stdarg.h>
#include <time.h>
#if DO_LOCKING
#include <fileapi.h>
#endif
#include "Eudora_TOC.h"

#define max(x, y) (((x) > (y)) ? (x) : (y))
#define min(x, y) (((x) < (y)) ? (x) : (y))
typedef unsigned char byte;

#define BLKSIZE 4096          // how much we read or write at a time, for efficiency
#define MAX_STR 50            // max size of search and replacement strings
#define MAX_TRANSLATIONS 500
#define MAXFILENAME 200
#define LINESIZE 200
#define MAPWIDTH 72
#define MAX_MATCHFLAG 31      // match flags are numbered from 0 to this

/* some assumptions:
  - the mailbox is at most 2GB, so 32-bit signed or unsigned integers are enough
  - fpos_t is actually just a 64-bit byte offset value
  - int is 32 bits
  */
// The buffer is divided into two halves so that we can match and replace over the buffer
// boundary. When we start looking in the second half, we slide the second half to the
// first half, and read the next data from the file into the second half.
// This works the same way for TOC files as for mailbox files.
byte bufferin[2 * BLKSIZE];
byte bufferout[2 * BLKSIZE];
// We need two buffers so that changes don't mess up later searches. Consider this case:
//    !0D 0A !0D -> * 0D *
// If we have only one buffer, the second 0A in the sequence xx 0A 0A xx won't be matched
// after the first 0A gets turned into 0D. So we use another buffer to retain the original data.
int bufferlen=0;              // how much is in the buffer
int bufferpos=0;              // where we're looking in the buffer, 0..BLKSIZE-1
fpos_t filepos1st;            // the file position of the first half of the buffer's data
fpos_t filepos2nd;            // the file position of the second half of the buffer's data
fpos_t fileposnext;           // the next file position to read
bool dirty1st=false;          // do we need to write the first half of the buffer?
bool dirty2nd=false;          // do we need to write the second half of the buffer?
unsigned long mbxsize;        // total size of the mailbox file
unsigned long matchflags = 0; // bitmap of which matchflags are currently set
int total_changes = 0;
int mailbox_changes;
char map[MAPWIDTH + 1];       // '_' for untouched sections, '*' for modified sections
bool neednewline = false;     //we've got something (like progress dots) being displayed
int last_megabyte;            // how many megabytes of the file we've read
bool skipheaders = false;     // option flag
bool skipbody = false;        //
bool skiptoc = false;         //
bool noeudora = false;        //
bool okeudoraalways = false;  //

struct string_t {   // a searchstring or a replacementstring
   byte str[MAX_STR];         // the characters
   byte flag[MAX_STR];        // character flags
   int len; };               // how long the string is
#define FL_NOT 1              // flag for "not this char" in search
#define FL_ORIG 2             // flag for "use original byte" in replacement

struct translation_t { // table of translations
   struct string_t srch;      // the search string
   struct string_t repl;      // the replacement string
   char htmltag[MAX_STR];     // optional HTML tag name to match, like "<meta"
   byte options;              // option bits, OPT_xxx
   byte status;               // status bits, STAT_xxx
   unsigned long testflags;   // bitmap of which match flags should be tested on search
   unsigned long setflags;    // bitmap of which match flags should be set on replace
   unsigned long clrflags;    // bitmap of which match flags should be cleared on replace
   int usecount;              // how often this translation was used
} translation[MAX_TRANSLATIONS] = { 0 };
#define OPT_HEADERS 0x01      // option for "only search in headers"
#define OPT_BODY 0x02         // option for "only search in body"
#define OPT_IGNORECASE 0x4    // option for "ignore case in search"
#define OPT_HTMLTAGMATCH 0x08 // option for "only search in HTML matching 'htmltag'"
#define OPT_REPLBLANKS 0x10   // option for "replace with blanks"
#define STAT_TAGMATCH 0x01    // status: "inside matching HTML tag"
int num_translations = 0;     // number of translations parsed and stored
//
//    display routines
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
   printf("For more information see https://github.com/LenShustek/Eudora_fix_mbx\n");
   exit(8); }

#define MAX_ERRLINE 200
char errline[MAX_ERRLINE];
void assert(bool test, const char *msg, ...) { // check for fatal error
   if (!test) {
      va_list args;
      if (neednewline) printf("\n");
      printf("ERROR: ");
      va_start(args, msg);
      vprintf(msg, args);
      va_end(args);
      printf("\n");
      system("pause"); // wait until the error is seen and acknowledged
      exit(8); } }
//
// I/O routines: either old-style fopen/fread/fwrite/fclose, which don't allow locking,
// or the new fileapi.h routines like CreateFile/Lockfile/ReadFile/WriteFile, which do.
//
enum fileid {MBX, TOC };
#if DO_LOCKING // new-style fileapi.h routines
HANDLE mbx_handle = NULL, toc_handle = NULL;
void fileopen(fileid whichfile, char *path) {
   HANDLE *pHandle = whichfile == MBX ? &mbx_handle : &toc_handle;
   if ((*pHandle = CreateFile(path, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL))
         == INVALID_HANDLE_VALUE) {
      int nchars = snprintf(errline, MAX_ERRLINE, "can't open %s\n  ", path);
      FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(), 0, errline+nchars, MAX_ERRLINE-nchars-1, NULL);
      assert(false, errline); }
   assert(LockFile(*pHandle, 0, 0, MAXINT32, 0), "can't lock file %s", path); }
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
   assert(SetFilePointer(*pHandle, (unsigned long) position, 0, FILE_BEGIN) != INVALID_SET_FILE_POINTER,
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
   assert (fgetpos(whichfile == MBX ? mbx_fid : toc_fid, &position) == 0,
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

int show_matchflags(const char *name, unsigned long flags) {
   int nchars = 0;
   for (int flagnum = 0; flags; flags >>= 1, ++flagnum)
      if (flags & 1)
         nchars += printf("<%s %d> ", name, flagnum);
   return nchars; }

void show_stats(void) {
   int toc_changes = total_changes - mailbox_changes;
   printf("%d mailbox change%s and %d table-of-contents change%s were made:\n",
          mailbox_changes, mailbox_changes == 1 ? "" : "s",
          toc_changes, toc_changes == 1 ? "" : "s");
   for (int ndx = 0; ndx < num_translations; ++ndx) {
      struct translation_t *t = &translation[ndx];
      if (SHOW_ALL_TRANSLATIONS || t->usecount > 0) { // maybe only show translations actually used
         printf("  "); int nchars = 0;
         if (t->options & OPT_HEADERS) nchars += printf("<headers> ");
         if (t->options & OPT_BODY) nchars += printf("<body> ");
         if (t->options & OPT_IGNORECASE) nchars += printf("<ignorecase> ");
         if (t->options & OPT_HTMLTAGMATCH) nchars += printf("<html %s> ", t->htmltag+1);
         nchars += show_matchflags("ifmatch", t->testflags);
         for (int byt = 0; byt < t->srch.len; ++byt) {
            byte ch = t->srch.str[byt];
            if (t->srch.flag[byt] == FL_NOT) nchars += printf("!");
            nchars += printf(ch > 31 && ch < 127 ? "%c" : "%02X ", ch ); }
         while (nchars < 15) nchars += printf(" ");
         printf("  changed to  "); nchars = 0;
         if (t->repl.len == 0) nchars += printf("<nothing>");
         if (t->options & OPT_REPLBLANKS) nchars += printf("<blanks>");
         else for (int byt = 0; byt < t->repl.len; ++byt) {
               byte ch = t->repl.str[byt];
               nchars += printf(ch == ' ' ? "<blank>" : ch > 31 && ch < 127 ? "%c" : "%02X ", ch);
               if (ch == '*') nchars += printf(" "); }
         nchars += printf(" ");
         nchars += show_matchflags("setmatch", t->setflags);
         nchars += show_matchflags("clearmatch", t->clrflags);
         while (nchars < 15) nchars += printf(" ");
         printf(" %d time%c\n", t->usecount, t->usecount == 1 ? ' ' : 's'); } } }
//
//    buffer management
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
   last_megabyte = 3; // show progress dots after this many megabytes
   matchflags = 0;
   if (DEBUG) printf("\nbuffer primed with %d bytes, filepos1st %lld, filepos2nd %lld\n", bufferlen, filepos1st, filepos2nd); }

void chk_buffer(fileid whichfile, bool forceend) { // keep the pointer in the first half of the buffer
   if (forceend || bufferpos >= BLKSIZE+1) { //+1 is so we always have one previous byte to check
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
      if (new_megabyte > last_megabyte) {
         printf("."); // print a dot after every megabyte
         last_megabyte = new_megabyte; }
      int nbytes = fileread(whichfile, bufferin + BLKSIZE, BLKSIZE); // read (up to) another half
      memcpy(bufferout + BLKSIZE, bufferin + BLKSIZE, BLKSIZE);
      fileposnext = fileposition(whichfile);
      if (DEBUG) printf("buffer starts at file pos %lld, read %d bytes at file pos %lld, next file pos %lld\n", filepos1st, nbytes, filepos2nd, fileposnext);
      dirty1st = dirty2nd;
      dirty2nd = false;
      bufferlen = bufferlen - BLKSIZE + nbytes;
      bufferpos -= BLKSIZE; } }
//
// parsing routines for the translations.txt file
//
char line[LINESIZE];

char lowercase(char ch) {
   if (ch >= 'A' && ch <= 'Z') ch += 'a' - 'A';
   return ch; }

char uppercase(char ch) {
   if (ch >= 'a' && ch <= 'z') ch -= 'a' - 'A';
   return ch; }

bool skip_blanks(char **pptr) { // returns true at end-of-line
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

void scan_tagname(char **pptr, char *dst) {
   int strlen = 0;
   while (**pptr != '>') {
      assert(**pptr, "HTML tagname not terminated: %s\n%s", *pptr, line);
      *dst++ = *(*pptr)++;
      assert(++strlen < MAX_STR - 2, "HTML tagname too big: %s\n%s", *pptr, line); }
   *dst = 0;
   ++*pptr; skip_blanks(pptr); }

void parse_string(char **p, byte *dst, int *dstndx) {  // parse: "abc", 'abc', or xxxx in hex
   char delim = **p;
   if (delim == '"' || delim == '\'') { // accumulate a character string
      ++*p;
      while (**p != delim) { // for each character
         assert(*dstndx < MAX_STR, "string too long: %s\n%s", *p, line);
         dst[(*dstndx)++] = *(*p)++;
         assert(**p, "unterminated string: %s\n%s", *p, line); }
      ++*p; }
   else { // accmulate a hex string
      bool firstnibble = true;
      while(1) { // for each nibble
         char ch = *(*p)++;
         if (ch == ' ' || ch == '\n' || ch == ';') break;
         if (ch >= '0' && ch <= '9') ch -= '0';
         else if (ch >= 'A' && ch <= 'F') ch -= 'A' - 10;
         else if (ch >= 'a' && ch <= 'f') ch -= 'a' - 10;
         else assert(false, "bad hex: %s\n%s", *p-1, line);
         assert(*dstndx < MAX_STR, "string too long: %s\n%s", *p-1, line);
         if (firstnibble) dst[*dstndx] = ch;
         else {
            dst[*dstndx] = (dst[*dstndx] << 4) + ch;
            ++*dstndx; }
         firstnibble = !firstnibble; }
      assert(firstnibble, "odd number of hex chars: %s\n%s", *p-1, line); }
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
      if (scan_keyword(pptr, "skipheaders")) skipheaders = true;
      else if (scan_keyword(pptr, "skipbody")) skipbody = true;
      else if (scan_keyword(pptr, "skiptoc")) skiptoc = true;
      else if (scan_keyword(pptr, "noeudora")) noeudora = true;
      else if (scan_keyword(pptr, "eudoraokforsystemmailboxes")) okeudoraalways = true;
      else assert(false, "invalid option: %s\n%s", *pptr, line); }

void read_translations(void) { // read and parse all lines in the translations file
#define TRANSLATION_FILE "translations.txt"
   FILE *text_fid;
   printf("reading " TRANSLATION_FILE "...\n");
   assert(text_fid = fopen(TRANSLATION_FILE, "r"), "Can't open " TRANSLATION_FILE);
   while (fgets(line, LINESIZE, text_fid)) {
      //printf("%s", line);
      //if (line[strlen(line) - 1] != '\n') printf("\n");
      char *ptr = line;
      if (skip_blanks(&ptr)) continue; // skip blank or comment line
      if (scan_keyword(&ptr, "options")) parse_options(&ptr);
      else { // must be a translation specification
         struct translation_t *tp = &translation[num_translations];
         char *startptr;
         do {  // process search string modifiers in any order
            startptr = ptr;
            if (scan_keyword(&ptr, "<headers>")) tp->options |= OPT_HEADERS;
            if (scan_keyword(&ptr, "<body>")) tp->options |= OPT_BODY;
            if (scan_keyword(&ptr, "<ignorecase>")) tp->options |= OPT_IGNORECASE;
            if (scan_keyword(&ptr, "<html ")) {
               tp->htmltag[0] = '<';
               scan_tagname(&ptr, tp->htmltag + 1);
               tp->options |= OPT_HTMLTAGMATCH; }
            if (scan_keyword(&ptr, "<ifmatch ")) tp->testflags |= parse_flagnum(&ptr); }
         while (ptr > startptr);
         while (*ptr != '=') { // parse all parts of the search string
            if (*ptr == '!') {
               tp->srch.flag[tp->srch.len] = FL_NOT; // flag the next char as "not this char"
               skip_blanks(&++ptr); } // skip ! and following blanks
            parse_string(&ptr, tp->srch.str, &tp->srch.len); }
         for (int ndx = 0; ndx < num_translations; ++ndx)  // check for a duplicated search string
            assert(translation[ndx].srch.len != tp->srch.len
                   || memcmp(translation[ndx].srch.str, tp->srch.str, tp->srch.len) != 0
                   || memcmp(translation[ndx].srch.flag, tp->srch.flag, tp->srch.len) != 0,
                   // Should we also check options and match flag operations? Or allow those not-quite duplicates?
                   "duplicate search string: %s", line);
         ++ptr; // skip =
         while (!skip_blanks(&ptr)) { // parse all parts of the replacement string
            if (scan_keyword(&ptr, "*")) {
               assert(tp->srch.flag[tp->repl.len] == FL_NOT, "* in replacement not matched by ! in search string: %s", line);
               tp->repl.flag[tp->repl.len] = FL_ORIG;
               tp->repl.str[tp->repl.len++] = '*'; }
            else if (scan_keyword(&ptr, "<setmatch ")) tp->setflags |= parse_flagnum(&ptr);
            else if (scan_keyword(&ptr, "<clearmatch ")) tp->clrflags |= parse_flagnum(&ptr);
            else if (scan_keyword(&ptr, "<blanks>")) {
               tp->options |= OPT_REPLBLANKS;
               tp->repl.len = tp->srch.len; }
            else parse_string(&ptr, tp->repl.str, &tp->repl.len); }
         assert(tp->repl.len <= tp->srch.len, "replacement longer than search string: %s", line);
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

char *format_filesize(unsigned long val) {
   static char buf[20]; // WARNING: returns pointer to static string
   if (val < 1024) snprintf(buf, 20, "%ld bytes", val);
   else if (val < 1024*1024) snprintf(buf, 20, "%ld KB", (val + 512) >> 10);
   else snprintf(buf, 20, "%ld MB", (val + 512*1024) >> 20);
   return buf; }

bool compare_names(char *ptr, const char *keyword) {
   do if (tolower(*ptr++) != tolower(*keyword++)) return false;
   while (*keyword && *ptr);
   return *keyword == *ptr; }
//
//    search-and-replace routines
//
void check_translation(struct translation_t *t, int fieldlen) {
   // See if this one translation matches at the current position in the buffer,
   // and do the replacements if so.
   int ndx;
   for (ndx = 0; ndx < t->srch.len; ++ndx) { // check each search character
      bool match = t->options & OPT_IGNORECASE
                   ? (lowercase(bufferin[bufferpos + ndx]) == lowercase(t->srch.str[ndx]))
                   : (bufferin[bufferpos + ndx] == t->srch.str[ndx]);
      if (t->srch.flag[ndx] == FL_NOT ? match : !match) break; }
   if (ndx == t->srch.len) { // all chars matched: do replacement
      if (fieldlen == 0) { // no field length means we're doing the mailbox
         // show in the map that we modified something in this area
         int mapndx = (int)(((filepos1st + bufferpos) * MAPWIDTH) / mbxsize); //64-bit arithmetic!
         if (mapndx < 0) mapndx = 0; if (mapndx >= MAPWIDTH) mapndx = MAPWIDTH - 1;
         map[mapndx] = '*'; }
      for (ndx = 0; ndx < t->repl.len; ++ndx) { // for each replacement char
         if (t->repl.flag[ndx] != FL_ORIG) // FL_ORIG: original, or a replacement from a previous match!
            bufferout[bufferpos + ndx] = t->options & OPT_REPLBLANKS ? ' ' : t->repl.str[ndx]; }
      if (bufferpos < BLKSIZE) dirty1st = true; else dirty2nd = true; //mark the start as dirty
      int deficit = t->srch.len - t->repl.len; // replacement is this many bytes too small
      if (deficit > 0) { // what to do about the difference?
         if (fieldlen == 0) { // in mailbox: pad with zeros
            while (deficit-- > 0)
               bufferout[bufferpos + t->repl.len + deficit] = 0; }
         else { // in TOC: slide the remaining field to the left
            int dst = t->repl.len; // offset of our destination
            for (int cnt = fieldlen-t->srch.len; cnt > 0; --cnt, ++dst) // move chars left
               bufferout[bufferpos + dst] = bufferin[bufferpos + dst] = bufferin[bufferpos + dst + deficit];
            for (; dst < fieldlen; ++dst) // fill vacated spots at the right of the field with zeros
               bufferout[bufferpos + dst] = bufferin[bufferpos + dst] = 0; } }
      if (bufferpos + t->srch.len >= BLKSIZE) dirty2nd = true; // ended in the second half, so it is dirty
      matchflags |= t->setflags;  // set and clear global match flags as requested
      matchflags &= ~t->clrflags;
      ++t->usecount;
      ++total_changes; } }

void check_TOC_translations(byte *field, int size) {
   // check for translations at all positions in a field of the TOC
   bufferpos = field - bufferin;  // offset of the start of the message's field in the buffer
   assert(bufferlen - bufferpos >= size, "TOC file too small! " // not enough data left
          "need %d bufferlen %d bufferpos %d", size, bufferlen, bufferpos);
   while (size > 0) { // for each character in the field
      for (int xlate = 0; xlate < num_translations; ++xlate) { // try all translations at this position
         struct translation_t *t = &translation[xlate];
         if (size >= t->srch.len) // if there are enough characters left for this translation
            check_translation(t, size); } // maybe do replacements, sliding instead of zero-padding
      ++bufferpos; --size; } }
//
//    main routine
//
int main(int argc, char **argv) {
   clock_t start_time = clock();
   printf("Eudora_fix_mbx  version %s  (c) L. Shustek, 2021\n", VERSION);
   if (argc <= 1) show_help();
   read_translations();
   //show_stats();

   //******** open both files, maybe

   char basefile[MAXFILENAME], mbxpath[MAXFILENAME], tocpath[MAXFILENAME];
   strncpy(basefile, argv[1], MAXFILENAME); basefile[MAXFILENAME - 1] = 0;
   int baselength = strlen(basefile); // remove .mbx extension to allow drag-and-drop of mailbox file onto the icon
   if (baselength > 4 &&
         (strcmp(basefile + baselength -4, ".mbx") == 0 ||
          strcmp(basefile + baselength - 4, ".MBX") == 0)) {
      basefile[baselength - 4] = 0; }

   char *mbxname = basefile; // find pointer to mailbox name: after the last \, if any
   for (char *ptr = basefile; *ptr; ++ptr)
      if (*ptr == '\\') mbxname = ptr + 1;

   if (!okeudoraalways) { // check the rules for whether Eudora can be running
      if (noeudora) assert(!eudora_running(), "Eudora is running; stop it first");
      else if (compare_names(mbxname, "In") || compare_names(mbxname, "Out")
               || compare_names(mbxname, "Trash") || compare_names(mbxname, "Junk"))
         assert(!eudora_running(), "Fixing system mailbox, but Eudora is running; stop it first"); }

   snprintf(mbxpath, MAXFILENAME, "%s.mbx", basefile);
   fileopen(MBX, mbxpath);
   snprintf(tocpath, MAXFILENAME, "%s.toc", basefile);
   if (!skiptoc) fileopen(TOC, tocpath);
   
   //******* process messages in the mailbox file

   printf("reading mailbox %s...", mbxpath);
   neednewline = true; // note that we didn't print \n so we can keep adding dots
   mbxsize = (unsigned long) filesize(MBX);   // initialize the mailbox map
   memset(map, '_', MAPWIDTH);
   map[MAPWIDTH] = 0;
   init_buffer(MBX);   // initialize the buffer
   bool linestart = true, inheader = false;
   do { // read and search the mailbox for changes to be made
      chk_buffer(MBX, false);
      // keep track of whether we're in the headers or the body of a message
      if (linestart && memcmp(bufferin + bufferpos, "From ???@???", 12) == 0) inheader = true;
      if (inheader && memcmp(bufferin + bufferpos, "\r\n\r\n", 4) == 0) inheader = false;
      linestart = bufferin[bufferpos] == '\r' || bufferin[bufferpos] == '\n';
      if (!(skipheaders && inheader || skipbody && !inheader)) // check global header/body restrictions
         // try each translation at this position
         for (int xlate = 0; xlate < num_translations; ++xlate) {
            struct translation_t *t = &translation[xlate];
            // Check for entering an HTML tag we need to match. Unlike the "inheader" boolean, STAT_TAGMATCH
            // isn't a global because it only applies to specific translations.
            if (t->options & OPT_HTMLTAGMATCH && memcmp(bufferin + bufferpos, t->htmltag, strlen(t->htmltag)) == 0)
               t->status |= STAT_TAGMATCH; // entering HTML tag to match
            if (t->status & STAT_TAGMATCH && bufferin[bufferpos] == '>') // exiting HTML tag to match
               t->status &= ~STAT_TAGMATCH;
            //see if search modifiers are met
            if (t->options & OPT_HEADERS && !inheader) continue; // not in header: go to next translation
            if (t->options & OPT_BODY && inheader) continue;     // not in body: same
            if (t->options & OPT_HTMLTAGMATCH && !(t->status & STAT_TAGMATCH)) continue; // not in matching HTML tag: same
            if (t->testflags && (t->testflags & matchflags) != t->testflags) continue;   // not all specified test flags match: same
            check_translation(t, 0); // check for searchstring match and do replacement if so
         } // continue looking for more matches even after doing a replacement
      ++bufferpos; } // next position in the mailbox
   while (bufferpos < bufferlen);
   // cleanup
   while (bufferlen > 0) chk_buffer(MBX, true); // write out any modified parts of the last buffer
   fileclose(MBX);
   printf("\n"); neednewline = false; // end the progress dots
   mailbox_changes = total_changes;

   //********* process the subject and address fields in the table of contents file

   if (!skiptoc) { // unless "options skiptoc"
      printf("reading table-of-contents %s...", tocpath);
      neednewline = true; // note that we didn't print \n so we can keep adding dots
      assert(fileread(TOC, bufferin, sizeof(struct Eudora_TOC_header_t)) == sizeof(struct Eudora_TOC_header_t),
             "can't read TOC header");
      struct Eudora_TOC_header_t *hdr = (struct Eudora_TOC_header_t *) bufferin;
      int nummsgs = hdr->SumCount;
      //printf("%d messages", hdr->SumCount);
      init_buffer(TOC);   // initialize the buffer with TOC message descriptors
      while (nummsgs-- > 0) { // for each message
         chk_buffer(TOC, false); // might change bufferpos!
         struct Eudora_TOC_message_t *msg = // ptr to message descriptor
            (struct Eudora_TOC_message_t *) (bufferin + bufferpos);
         //printf("msgcnt %d at bufferpos %d, bufferlen %d, datetime %s\n",
         //       nummsgs, bufferpos, bufferlen, msg->date_time);
         check_TOC_translations(  // check for translations in the sender or recipient namelist
            (byte*)&(msg->sender_recipient), sizeof(msg->sender_recipient));
         check_TOC_translations(  // check for translations in the subject field
            (byte*)&(msg->subject), sizeof(msg->subject));
         ++msg; // point to next message descriptor in the buffer
         bufferpos = (byte *)msg - bufferin; // move the buffer pointer there
      } // move to next message descriptor
      // cleanup
      while (bufferlen > 0) chk_buffer(TOC, true); // write out any modified parts of the last buffer
      fileclose(TOC);
      printf("\n"); neednewline = false; } // end the progress dots
//
//      final reporting
//
   if (total_changes == 0) {
      printf("no changes were made\n");
      if (SHOW_ALL_TRANSLATIONS) show_stats(); }
   else {
      show_stats();
      if (mailbox_changes > 0) {
         printf("In the %s used by the mailbox, here's a map of the areas we changed:\n",
                format_filesize(mbxsize));
         printf("%s\n", map); }
      // update the TOC file's timestamp so Eudora won't rebuild it
      update_filetime(tocpath); }
   printf("done in %.2f seconds\n", float(clock() - start_time) / CLOCKS_PER_SEC);
   //system("pause"); //should we do this? might be annoying if we're run from a batch file
   return 0; }

//*