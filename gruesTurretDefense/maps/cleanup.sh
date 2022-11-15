#!/bin/bash
set -eu
if [[ $# -ne 1 ]]; then
	echo "Usage: ${0##/} <filename>" >&2
	exit 1
fi

file=$1
if [[ ! -f $file ]]; then
	echo "[${0##/}] file not found: ${file}" >&2
	exit 1
fi

cat $file | \
	awk '/name brick/{for(x=NR-1;x<=NR+6;x++)d[x];}{a[NR]=$0}END{for(i=1;i<=NR;i++)if(!(i in d))print a[i]}' | \
	sed -e 's/brick[0-9]\+/brick/g' \
	| \
	awk '/name concrete_dark/{for(x=NR-1;x<=NR+6;x++)d[x];}{a[NR]=$0}END{for(i=1;i<=NR;i++)if(!(i in d))print a[i]}' | \
	sed -e 's/concrete_dark[0-9]\+/concrete_dark/g' \
	| \
	awk '/name concrete/{for(x=NR-1;x<=NR+6;x++)d[x];}{a[NR]=$0}END{for(i=1;i<=NR;i++)if(!(i in d))print a[i]}' | \
	sed -e 's/concrete[0-9]\+/concrete/g' \
	| \
	awk '/name glass/{for(x=NR-1;x<=NR+6;x++)d[x];}{a[NR]=$0}END{for(i=1;i<=NR;i++)if(!(i in d))print a[i]}' | \
	sed -e 's/glass[0-9]\+/glass/g' \
	| \
	awk '/name metal/{for(x=NR-1;x<=NR+6;x++)d[x];}{a[NR]=$0}END{for(i=1;i<=NR;i++)if(!(i in d))print a[i]}' | \
	sed -e 's/metal[0-9]\+/metal/g' \
	\
	> ${file}-new
	
mv ${file} ${file}-bak
mv ${file}-new ${file}
	
exit 0

