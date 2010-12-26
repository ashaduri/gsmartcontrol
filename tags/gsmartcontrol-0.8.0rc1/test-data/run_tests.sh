#!/bin/sh

BIN="../debuggy/src/applib/smartctl_parser_test"

files=0;
errors=0

for f in */*; do
	out=`$BIN "$f" 2>&1`

	echo $out | grep '<error>' &> /dev/null;
	if [ $? != 1 ]; then
		echo "Error in $f";
		let errors+=1
	fi

	echo $out | grep '<warn>' &> /dev/null;
	if [ $? != 1 ]; then
		echo "Warning in $f";
		let errors+=1
	fi

	let files+=1
done


echo "All done. Files: $files, errors: $errors."
