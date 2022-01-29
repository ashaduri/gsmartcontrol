#!/bin/bash
###############################################################################
# License: BSD Zero Clause License file
# Copyright:
#   (C) 2008 - 2022 Alex Butcher <alex dot butcher 'at' assursys.co.uk>
#   (C) 2008 - 2022 Alexander Shaduri <ashaduri@gmail.com>
###############################################################################

device="$1";

if [ "$device" = "" ]; then
	echo "Usage: $0 <device>"
	exit 1;
fi

dev_base=$(basename "$device")
out_file=/var/run/smart-"$dev_base"

# Change the path to smartctl if necessary.
smartctl -x "$device" > "$out_file" 2>&1

chmod 644 "$out_file"
