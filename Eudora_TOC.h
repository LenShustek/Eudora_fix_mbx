//file: Eudora_TOC.h
// much of what I learned came from http://users.starpower.net/ksimler/eudora/toc.html
// some of it came from the source file tocdoc.cpp
 
#include <stdint.h>
//Note that integers are stored in little-endian order

struct Eudora_TOC_header_t { // total of 104 (0x68) bytes
   char version[2];               //00  usually "1", zero-terminated
   char UsedVersion[6];           //02  bitmap of previous used versions?
   char mbx_name[32];             //08  mailbox name without extension
   int16_t mbx_type;              //28  0=in, 1=out, 2=junk, 3=trash, 4=regular, 5=folder
   int16_t flags;                 //2A  0x01=group by subject, 0x02=needs sorting, 0x04=show file browser
                                  //    0x18= file browser state, 0x20=hide deleted iMAP messages
   int16_t NeedsCompact;          //2C  mailbox needs compacting?
   int16_t windows_position[4];   //2E  top left x,y; bottom right x,y; in pixels
   int16_t column_widths[8];      //36  saved width of various columns
   int16_t Uread_Status;          //46  0=unknown, 1=yes, 2=no
   int32_t NextUniqueMessageId;   //48  
   int32_t PluginID;              //4C  
   int32_t PluginTag;             //50  
   int16_t SplitterPos;           //54  
   char saved_sort_columns[9];    //56  width of the columns in the mailbox window
   char AdFailure;                //5F
   int32_t OldMbxSize;            //60  +1, actually. tocdoc.cpp "so that we can see if the .mbx file has been changed behind our back. 
   int16_t UnusedDiskSpace;       //64  not used?
   uint16_t SumCount;             //66  number of messages (low 2 bytes)
};

#pragma pack(push)
#pragma pack(1)
struct Eudora_TOC_message_t { // total of 218 (0xda) bytes
   int32_t offset;               // offset of the message in the .mbx file
   int32_t length;               // size of the message in the .mbx file
   int32_t GMT_date_time;        // seconds since 1/1/1970
   int16_t status;               //0=unread, 1=read, 2=replied, 3=forwarded, 4=redirected
                                 //5=TOC rebuilt, 6=saved, 7=queued, 8=sent, 9=unsent, 10=time queued
   char flags1;                  //x80=alt sig, 0x40=sig, 0x20=wordwrap, 0x10=tabs, 0x08=keepcopy, 0x04=returnreceipt
   char flags2;                  //0x80=MIME, 0x40=UUcode, 0x01=attachment present
   int16_t priority;             // 1 (high) to 3 (normal) to 5 (low)
   char date_time[32];           // display version
   char sender_recipient[64];    // sender, except receipient for outbox
   char subject[64];             // display version
   int16_t window_position[4];   // top left x,y; bottom right x,y; in pixels
   int16_t mystery1;
   int32_t mystery2;
   char mystery3[26];
};
#pragma pack(pop)
