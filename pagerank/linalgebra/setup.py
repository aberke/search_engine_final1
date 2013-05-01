#!/usr/bin/env python
# Setup script for the linalgebra module

from distutils.core import setup, Extension

linalgebra = Extension("linalgebra", sources = ["linalgebra.c"])

setup(
    name = "linalgebra",
    version = "1.0",
    description = "A Python module for dealing with 'sparce' matrices encountered in 158 project.",
    ext_modules = [linalgebra]
)