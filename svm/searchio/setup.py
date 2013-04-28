#!/usr/bin/env python
# Setup script for the searchio module

from distutils.core import setup, Extension

searchio = Extension("searchio", sources = ["searchio.c", "stemmer.c", "sparseindex.c"])

setup(
    name = "searchio",
    version = "1.0",
    description = "A Python module for interacting with CS158 Search Engine indices.",
    ext_modules = [searchio]
)
