# agon-utilities
MOS utilities for Agon: copy, view, editor...

These are small utilities that everybody has taken for granted on UNIX, but even on MS-DOS for a long time. But Agon did not have them until now.
The project will include an editor some day. All are MOS commands, all are written in Assembly language and all run in 24-bit ADL mode.
Parts of the code are based on the repository https://github.com/breakintoprogram/agon-projects

The projects are built with Zilog ZDS-II and binaries are provided, that you can put in the `/mos` directory
of the SD-card.

Note: the copy utility has been removed because it is now an internal command in MOS.

### comp

Once you copied a file, you want to check whether the copied file is an exact copy. The comp utility compares two files. It tells whether they are the same,
at which offset the first difference occurs or whether one file is larger than the other but they are the same up to the length of the smaller file.

Example command line: `comp file1.bin file2.bin`

### more

The well-known utility from early Unix and also MS-DOS. MOS does not support pipelines, so you cannot pipe the output of the CAT command to it. It can just show
a single file and pauses after a screenful has been shown. It reads the screen width and height, so it works well in all three screen modes.

Example command line: `more file.txt`

### memfill

Can be useful to clean the contents of RAM, so you see immediately which memory areas have been touched by your program. memdump is already there in a different respository.

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

### nano

This is an editor with nano-style key bindings.

Example command: `nano myfile.txt`

The command can take a second parameter to specify the buffer, so larger files can be loaded.
Example command when specifying a buffer. This works the same way as for the copy command.

Example command using a buffer:
`nano myfile.txt &90000`

It can take a third parameter to specify the line number on which the cursor must start. Example:
`nano myfile.txt &90000 231`

The edditor runs in MODE 0 (it selects this mode when started) and tries to return to the mode from which it was called.
Key bindings are nano-style, but not all are implemented (far from).

Implemented key bindings: Control-A, Control-E to go to the start or end of a line, Control-Y and Control-V for page-up and
page-down, COntrol-L to redraw the screen with the current line in the centre, Control-G to see a help screen, Control-X to exit, Control-O to save the file, 
Control-D for forward delete. Control-K to cut current line (repeat to cut block of lines), Control-U to paste
And of course the cursor keys, TAB, Backspace and Enter.

Different from nano (as nano uses impossible control and alt combinations for these functions): Control-C to copy current line,
Control-H to go to a specific line (enter a number).

Planned: Control-R for reading and inserting a file, Control-W and Control-Q
for search forward and backward. Also Control-T to insert special characters.

Possibly planned: Control-J for justifying paragraph, Control-\ for search+replace, Control-W for wordwrap functionality (non-nano).

 
