#! /bin/sh

#echo "Adding libtools..."
#libtoolize --automake --copy

# echo "Running aclocal (builds aclocal.m4)..."
# aclocal -I ./autoconf_m4

# echo "Running autoheader (generates config.h.in)..."
# autoheader

# echo "Running automake..."
# automake --add-missing --copy --foreign

# echo "Running autoconf..."
# autoconf


autoreconf --verbose --install -W all --force


rm -f config.cache

echo
echo 'run "./configure ; make ; make install"'
echo

