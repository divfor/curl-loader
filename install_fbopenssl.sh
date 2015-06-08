#!/bin/bash

if [ -f "/usr/lib/libfbopenssl.so" ]; then
   exit 0
else
   cd packages/
   tar zxvf fbopenssl-0.0.4-for-curl-loader.tar.gz
   cd fbopenssl/
   make clean
   make
   make install
   cd ../..
   exit 0
fi

