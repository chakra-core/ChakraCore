Fast Source Map Concatenation
-----------------------------

[![Build Status](https://travis-ci.org/ef4/fast-sourcemap-concat.svg?branch=master)](https://travis-ci.org/ef4/fast-sourcemap-concat)
[![Build status](https://ci.appveyor.com/api/projects/status/0iy8on5vieoh3mp2/branch/master?svg=true)](https://ci.appveyor.com/project/embercli/fast-sourcemap-concat/branch/master)

This library lets you concatenate files (with or without their own
pre-generated sourcemaps), and get a single output file along with a
sourcemap.

It was written for use in ember-cli via broccoli-sourcemap-concat.

source-map dependency
---------------------

We depend on mozilla's source-map library, but only to use their
base64-vlq implementation, which is in turn based on the version in
the Closure Compiler.

We can concatenate much faster than source-map because we are
specifically optimized for line-by-line concatenation.
