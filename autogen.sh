#!/bin/sh

# autogen.sh
#
# This script will auto-generate a configure script for checking build environments.

rm -f config.cache aclocal.m4
aclocal #create m4 environment for autotools to use
autoconf #generate configure script
#don't call automake as we already have a simple template in Makefile.in
