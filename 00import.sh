#!/bin/sh
# import from standalone repo -> TL tree (texjporg/tex-jp-build)
mkdir -p 00texjporg
mkdir -p 00texjporg/tests

# without encoding conversion
# (tests/jistest.pl is included but overwritten later)
for x in {README,COPYING,ChangeLog,*.man,*.test,tests/*}; do \
	cp "$x" "00texjporg/$x"; \
done

# with encoding conversion
# GNU iconv can be used to convert CP932 -> UTF-8
for x in *.{c,h,txt} tests/jistest.pl; do \
	if [ "$x" != "config.h" ]; then \
		iconv -f CP932 -t UTF-8 "$x" >"00texjporg/$x"; \
	fi \
done
