#!/bin/sh

# bootstrap.sh
#
# This is the bootstrapping script to auto-generate a configure
# script for checking build environments, etc.
#

rm -f config.cache aclocal.m4
aclocal #create m4 environment for autotools to use
autoconf #generate configure script
#automake --add-missing #generate Makefile