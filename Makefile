ASM=ez80asm

.PHONY: binaries
binaries: mos/memfill.bin mos/more.bin mos/font.bin mos/fontctl.bin mos/comp.bin mos/nano.bin bin/loadfont.bin bin/recode.bin mos/find.bin mos/grep.bin mos/wc.bin mos/concat.bin mos/cal.bin bin/sort.bin  bin/mc.bin bin/12amc.ovl 

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

mos/find.bin: find/src/*.[ch]
	mkdir -p bin
	cd find;make;mv bin/find.bin ../mos

mos/grep.bin: grep/src/*.[ch]
	mkdir -p bin
	cd grep;make;mv bin/grep.bin ../mos

mos/wc.bin: wc/src/*.[ch]
	mkdir -p bin
	cd wc;make;mv bin/wc.bin ../mos

mos/concat.bin: concat/src/*.[ch]
	mkdir -p bin
	cd concat;make;mv bin/concat.bin ../mos

mos/cal.bin: cal/src/*.[ch]
	mkdir -p bin
	cd cal;make;mv bin/cal.bin ../mos

bin/sort.bin: sort/src/*.[ch]
	mkdir -p bin
	cd sort;make;mv bin/sort.bin ../bin


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
