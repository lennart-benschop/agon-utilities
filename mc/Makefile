# ----------------------------
# Makefile Options
# ----------------------------

NAME = mc
ICON = icon.png
DESCRIPTION = "Midnight Commander look-alike"
COMPRESSED = NO
ARCHIVED = NO
INIT_LOC = 040000
BSSHEAP_LOW = 060000
BSSHEAP_HIGH = 08FFFF
STACK_HIGH = 08FFFF

CFLAGS = -Werror -Wall -Wextra -Oz -DCEDEV
CXXFLAGS = -Werror -Wall -Wextra -Oz - DCEDEV
LDHAS_EXIT_HANDLER:=0
# ----------------------------

include $(shell cedev-config --makefile)

