#!/bin/sh

lint -m -u -v -DHAVE_CONFIG_H src/*.c src/interpreter/interpreter.c src/interpreter/interpreter_playlist.c src/xml/*.c src/playlist_builtin/*.c -I. -Isrc -Iresolver -Ilibshout -Ithread -Ithread/avl -I$HOME/lame3.86

lint -m -u -v -DHAVE_CONFIG_H -I. -Ithread -Ithread/avl resolver/*.c

lint -m -u -v -DHAVE_CONFIG_H -I -Ithread -Ithread/avl thread/thread.c thread/avl/avl.c

lint -m -u -v -DHAVE_CONFIG_H -I -Ithread -Ithread/avl -Ilibshout libshout/*.c
