# agon-utilities
MOS utilities for Agon: copy, view, editor...

These are small utilities that everybody has taken for granted on UNIX, but even on MS-DOS for a long time. But Agon did not have them until now.
The project will include an editor some day. All are MOS commands, all are written in Assembly language and all run in 24-bit ADL mode.
Parts of the code are based on the repository https://github.com/breakintoprogram/agon-projects

The projects are built with ez80asm
(https://github.com/AgonConsole8/agon-ez80asm) and agondev
(https://github.com/AgonConsole8/agondev). Under Linux, make sure to have a
driectory containing ez80asm and the binary directory of agondev in your
path and you can just run make to build the utilities.

All pre-assembled and precompiled binaries are also provided. The
`loadfont`, `recode` and `sort` programs, as well as everything
belonging to `mc`, must be in the `bin` directory and the other
binaries in the `mos` directory. Assumed is Agon Console8 MOS-2.23 or
higher and VDP-2.8.0 or higher.

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

Three fonts are included: 
- `bbcasc-8.bin` is the binary version of the font loaded on bootup by the vdp-1.02. It was derived from the file `agon_font.h`
 in the VDP project. It contains only ASCII characters with the pounds sign at position 0x60.
- `bbclat-8.bin` is the binary version of the font loaded on bootup by the vdp-1.04 or 2.x. It was derived from the file `agon_font.h`
 in the VDP project. It contains a complete CP-1252 character set.
- `latin1-8.bin` is an ISO 8859-1 font (Latin1) that I derived from the following github project. The font provided had
 the bytes bit-reversed compared to what we need, so I changed it to the right format. It does not look pretty, but it's a start.
https://github.com/dhepper/font8x8

You can check out https://github.com/epto/epto-fonts The `*.font`
files included in the project contain meta-information, but they start
with the bitmap data in a form that the font utility can use.  The 8x8
fonts from this set, you can just load with the font utility. It
ignores the meta data. Some of them are only ASCII, some of them have
code page 437 fonts. This program is superseded by the `loadfont` and
`fontctl` programs.

### loadfont

This program uses the new font commands of VDP-2.8.0. It can load raw
binary fonts (like the font program does) and PSF fonts as used by the
Linux console.

Example:

`loadfont 10 Lat15-Fixed16.psf`

The first parameter is a buffer ID, a number in the range
0..65534. Instead we can use the word `sys` to select the system
font. The system font can only be loaded with 8x8 characters, while
many font sizes are supported with PSF files. An optional third
parameter on the command line specifies a code page. Currently
supported code pages include windows code pages CP1250 (Middle and
Eastern Europe), CP1251 (Cyrillic), CP1252 (Western Europe), CP1253
(Greek), CP1254 (Turkish), CP1257 (Baltic), all Latin, Greek and
Cyrillic character stes from the ISO-8859 series, DOS CP437, Cyillic
KOI-8R and KOI8-U and macroman (Old Macintosh).

If a PSF font has a Unicode table, it will be used to place the glyphs
of the font at the appropriate code points in the code page.  Instead
of a code page we can specify `none` for no translation and `upper` to
load the upper 256 characters of a 512-character PSF font. Note that
most PSF fonts with Latin-1 or Windows-1252 support have many of the
non-ASCII characters at different positions, therefore the Unicode
table will be needed.

Example:
`loadfont 11 Lat15-terminus20x10.psf cp437`

Usable psf fonts can be found here
https://www.zap.org.au/projects/console-fonts-zap/ and in the
`/usr/share/consolefonts` directory of a Linux system (gunzip
compressed psf files first). Also the terminus-fonts package can be
used.

### fontctl

This program selects a font, previously loaded by the `loadfont` command.

Example:

`fontctl 11`

The parameter is a buffer ID, the same as used with `loadfont`. The
word `sys` can be used instead to select the system font.

A second parameter `show` can be used to show all 256 characters of the font.
Instead, `clear` can be used to remove the font and its buffer.

Example:

`fontctl 11 show`

selects font 11 and shows it.

`fontctl 11 clear`

clears font 11.

### recode

This program converts text files between Unicode and various code
pages.  Code pages include the same code pages as available for
`loadfont`, plus utf8 and utf16. Apart from the text encoding we can
change the end-of-line encoding to CR only (old MAC), LF only (Linux)
or CR-LF (DOS, Windows). Files are updated in-place and the old file
is saved with a `.bak` suffix.

Example commands:

`recode crlf myfile.txt`

This changes the end-of-line characters to CR-LF without changing the
character encodings.

`recode lf cp1252 utf8 foo.txt`

This converts from Windows 1252 to UTF8 and changes the end-of-line
characters to LF only.

`recode cp1252 latin9 bar.txt`

This converts a text file from CP1252 to Latin9 (ISO8859-15) without
changing the end-of-line characters.

### cal

This is a simple Unix-style calender program. Specify month and year to
show a calendar of just one month. Example:

`cal 5 2024`

shows a calendar of just May 2024. Specify just a year to show the calendar
of the whole yar. Example:

`cal 2024`

Options: -s shows Sunday as the first day of the week (default is Monday).

### concat

This program concatenates one or more files to form another file:

Example:

`concat prog1.bas prog2.bas >prog.bas`

Concatenates prog1.bas and prog2.bas and writes the whole to
prog.bas. Comparable to the Unix cat command,

### find

This program finds files (in the entire file system) whose name matches a
specific pattern.

Example:

`find *.c`

This will find all files whose name matches the pattern `*.c` in the whole
file system.

You can use the '>' character to redirect the output to a file. Example:

`find *.asm >all_asmfiles.txt`

### grep

This program searches test files for specific strings.

Example:

`grep dirlist *.c`

This will find all lines containing "dirlist" in all C source files.

You can do a case-insensitive grep using the `-i` option. It will only consider
ASCII letters. You can also enclose the search string in double quotes to
search for a string containint spaces"

Example:

`grep -i "these few words" *.txt`

You can use the '>' character to redirect the output to a file and you
can have multiple file names or wildcards. Example:

`grep dirlist *.c *.h  >results.txt`

### sort

This program sorts lines in a text file in ASCII-lexigographic order.

Example:

`sort myfile.txt`

You can use the '>' character to redirect the output to a file.
Example:

`sort myfile.txt >sorted.txt`

THe proghram has the follwoignoptions: -f to sort case-insensitive and -r
to sort in reverse order.

### wc

This program counts lines, words and characters in one or more files.
It uses the options -l, -w and -c to specify that lines words and characters
must be counted. If none are specified, all are counted.

Example:

`wc -w *.c`

Count only words in all C source files

`wc readme.txt info.txt`

Count lines, words and characters in readme.txt and info.txt

### nano

This is an editor with nano-style key bindings.

Example command: `nano myfile.txt`

The command can take a second parameter to specify the buffer, so larger files can be loaded.
Example command when specifying a buffer. This works the same way as for the copy command.

Example command using a buffer:
`nano myfile.txt &90000`

It can take a third parameter to specify the line number on which the cursor must start. Example:
`nano myfile.txt &90000 231`

The editor runs in MODE 0 (it selects this mode when started) and tries to return to the mode from which it was called.
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

### mc

This is the 12AM Commander, a Midnight-commander look-alike. It can
only work on MOS 2.2.3 and up. To install it, you must have `mc.bin`,
`12amc.ovl`, `12amc.hlp` and `12amc.cfg` all in the `/bin`
directory. The file `12amc.cfg` is a configuration file that can be
edited. You can specify viewer and editor commands and specify
programs to be run for certain file extensions.

To start the program, just type `mc`. To exit, simply press F10 or
ESC.  You will see two directories side by side and you can switch
between them with the TAB key. For more information type F1.

Note: the main code of mc resides in `12amc.ovl` and this must be
loaded and run through `mc.bin` (not directly via load & run commands
for example). The program `mc.bin` stores a launcher in internal RAM,
which can load and execute external programs at address &40000 and
later reload and rerun the `12amc.ovl` program. This launcher is an
important part of the functionality.

