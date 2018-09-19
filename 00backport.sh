#!/bin/sh
# backport from TL tree (texjporg/tex-jp-build) -> standalone repo

# without encoding conversion
for x in 00texjporg/{README,COPYING,ChangeLog,*.test}; do \
	cp "$x" "./${x##*/}"; \
done

# with encoding conversion
# GNU iconv can be used to convert UTF-8 -> CP932
for x in 00texjporg/*.{c,h,txt}; do \
	if [ "$$x" != "00texjporg/config.h" ]; then \
		iconv -f UTF-8 -t CP932 "$x" >"./${x##*/}"; \
	fi \
done
