## Description

Personal sandbox

## Install

```
conan install . --build=missing
conan install . -s build_type=Debug --build=missing
```

## Conan profile example:

```
[settings]
arch=x86_64
build_type=Release
compiler=clang
compiler.cppstd=23
compiler.libcxx=libstdc++11
compiler.version=18
os=Linux

[conf]
tools.build:compiler_executables={'c': '/usr/bin/clang', 'cpp': '/usr/bin/clang++'}
tools.cmake.cmaketoolchain:generator=Ninja
```
