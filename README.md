

## Resources ##

https://github.com/gkostka/lwext4
http://www.giis.co.in/libext2fs.pdf
http://gauss.ececs.uc.edu/Courses/c4029/labs/lab11.html
https://github.com/libfuse/libfuse/blob/master/example/passthrough.c

create ext2 image with:
```
# dd if=/dev/zero of=file.fs bs=1M count=128
# mkfs.ex2 file.fs
```

## File Storage ##

Every file is determinastly split by rolling hash. Every resulting block SHA1 hash is then computed and stored in a database where the key is the SHA1 hash, and the value is the data block.

## File System ##

Every file is represented with an inode which contains the filename, filesize, permissions, and list of SHA1 hashes that constitute the file's data. 

