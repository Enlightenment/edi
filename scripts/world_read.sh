#!/bin/sh

for file in "$@" ; do
	chmod a+r "$DESTDIR/$file"
done
