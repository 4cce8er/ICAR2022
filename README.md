# ICAR_2022

## Ad-hoc solution for gzipToTrace problem on Rackham:
I experienced segmentation fault when the program executes
`inbuf.push(boost::iostreams::gzip_decompressor())` which is in gzipToTrace.h.

Using gdb, I found out boost_iostream's linked .so library does not match its
header file version. I had `module load`ed boost 1.80 but I was linking against
`/usr/lib64/libboost_iostreams.so.1.53.0`. It seems that the function
`gzip_header::reset()` cannot find its definition in the .so file.

The current solution is to add a `-L` flag while compiling to include the
shared lib path, so as to not use the .so file in `/usr/lib64`.
The path I am using is:

```
/sw/libs/boost/1.80.0_gcc10.3.0/rackham/lib/
```

Of course this specifically works for boost 1.80.0. Thus please load module:

```
module load gcc/10.3.0
module load boost/1.80.0_gcc10.3.0
```

before compiling the program.
