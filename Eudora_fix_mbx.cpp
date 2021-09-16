//file: Eudora_fix_mbx.cpp
/*-----------------------------------------------------------------------------------------------------

Eudora_fix_mbx: Repair UTF-8 character codes and problematic HTML in Eudora mailboxes

This is a command-line (non-GUI) Windows program that can modify the data of a
Eudora mailbox in ways like this:

  - change UTF-8 characters that aren't rendered correctly to related ASCII character(s)
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

The searchstring is an arbitrary sequence of:
        "quotedstring"
        'quotedstring'
         hexadecimal string
A single-character item can optionally preceeded by ! to mean "not this character".

The replacementstring is an arbitrary sequence of:
        "quotedstring"
        'quotedstring'
         hexadecimal string
         *
The * means "use one character from the source", and the corresponding
search character needs to be one that was preceeded by !.

The rules are:
  - The searchstring may be 1 to 50 bytes long
  - The replacement may not be longer than the string searched for.
  - If the replacement is shorter than the search string, the remaining bytes are
    changed to zero in the mailbox, which are ignored when Eudora renders the text.
  - The components may be separated by one or more spaces.
  - A ; starts a comment

  Here are some example translations.txt lines:
      E28093 = "-"  ;  En dash
      E28094 = "--" ;  Em dash
      E2809C = '"'  ;  left double quote
      E2808B = ""   ;  zero-width space
      !0D 0A !0D = * 0D *  ; replace isolated linefeeds with carriage returns
      "<o:p>" = "<p>"      ; replace Outlook namespace tag with paragraph tag
  See the accompanying translations.txt file for a suggested starting file.

The program is invoked with a single argument, which is the
base filename of both the mailbox and table-of-contents files:

      Eudora_fix_mbx In

If the argument ends with ".mbx", it is removed. That allows the program to be run
by dragging and dropping the mailbox file onto the program's icon. The downside of
doing that is that the status report at the end will disaappear before you can read it.
But if there is an error, the program will pause until you acknowledge it.

The translations.txt file is expected to be in the current directory.

I don't guarantee this will work well for you, so keep a backup
of the mailbox file in case you don't like what it did!

Len Shustek, September 2021
------------------------------------------------------------------------------------------------------*/
/*----- Change log -------

12 Sep 2021, L. Shustek, V0.1  First version, written in the C subset of C++. I hate C++.
14 Sep 2021, L. Shustek, V0.2  Elaborate to allow NOT searches, string search specifiers,
                               and "same as source" replacements. The syntax of the
                               translation.txt file has changed.
16 Sep 2021, L. Shustek, V0.3  Show a map of places changed. Minor cosmetic fixes.
                               Fix serious bug in ".mbx" removal code.

Future ideas:
 - pause after the status report so that it can be seen when we're run by drag-and-drop onto
   the icon, but provide a command-line switch to disable it (and the error pauses?) in case
   we're run from a batch file.
 - a command-line switch for "quiet mode", where only errors are shown
 - a command line switch to specify the translations.txt file location
 - read the TOC file and do replacements in the subject field
 -
*/
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

#define VERSION "0.3"
#define DEBUG false

#include "stdafx.h"
#include <stdio.h>
#include <stdio.h>
#include <stdlib.h>
#include <Share.h>
#include <Windows.h>
#include <stdarg.h>

#define max(x, y) (((x) > (y)) ? (x) : (y))
#define min(x, y) (((x) < (y)) ? (x) : (y))
typedef unsigned char byte;

#define BLKSIZE 4096          // how much we read or write at a time, for efficiency
#define MAX_STR 50            // max size of search and replacement strings
#define MAX_TRANSLATIONS 250
#define MAPWIDTH 72
#define MAXNAME 80

/* some assumptions:
  - the mailbox is at most 2GB, so 32-bit signed integers are enough
  - fpos_t is actually just a 64-bit byte offset value
  - int is 32 bits
  */
// The buffer is divided into two halves so that we can match and replace over the buffer
// boundary. When we start looking at the second half, we slide the second half to the
// first half, and read the next data from the file into the second half.
byte bufferin[2 * BLKSIZE];
byte bufferout[2 * BLKSIZE];
// We need two buffers so that changes don't mess up later searches. Consider this case:
//    !0D 0A !0D -> * 0D *
// If we have only one buffer, the second 0A in the sequence xx 0A 0A xx won't be matched
// after the first 0A gets turned into 0D. So we use another buffer to retain the original data.
int bufferlen=0;        // how much is in the buffer
int bufferpos=0;        // where we're looking in the buffer, 0..BLKSIZE-1
int filesize;
fpos_t filepos1st;      // the file position of the first half of the buffer's data
fpos_t filepos2nd;      // the file position of the second half of the buffer's data
fpos_t fileposnext;     // the next file position to read
bool dirty1st=false;    // do we need to write the first half of the buffer?
bool dirty2nd=false;    // do we need to write the second half of the buffer?
int total_changes = 0;
char map[MAPWIDTH + 1];
FILE *fid;

