#!/bin/sh

# autogen.sh
#
# This script will auto-generate a configure script for checking build environments.
autoconf #generate configure script
#don't call automake as we already have a simple template in Makefile.in
