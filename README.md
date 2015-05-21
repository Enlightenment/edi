EDI - The Enlightened IDE
===

This is a project to create a complete IDE using the EFL.
It aims to lower the barrier to getting involved in Enlightenment development
and in creating apps based on the EFL suite.

![EDI Logo](data/desktop/edi.png?raw=true)

## Requirements

autotools
EFL & Elementary from git master (>= 1.14.99)
libclang-dev (or llvm-clang-devel)

## Installation

Using autotools to install this software is the usual:

    ./autogen.sh
    make
    sudo make install

## Usage

After installing just launch

    edi

and it will prompt for a project (directory) location or you can specify like:

    edi ~/Code/myproject

to open the specified project.

