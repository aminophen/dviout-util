#!/bin/sh
# backport from TL tree (texjporg/tex-jp-build) -> standalone repo

# without encoding conversion
# (tests/jistest.pl is included but overwritten later)
for x in 00texjporg/{README,COPYING,ChangeLog,*.man,*.test}; do \
	cp "$x" "./${x##*/}"; \
done
cp 00texjporg/tests/* tests/

# with encoding conversion
# GNU iconv can be used to convert UTF-8 -> CP932
for x in 00texjporg/*.{c,h,txt}; do \
	if [ "$x" != "00texjporg/config.h" ]; then \
		iconv -f UTF-8 -t CP932 "$x" >"./${x##*/}"; \
	fi \
done
iconv -f UTF-8 -t CP932 "00texjporg/tests/jistest.pl" >"./tests/jistest.pl"
