# agon-utilities
MOS utilities for Agon: copy, view, editor...

These are small utilities that everybody has taken for granted on UNIX, but even on MS-DOS for a long time. But Agon did not have them until now.
The project will include an editor some day. All are MOS commands, all are written in Assembly language and all run in 24-bit ADL mode.
Parts of the code are based on the repository https://github.com/breakintoprogram/agon-projects

The projects are built with Zilog ZDS-II and binaries are provided, that you can put in the `/mos` directory
of the SD-card.

### copy

The copy command can copy a single file. By default the command is limited to A file up to about 31kB, as it has to store the file instead the area
reserved for MOS commands. However, a third parameter can specify a buffer address and then the program can use a buffer at the specified address, extending
until the start of the MOS area. For example specifying &80000 will allow the area &80000-&AFFFF as a file buffer. The program has to store the entire
file in a single buffer due to limitations of the API calls in MOS v1.02.

Example command line: `copy file1.bin file2.bin`

Example command line specifying buffer: `copy file1.bin file2.bin &80000`

### comp

Once you copied a file, you want to check whether the copied file is an exact copy. The comp utility compares two files. It tells whether they are the same,
at which offset the first difference occurs or whether one file is larger than the other but they are the same up to the length of the smaller file.

Example command line: `comp file1.bin file2.bin`

### more

The well-known utility from early Unix and also MS-DOS. MOS does not support pipelines, so you cannot pipe the output of the CAT command to it. It can just show
a single file and pauses after s screenful has been shown. It reads the screen widt and height, so it works well in all three screen modes.

Example command line: `more file.txt`

### memfill

Can be useful to clean the contents of RAM, so you see immediatley which memory areas have been touched by your program. memdump is already there in a different respository.

Example command line: `memfill &80000 &10000 &ff`

### font

The command can load the character definitions from a file. The file is a 2048-byte binary file that contains the bitmaps for all characters from 0 to 255, 
although only chars 32..126 and 128..255 can be used.

Example command: `font /fonts/bbcasc-8.bin`

Example command that shows the font just loaded: 

`font /fonts/latin1-8.bin show`

Two fonts are included: 
- `bbcasc-8.bin` is the binary version of the font loaded on bootup by the vdp. It was derived from the file `agon_font.h`
 in the VDP project. It contains only ASCII characters with the pounds sign at position 0x60.
- `latin1-8.bin` is an ISO 8859-1 font (Latin1) that I derived from the following github project. The font provided had
 the bytes bit-reversed compared to what we need, so I changed it to the right format. It does not look pretty, but it's a start.
https://github.com/dhepper/font8x8

You can check out https://github.com/epto/epto-fonts
The `*.font` files included in the project contain meta-information, but they start with the bitmap data in a form that the font utility can use.
The 8x8 fonts from this set, you can just load with the font utility. It ignores the meta data. Some of them are only ASCII, some of them have code page 437 fonts.