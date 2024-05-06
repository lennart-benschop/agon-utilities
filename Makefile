ASM=ez80asm

.PHONY: binaries
binaries: mos/memfill.bin mos/more.bin mos/font.bin mos/fontctl.bin mos/comp.bin mos/nano.bin bin/loadfont.bin bin/recode.bin

loadfont/src/codepages.h: loadfont/src/gen_codepages.py
	cd loadfont/src;python3 gen_codepages.py >codepages.h

mos/memfill.bin: memfill.asm *.inc
	$(ASM) $< $@

mos/more.bin: more.asm *.inc
	$(ASM) $< $@

mos/font.bin: font.asm *.inc
	$(ASM) $< $@

mos/fontctl.bin: fontctl.asm *.inc
	$(ASM) $< $@

mos/comp.bin: comp.asm *.inc
	$(ASM) $< $@

mos/nano.bin: nano.asm *.inc
	$(ASM) $< $@

bin/loadfont.bin: loadfont/src/*.[ch]
	mkdir -p bin
	cd loadfont;make;mv bin/loadfont.bin ../bin

bin/recode.bin: recode/src/*.[ch] loadfont/src/codepages.h
	mkdir -p bin
	cd recode;make;mv bin/recode.bin ../bin


