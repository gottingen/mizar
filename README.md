# mizar

Mizar is a fork of [RocksDB v9.2.0](https://github.com/facebook/rocksdb/releases) with the following changes:
- promote containers performance by using turbo
- built with EA(Elastic Architecture) environment
- to avoid conflict with RocksDB, the library name is changed to `mizar`
- change license to GAPL-3.0(GNU AFFERO GENERAL PUBLIC LICENSE)

note that mizar may not run on windows, we focus on server-side performance and stability on linux.
although it may run on windows, we do not test it on windows.

## Build

requirements:
- cmake >= 3.24
- gcc >= 9.3.0

### install gottingen/essential

the dependencies of mizar are installed to /opt/EA/inf, such as zlib, snappy, zstd, they are all static libraries. 
so the program executable file is independent of the system environment.

```bash
git clone https://github.com/goettingen/essential.git
cd essential
./install_base.sh
```

```
using pacakge to install is convenient for uninstalling.

### build mizar

```bash
git clone https://github.com/goettingen/mizar.git
cd mizar
mkdir build
cd build
cmake ..
make -j 6
make package
```


# below is rocksdb's original README.md

## RocksDB: A Persistent Key-Value Store for Flash and RAM Storage

[![CircleCI Status](https://circleci.com/gh/facebook/rocksdb.svg?style=svg)](https://circleci.com/gh/facebook/rocksdb)

RocksDB is developed and maintained by Facebook Database Engineering Team.
It is built on earlier work on [LevelDB](https://github.com/google/leveldb) by Sanjay Ghemawat (sanjay@google.com)
and Jeff Dean (jeff@google.com)

This code is a library that forms the core building block for a fast
key-value server, especially suited for storing data on flash drives.
It has a Log-Structured-Merge-Database (LSM) design with flexible tradeoffs
between Write-Amplification-Factor (WAF), Read-Amplification-Factor (RAF)
and Space-Amplification-Factor (SAF). It has multi-threaded compactions,
making it especially suitable for storing multiple terabytes of data in a
single database.

Start with example usage here: https://github.com/facebook/rocksdb/tree/main/examples

See the [github wiki](https://github.com/facebook/rocksdb/wiki) for more explanation.

The public interface is in `include/`.  Callers should not include or
rely on the details of any other header files in this package.  Those
internal APIs may be changed without warning.

Questions and discussions are welcome on the [RocksDB Developers Public](https://www.facebook.com/groups/rocksdb.dev/) Facebook group and [email list](https://groups.google.com/g/rocksdb) on Google Groups.

## License

RocksDB is dual-licensed under both the GPLv2 (found in the COPYING file in the root directory) and Apache 2.0 License (found in the LICENSE.Apache file in the root directory).  You may select, at your option, one of the above-listed licenses.
