#!/bin/bash
############################################################################
# Copyright:
#      (C) 2008 - 2009  Alexander Shaduri <ashaduri 'at' gmail.com>
# License: See LICENSE_zlib.txt file
############################################################################

# We use #!/bin/bash of #!/bin/sh because freebsd (tested with 6.3) in its
# infinite wisdom has csh-like /bin/sh (even though the SUS says it
# must be bourne-compatible).


if [ "$1" == "" ] | [ "$2" == "" ] | [ "$3" == "" ]; then
	echo "Usage: $0 <input_file> <cpp_file> <symbol_name>";
	exit 1;
fi

if [ ! -e $1 ]; then
	echo "Cannot open input file \"$1\".";
	exit 2;
fi


in_file="$1";
out_file="$2";
sym_name="$3";
size=`stat -c "%s" "$in_file"`


# header. extern means "global" in this context (needed because const
# variables are static by default).
echo "

extern const unsigned char ${sym_name}[] = {
" > "$out_file";


# file binary. "xxd(1) -i" can also do the trick
hexdump -v -e '1/1 "0x%X,\n"' "$in_file" >> "$out_file";


# footer. add 0x0 byte for easy stringifying
echo "
 0x0 };

extern const unsigned int ${sym_name}_size = ${size};

" >> "$out_file";


