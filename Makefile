ASM=ez80asm

.PHONY: binaries
binaries: mos/memfill.bin mos/more.bin mos/font.bin mos/comp.bin mos/nano.bin

mos/memfill.bin: memfill.asm *.inc
	$(ASM) $< $@

mos/more.bin: more.asm *.inc
	$(ASM) $< $@

mos/font.bin: font.asm *.inc
	$(ASM) $< $@

mos/comp.bin: comp.asm *.inc
	$(ASM) $< $@

mos/nano.bin: nano.asm *.inc
	$(ASM) $< $@