struct string_t {   // a searchstring or replacementstring
   byte str[MAX_STR];      // the characters
   char flag[MAX_STR];     // flags
   int len; };             // how long
#define FL_NOT 1           // flag for "not this char" in search
#define FL_ORIG 2          // flag for "use original" byte in replacement

struct translation_t { // table of translations
   struct string_t srch;     // the search string
   struct string_t repl;     // the replacement string
   int used;                 // how often this translation was used
} translation[MAX_TRANSLATIONS] = { 0 };
int num_translations = 0;

void assert(bool test, const char *msg, ...) {
   if (!test) {
      va_list args;
      printf("ERROR: ");
      va_start(args, msg);
      vprintf(msg, args);
      va_end(args);
      printf("\n");
      system("pause"); // wait until the error is seen and acknowledged
      exit(8); } }

void show_stats(void) {
   printf("%d changes were made:\n", total_changes);
   for (int ndx = 0; ndx < num_translations; ++ndx) {
      int nchars = 0;
      printf("  ");
      for (int byt = 0; byt < translation[ndx].srch.len; ++byt) {
         byte ch = translation[ndx].srch.str[byt];
         if (translation[ndx].srch.flag[byt] == FL_NOT) nchars += printf("!");
         nchars += printf(isprint(ch) ? "%c" : "%02X ", ch ); }
      while (nchars < 12) nchars += printf(" ");
      printf(" changed to  ");
      nchars = 0;
      if (translation[ndx].repl.len == 0) nchars += printf("(nothing)");
      else for (int byt = 0; byt < translation[ndx].repl.len; ++byt) {
            char ch = translation[ndx].repl.str[byt];
            nchars += printf(ch == ' ' ? "(blank)" : isprint(ch) ? "%c" : "%02X ", ch);
            if (ch == '*') nchars += printf(" "); }
      while (nchars < 15) nchars += printf(" ");
      printf(" %d time%c\n", translation[ndx].used, translation[ndx].used == 1 ? ' ' : 's'); } }

void chk_buffer(bool forceend) { // keep the pointer in the first half of the buffer
   if (forceend || bufferpos >= BLKSIZE+1) { //+1 is so we always have one previous byte to check
      if (dirty1st) {
         if (DEBUG) printf("writing first %d bytes of buffer to file position %lld\n", min(BLKSIZE, bufferlen), filepos1st);
         fsetpos(fid, &filepos1st);// write out first half if it was changed
         fwrite(bufferout, 1, min(BLKSIZE, bufferlen), fid); }
      memcpy(bufferin, bufferin + BLKSIZE, BLKSIZE);  // slide second half to first half
      memcpy(bufferout, bufferout + BLKSIZE, BLKSIZE);
      filepos1st = filepos2nd;
      filepos2nd = fileposnext;
      fsetpos(fid, &fileposnext);
      int nbytes = (int) fread(bufferin + BLKSIZE, 1, BLKSIZE, fid); // read (up to) another half
      memcpy(bufferout + BLKSIZE, bufferin + BLKSIZE, BLKSIZE);
      fgetpos(fid, &fileposnext);
      if (DEBUG) printf("buffer starts at file pos %lld, read %d bytes at file pos %lld, next file pos %lld\n", filepos1st, nbytes, filepos2nd, fileposnext);
      dirty1st = dirty2nd;
      dirty2nd = false;
      bufferlen = bufferlen - BLKSIZE + nbytes;
      bufferpos -= BLKSIZE; } }

void parse_string(char **p, byte *dst, int *dstndx) {  // parse "xxx", 'yyy', or xxxx
   char delim = **p;
   if (delim == '"' || delim == '\'') { // accumulate a character string
      ++*p;
      while (**p != delim) { // for each character
         assert(*dstndx < MAX_STR, "string too long");
         dst[(*dstndx)++] = *(*p)++;
         assert(**p, "unterminated string"); }
      ++*p; }
   else { // accmulate a hex string
      bool firstnibble = true;
      while(1) { // for each nibble
         char ch = *(*p)++;
         if (ch == ' ' || ch == '\n' || ch == ';') break;
         if (ch >= '0' && ch <= '9') ch -= '0';
         else if (ch >= 'A' && ch <= 'F') ch -= 'A' - 10;
         else if (ch >= 'a' && ch <= 'f') ch -= 'a' - 10;
         else assert(false, "bad hex at %s", *p);
         assert(*dstndx < MAX_STR, "string too long");
         if (firstnibble) dst[*dstndx] = ch;
         else {
            dst[*dstndx] = (dst[*dstndx] << 4) + ch;
            ++*dstndx; }
         firstnibble = !firstnibble; }
      assert(firstnibble, "odd number of hex chars"); }
   while (**p == ' ')++*p; }

