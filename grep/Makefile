# ----------------------------
# Makefile Options
# ----------------------------

NAME = grep
ICON = icon.png
DESCRIPTION = "Utility search files for strings"
COMPRESSED = NO
ARCHIVED = NO
INIT_LOC = 0B0000
BSSHEAP_LOW = 0B5000
BSSHEAP_HIGH = 0B7FFF
STACK_HIGH = 0B7FFF

CFLAGS = -Werror -Wall -Wextra -Oz -DCEDEV
CXXFLAGS = -Werror -Wall -Wextra -Oz - DCEDEV
LDHAS_ARG_PROCESSING = 1
LDHAS_EXIT_HANDLER:=0
# ----------------------------

include $(shell cedev-config --makefile)

