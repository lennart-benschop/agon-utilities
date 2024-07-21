ASM=ez80asm

.PHONY: binaries
binaries: mos/memfill.bin mos/more.bin mos/font.bin mos/fontctl.bin mos/comp.bin mos/nano.bin bin/loadfont.bin bin/recode.bin bin/mc.bin bin/12amc.ovl

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

bin/12amc.ovl: mc/src/*.[ch] 
	mkdir -p bin
	cd mc;make;mv bin/mc.bin ../bin/12amc.ovl

bin/mc.bin: mc/launcher.bin mc/mc.asm
	mkdir -p bin
	cd mc;$(ASM) mc.asm  ../bin/mc.bin

mc/launcher.bin: mc/launcher.asm mos_api.inc
	cd mc;$(ASM) launcher.asm

clean:
	cd mc;make clean;cd ../recode;make clean;cd ../loadfont;make clean;cd ..; rm -f bin/*.ovl bin/*.bin mos/*.bin
