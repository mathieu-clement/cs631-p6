#!/bin/sh
CFLAGS=-g make pa && ./pa seq 10 \| sort -n -r \| wc -l && cat pa.log
