#!/bin/bash

# Default for all directories
svn propset svn:ignore -R -F .svnignore-default.txt .

# Properties for each directory
for dir in . autoconf.m4 po; do
	pushd $dir
		svn propset svn:ignore -F .svnignore.txt .
	popd
done
