#!/bin/bash

dd if=/dev/zero of=file.fs bs=1M count=128
mkfs.ext2 file.fs

dd if=/dev/zero of=parts.fs bs=1M count=256
mkfs.ext2 parts.fs