# ----------------------------
# Makefile Options
# ----------------------------

NAME = cal
ICON = icon.png
DESCRIPTION = "Unix-style calendar"
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