void read_translations(void) { // parse the translations file
#define TRANSLATION_FILE "translations.txt"
#define LINESIZE 80
   char line[LINESIZE];
   printf("reading " TRANSLATION_FILE "...\n");
   assert(fid = fopen(TRANSLATION_FILE, "r"), "Can't open " TRANSLATION_FILE);
   while (fgets(line, LINESIZE, fid)) {
      printf("%s", line);
      if (line[strlen(line) - 1] != '\n') printf("\n");
      char *ptr = line;
      while (*ptr == ' ') ++ptr;
      if (*ptr == 0 || *ptr == '\n' || *ptr == ';') continue; // skip blank or comment line
      struct translation_t *tp = &translation[num_translations];
      while( *ptr != '=') { // all parts of the search string
         if (*ptr == '!') {
            tp->srch.flag[tp->srch.len] = FL_NOT;
            while (*++ptr == ' '); }
         parse_string(&ptr, tp->srch.str, &tp->srch.len); }
      while (*++ptr == ' ') ;
      while (*ptr && *ptr != ';' && *ptr != '\n') { // all parts of the replacement string
         if (*ptr == '*') {
            assert(tp->srch.flag[tp->repl.len] == FL_NOT, "* in replacement not matched by ! in search string");
            tp->repl.flag[tp->repl.len] = FL_ORIG;
            tp->repl.str[tp->repl.len++] = '*';
            while (*++ptr == ' '); }
         else parse_string(&ptr, tp->repl.str, &tp->repl.len); }
      assert(tp->repl.len <= tp->srch.len, "replacement longer than search string");
      assert(++num_translations < MAX_TRANSLATIONS, "too many translations"); }
   fclose(fid);
   printf("processed and stored %d translations\n\n", num_translations); }

void show_help(void) {
   printf("\nRepair UTF-8 character codes and problematic HTML in Eudora mailboxes.\n");
   printf("It reads translation.txt, which has lines like:\n");
   printf("   E28098 = \"'\"         ; left single quote\n");
   printf("   C2A1 =  '\"'          ; double quote\n");
   printf("   '<o:p>' = '<p>'      ; repair Outlook HTML\n");
   printf("   !0D 0A !0D = * 0D *  ; change isolated LF to CR \n");
   printf("invoke as: Eudora_fix_mbx filename\n");
   printf("The mailbox filename.mbx is changed in place, so keep a backup!\n");
   printf("It also updates the timestamp of filename.toc so Eudora doesn't rebuild it.\n");
   printf("See the README file for more information.\n");
   exit(8); }

bool eudora_running(void) {
#define LINESIZE 80
   char buf[LINESIZE];
#define EUDORA_NAME "Eudora.exe"
   snprintf(buf, LINESIZE, "tasklist /FI \"IMAGENAME eq " EUDORA_NAME"\"");
   if (DEBUG)printf("%s\n", buf);
   FILE *fp = _popen(buf, "r");
   assert(fp, "can't start tasklist\n");
   bool running = false;
   while (fgets(buf, LINESIZE, fp)) {
      if (DEBUG) printf("tasklist: %s", buf);
      // see if any line starts with the Eudora executable file name
      if (strncmp(buf, EUDORA_NAME, sizeof(EUDORA_NAME) - 1) == 0)
         running = true; }
   assert(_pclose(fp) == 0, "can't find 'tasklist' command; check path\n");
   return running; }

