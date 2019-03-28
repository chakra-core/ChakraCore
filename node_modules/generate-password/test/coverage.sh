#!/bin/sh
set -x

ROOT=$PWD

# Make a copy of src into src-cov, with the coverage code.
$ROOT/node_modules/.bin/jscover $ROOT/src $ROOT/src-cov

# Run the tests.
JSCOV=1 $ROOT/node_modules/.bin/mocha -R mocha-lcov-reporter > $ROOT/coverage.lcov

# Update the server with the results.
$ROOT/node_modules/.bin/codecov

# Clean up.
rm -f $ROOT/coverage.lcov
rm -rf $ROOT/src-cov
