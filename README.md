# agon-utilities
MOS utilities for Agon: copy, view, editor...

These are small utilities that everybody has taken for granted on UNIX, but even on MS-DOS for a long time. But Agon did not have them until now.
The project will include an editor some day. All are MOS commands, all are written in Assembly language and all run in 24-bit ADL mode.
Parts of the code are based on the repository https://github.com/breakintoprogram/agon-projects

##copy

The copy command can copy a single file. By default the command is limited to A file up to about 31kB, as it has to store the file instead the area
reserved for MOS commands. However, a third parameter can specify a buffer address and then the program can use a buffer at the specified address, extending
until the start of the MOS area. For example specifying &80000 will allow the area &80000-&AFFFF as a file buffer. The program has to store the entire
file in a single buffer due to limitations of MOS v1.02.

##comp

Once you copied a file, you want to check whether the copied file is an exact copy. The comp utility compares two files. It tells whether they are the same,
at which offset the first difference occurs or whether one file is larger than the other but they are the same up to the length of the smaller file.

##more

The well-known utility from early Unix and also MS-DOS. MOS does not support pipelines, so you cannot pipe the output of the CAT command to it. It can just show
a single file and pauses after s screenful has been shown.

##memfill

Can be useful to clean the contents of RAM, so you see immediatley which memory areas have been touched by your program. membdump is already there in a different respository.
