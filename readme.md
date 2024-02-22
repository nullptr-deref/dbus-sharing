# DBus Sharing Framework

We'are all familiar with the sharing concept when some programs transfer
files to other programs to continue processing.

## General description

This framework consists of an executable which is being launched as a systemd
service. After that this service listens for specified sharing dbus calls and
launches another application for further file processing.

## Configuration

Default configuration file is `/etc/sharing/dbus-sharing.conf`.
It stores all sharing entries, their aliases, paths to executables and
lists of accepted file extensions.

For example:
```conf
[vim]
formats=txt,c,cpp,cxx,c++,h,hpp,hxx,h++,md,html
cmd=/usr/bin/vim
```

## Build & installation

Building & install process is straight-forward and as simple as:
```bash
$ mkdir build && cd build
$ cmake .. -DCMAKE_BUILD_TYPE=Debug
$ cmake --build . -t install
```

## Running

To run in under user-space permissions, one should type:
```bash
$ systemctl --user start org.rt.sharing
```
