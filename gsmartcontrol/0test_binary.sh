#!/bin/bash

# This file can be used as a test file for
# system/smartctl_binary = string "../../0test_binary.sh"

#smartctl $*
#exit

#echo adbc
#exit

dir="../../test-data/smartmon-site";
#dir="../../test-data/prob";
#dir="../../test-data/smartmon-mail";
#dir="../../test-data/mine";


ls_out=`ls --color=no -1 "$dir"`;
n_lines=`echo "$ls_out" | wc -l`;
random -e "$n_lines"
line_no=$?  # 0-based

f=`echo "$ls_out" | tail -n $(($n_lines - $line_no)) | head -n 1`

cat "$dir"/"$f"

#cat "../../test-data/smartmon-site/ST910021AS.txt.health_failed"
# echo "-------- A --------"


if [ "$1" != "--info" ]; then
#	sleep 4
	:
fi

