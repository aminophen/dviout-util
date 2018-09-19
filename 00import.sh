#!/bin/sh
# import from standalone repo -> TL tree (texjporg/tex-jp-build)

# without encoding conversion
for x in {README,COPYING,ChangeLog,*.test}; do \
	cp "$x" "00texjporg/$x"; \
done

# with encoding conversion
# GNU iconv can be used to convert CP932 -> UTF-8
for x in *.{c,h,txt}; do \
	if [ "$x" != "config.h" ]; then \
		iconv -f CP932 -t UTF-8 "$x" >"00texjporg/$x"; \
	fi \
done
