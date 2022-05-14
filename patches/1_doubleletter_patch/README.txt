When Eudora receives a message that is identified as using the UTF-8 character 
set, it translates a limited set of multi-byte codes into single-byte ANSI 
(extended ASCII) characters before the message is stored in the Eudora mailbox. 
See the table below for the list. 

Unfortunately the code has a bug which causes unrelated characters at the end of 
the line to be repeated. In particular, if the translations change N bytes into 
M bytes, then the last N-M-1 characters of the line are repeated. 

The patched version of eudora.exe in this directory fixes the bug. For all the 
gory details, see doubled_letter_bugfix.txt. 

Len Shustek, 17 March 2022


These are the 12 2-byte and 15 3-byte codes that Eudora translates into ANSI:

 0xC5 0x92      becomes ANSI 0x8C 'Œ' Latin Capital Ligature OE
 0xC5 0x93      becomes ANSI 0x93 'œ' Latin Small Ligature OE
 0xC5 0xA0      becomes ANSI 0x8A 'Š' Latin Capital Letter S With Caron
 0xC5 0xA1      becomes ANSI 0x9A 'š' Latin Small Letter S With Caron
 0xC5 0xB8      becomes ANSI 0x9F 'Ÿ' Latin Capital Letter Y With Diaeresis
 0xC5 0xBD      becomes ANSI 0x8E 'Ž' Latin Capital Letter Z With Caron
 0xC5 0xBE      becomes ANSI 0x9E 'ž' Latin Small Letter Z With Caron
 0xC5 0xBF      becomes ANSI 0x83 'ƒ' Latin Small Letter Long S (F)
 0xCB 0x82      becomes ANSI 0x8B '‹' single left-pointing angle quotation mark
 0xCB 0x83      becomes ANSI 0x9B '›' single right-pointing angle quotation mark
 0xCB 0x86      becomes ANSI 0x88 'ˆ' Modifier Letter Circumflex Accent
 0xCB 0x9C      becomes ANSI 0x98 '˜' Small Tilde
 0xE2 0x80 0x93 becomes ANSI 0x96 '­' En Dash
 0xE2 0x80 0x94 becomes ANSI 0x97 '—' Em dash
 0xE2 0x80 0x9A becomes ANSI 0x82 '‚' Single Low Quotation Mark
 0xE2 0x80 0x9E becomes ANSI 0x84 '„' Double Low Quotation Mark
 0xE2 0x80 0xA0 becomes ANSI 0x86 '†' Dagger
 0xE2 0x80 0xA1 becomes ANSI 0x87 '‡' Double Dagger
 0xE2 0x80 0xA2 becomes ANSI 0x95 '•' Bullet
 0xE2 0x80 0xA6 becomes ANSI 0x85 '…' Horizontal Ellipsis
 0xE2 0x80 0xB0 becomes ANSI 0x89 '‰' Per Mille Sign
 0xE2 0x80 0xB2 becomes ANSI 0x92 '’' Right Single Quotation Mark
 0xE2 0x80 0xB3 becomes ANSI 0x94 '”' Right Double Quotation Mark
 0xE2 0x80 0xB5 becomes ANSI 0x91 '‘' Left Single Quotation Mark
 0xE2 0x80 0xB6 becomes ANSI 0x93 '“' Left Double Quotation Mark
 0xE2 0x82 0xAC becomes ANSI 0x80 '€' Euro Sign
 0xE2 0x84 0xA2 becomes ANSI 0x99 '™' Trade Mark Sign
         
 