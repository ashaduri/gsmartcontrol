#!/bin/bash
############################################################################
# Copyright:
#      (C) 2008  Alex Butcher <alex dot butcher 'at' assursys.co.uk>
#      (C) 2008  Alexander Shaduri <ashaduri 'at' gmail.com>
# License: See LICENSE_whatever.txt file
############################################################################

device="$1";

if [ "$device" = "" ]; then
	echo "Usage: $0 <device>"
	exit 1;
fi

dev_base="`basename \"$device\"`"
out_file=/var/run/smart-"$dev_base"

# Change the path to smartctl if necessary.
smartctl -a "$device" 2>&1 > "$out_file"

chmod 644 "$out_file"
