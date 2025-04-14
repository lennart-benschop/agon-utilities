# ----------------------------
# Makefile Options
# ----------------------------

NAME = wc
ICON = icon.png
DESCRIPTION = "Utility to count lines, words chars"
COMPRESSED = NO
ARCHIVED = NO
RAM_START=0xb0000
RAM_SIZE=0x8000

CFLAGS = -Werror -Wall -Wextra -Oz -DCEDEV
CXXFLAGS = -Werror -Wall -Wextra -Oz - DCEDEV
LDHAS_ARG_PROCESSING = 1
LDHAS_EXIT_HANDLER:=0
# ----------------------------

include $(shell agondev-config --makefile)

