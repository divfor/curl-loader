#!/bin/sh

cd packages/
rm -f $1.tar.bz2
tar jcvf $1.tar.bz2 $1/
rm -rf $1/