void update_filetime(const char *filepath) {
#if 0  // this technique doesn't work if a path was specified and we're not running in the same directory
   char stringbuf[MAXNAME];
   snprintf(stringbuf, MAXNAME, "copy /b %s+,,", filepath);
   if (DEBUG) printf("%s\n", stringbuf);
   int rc = system(stringbuf); // update timestamp of the corresponding TOC file
   printf("%s timestamp %s\n", argv[1], rc == 0 ? "updated" : "could not be updated");
#endif
   // this technique seems to work in more cases
   SYSTEMTIME thesystemtime;
   GetSystemTime(&thesystemtime); // get time now
   FILETIME thefiletime;
   SystemTimeToFileTime(&thesystemtime, &thefiletime); // convert to file time format
   printf("updating timestamp of %s\n", filepath);
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

int main(int argc, char **argv) {
   printf("Eudora_fix_mbx  version %s  (c) L. Shustek, 2021\n", VERSION);
   if (argc <= 1) show_help();
   assert(!eudora_running(), "Eudora is running; stop it first");
   read_translations();
   //show_stats();

   char basefile[MAXNAME], stringbuf[MAXNAME];
   strncpy(basefile, argv[1], MAXNAME);
   // remove .mbx extension to allow drag-and-drop of mailbox file onto the icon
   int baselength = strlen(basefile);
   if (baselength > 4 &&
         (strcmp(basefile + baselength -4, ".mbx") == 0 ||
          strcmp(basefile + baselength - 4, ".MBX") == 0)) {
      basefile[baselength - 4] = 0; }

   snprintf(stringbuf, MAXNAME, "%s.mbx", basefile);
   printf("reading mailbox %s...\n", stringbuf);
   assert(fid = fopen(stringbuf, "rb+"), "can't open file %s", stringbuf);

   // initialize the map
   assert(fseek(fid, 0, SEEK_END) == 0, "can't seek to end of file, errno=%d", errno);
   filesize = ftell(fid); // get the size of the file
   assert(fseek(fid, 0, SEEK_SET) == 0, "can't seek to start of file");
   memset(map, '_', MAPWIDTH);
   map[MAPWIDTH] = 0;

   // initialize the buffer
   fgetpos(fid, &filepos1st);
   bufferlen = (int) fread(bufferin, 1, BLKSIZE, fid); // prime both halves of the buffer
   memcpy(bufferout, bufferin, BLKSIZE);
   fgetpos(fid, &filepos2nd);
   bufferlen += (int) fread(bufferin + BLKSIZE, 1, BLKSIZE, fid);
   memcpy(bufferout + BLKSIZE, bufferin + BLKSIZE, BLKSIZE);
   fgetpos(fid, &fileposnext);
   if (DEBUG) printf("buffer primed with %d bytes, filepos1st %lld, filepos2nd %lld\n", bufferlen, filepos1st, filepos2nd);

   do { // read and search the mailbox for changes to be made
      chk_buffer(false);
      for (int xlate = 0; xlate < num_translations; ++xlate) { // try each translation at this position
         struct translation_t *t = &translation[xlate];
         int ndx;
         for (ndx = 0; ndx < t->srch.len; ++ndx) { // check each search character
            if (t->srch.flag[ndx] == FL_NOT)  {
               if (bufferin[bufferpos + ndx] == t->srch.str[ndx]) break; }
            else if (bufferin[bufferpos + ndx] != t->srch.str[ndx]) break; }
         if (ndx == t->srch.len) { // all chars matched: do replacement
            // first: show in the map that we modified somethng in this area
            int mapndx = (int) (((filepos1st + bufferpos) * MAPWIDTH) / filesize); //64-bit arithmetic!
            assert(mapndx >= 0 && mapndx < MAPWIDTH, //TEMP
                   "mapndx = %d, filepos1st = %lld, bufferpos = %ld, filesize = %lld",
                   mapndx, filepos1st, bufferpos, filesize);
            if (mapndx < 0) mapndx = 0; if (mapndx >= MAPWIDTH) mapndx = MAPWIDTH-1;
            map[mapndx] = '*';
            for (ndx = 0; ndx < t->repl.len; ++ndx) { // for each replacement char
               if (t->repl.flag[ndx] != FL_ORIG) // original, or a replacement from a previous match!
                  bufferout[bufferpos + ndx] = t->repl.str[ndx]; }
            if (bufferpos < BLKSIZE) dirty1st = true; else dirty2nd = true;
            for (int zerocount = 0; zerocount < t->srch.len - t->repl.len; ++zerocount)
               bufferout[bufferpos + t->repl.len + zerocount] = 0; // pad with zeros
            if (bufferpos + t->srch.len >= BLKSIZE) dirty2nd = true;
            ++t->used;
            ++total_changes; } } // continue looking for more matches even after doing a replacement
      ++bufferpos; } // next position in the mailbox
   while (bufferpos < bufferlen);

   // cleanup
   while (bufferlen > 0) chk_buffer(true); // write out any modified parts of the last buffer
   fclose(fid);
   if (total_changes == 0) printf("no changes were made\n");
   else {
      show_stats();
      printf("in the %s used by the mailbox, here's a map of the areas we changed:\n",
             format_filesize(filesize));
      printf("%s\n", map);
      // update the TOC file's timestamp so Eudora won't rebuild it
      snprintf(stringbuf, MAXNAME, "%s.toc", basefile);
      update_filetime(stringbuf); }
   //system("pause"); //should we do this? might be awkward if we're run from a batch file
   return 0; }

