Edi - The Enlightened IDE
===

This is a project to create a complete IDE using the EFL.
It aims to lower the barrier to getting involved in Enlightenment development
and in creating apps based on the EFL suite.

![Edi Logo](data/desktop/edi.png?raw=true)

## Requirements

meson
ninja
EFL latest release (>= 1.22.0)
libclang-dev (or llvm-clang-devel)

## Installation

Using meson and ninja to install this software is the usual:

    meson build/
    cd build
    ninja
    sudo ninja install

## Usage

After installing just launch

    edi

and it will prompt for a project (directory) location or you can specify like:

    edi ~/Code/myproject

to open the specified project.

Also included are handy utility apps that you can try

    edi_build
    edi_scm

