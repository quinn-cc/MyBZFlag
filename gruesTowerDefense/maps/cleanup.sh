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
	awk '/name base_brick_siding/{for(x=NR-1;x<=NR+7;x++)d[x];}{a[NR]=$0}END{for(i=1;i<=NR;i++)if(!(i in d))print a[i]}' | \
	sed -e 's/base_brick_siding[0-9]\+/base_brick_siding/g' \
	| \
	awk '/name brick/{for(x=NR-1;x<=NR+6;x++)d[x];}{a[NR]=$0}END{for(i=1;i<=NR;i++)if(!(i in d))print a[i]}' | \
	sed -e 's/brick[0-9]\+/brick/g' \
	| \
	awk '/name concrete_dark/{for(x=NR-1;x<=NR+6;x++)d[x];}{a[NR]=$0}END{for(i=1;i<=NR;i++)if(!(i in d))print a[i]}' | \
	sed -e 's/concrete_dark[0-9]\+/concrete_dark/g' \
	| \
	awk '/name concrete_green/{for(x=NR-1;x<=NR+6;x++)d[x];}{a[NR]=$0}END{for(i=1;i<=NR;i++)if(!(i in d))print a[i]}' | \
	sed -e 's/concrete_green[0-9]\+/concrete_green/g' \
	| \
	awk '/name concrete_grey/{for(x=NR-1;x<=NR+6;x++)d[x];}{a[NR]=$0}END{for(i=1;i<=NR;i++)if(!(i in d))print a[i]}' | \
	sed -e 's/concrete_grey[0-9]\+/concrete_grey/g' \
	| \
	awk '/name concrete_red/{for(x=NR-1;x<=NR+6;x++)d[x];}{a[NR]=$0}END{for(i=1;i<=NR;i++)if(!(i in d))print a[i]}' | \
	sed -e 's/concrete_red[0-9]\+/concrete_red/g' \
	| \
	awk '/name default/{for(x=NR-1;x<=NR+6;x++)d[x];}{a[NR]=$0}END{for(i=1;i<=NR;i++)if(!(i in d))print a[i]}' | \
	sed -e 's/default[0-9]\+/default/g' \
	| \
	awk '/name glow_green/{for(x=NR-1;x<=NR+6;x++)d[x];}{a[NR]=$0}END{for(i=1;i<=NR;i++)if(!(i in d))print a[i]}' | \
	sed -e 's/glow_green[0-9]\+/glow_green/g' \
	| \
	awk '/name metal/{for(x=NR-1;x<=NR+6;x++)d[x];}{a[NR]=$0}END{for(i=1;i<=NR;i++)if(!(i in d))print a[i]}' | \
	sed -e 's/metal[0-9]\+/metal/g' \
	| \
	awk '/name glow_red/{for(x=NR-1;x<=NR+6;x++)d[x];}{a[NR]=$0}END{for(i=1;i<=NR;i++)if(!(i in d))print a[i]}' | \
	sed -e 's/glow_red[0-9]\+/glow_red/g' \
	| \
	awk '/name scarwall_logo/{for(x=NR-1;x<=NR+6;x++)d[x];}{a[NR]=$0}END{for(i=1;i<=NR;i++)if(!(i in d))print a[i]}' | \
	sed -e 's/scarwall_logo[0-9]\+/scarwall_logo/g' \
	| \
	awk '/name glow_lamppost/{for(x=NR-1;x<=NR+6;x++)d[x];}{a[NR]=$0}END{for(i=1;i<=NR;i++)if(!(i in d))print a[i]}' | \
	sed -e 's/glow_lamppost[0-9]\+/glow_lamppost/g' \
	| \
	awk '/name wood/{for(x=NR-1;x<=NR+6;x++)d[x];}{a[NR]=$0}END{for(i=1;i<=NR;i++)if(!(i in d))print a[i]}' | \
	sed -e 's/wood[0-9]\+/wood/g' \
	| \
	awk '/name rock/{for(x=NR-1;x<=NR+6;x++)d[x];}{a[NR]=$0}END{for(i=1;i<=NR;i++)if(!(i in d))print a[i]}' | \
	sed -e 's/rock[0-9]\+/rock/g' \
	| \
	awk '/name gemstone_blue/{for(x=NR-1;x<=NR+6;x++)d[x];}{a[NR]=$0}END{for(i=1;i<=NR;i++)if(!(i in d))print a[i]}' | \
	sed -e 's/gemstone_blue[0-9]\+/gemstone_blue/g' \
	| \
	awk '/name gemstone_purple/{for(x=NR-1;x<=NR+6;x++)d[x];}{a[NR]=$0}END{for(i=1;i<=NR;i++)if(!(i in d))print a[i]}' | \
	sed -e 's/gemstone_purple[0-9]\+/gemstone_purple/g' \
	| \
	awk '/name glass_blue/{for(x=NR-1;x<=NR+6;x++)d[x];}{a[NR]=$0}END{for(i=1;i<=NR;i++)if(!(i in d))print a[i]}' | \
	sed -e 's/glass_blue[0-9]\+/glass_blue/g' \
	\
	> ${file}-new
	
mv ${file} ${file}-bak
mv ${file}-new ${file}
	
exit 0

