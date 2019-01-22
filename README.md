


## File Storage ##

Every file is determinastly split by rolling hash. Every resulting block SHA1 hash is then computed and stored in a database where the key is the SHA1 hash, and the value is the data block.

## File System ##

Every file is represented with an inode which contains the filename, filesize, permissions, and list of SHA1 hashes that constitute the file's data. 