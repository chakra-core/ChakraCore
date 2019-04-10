# dependency-diff
Generate npm install commands or objects from the diff of two package.json files


Use this module when you want to determine differences in package.json dependencies.
This module is part of MinerLabs's continuous build and deploy system.
Utilizing this module you can minimize downtime with targeted installs and uninstalls.

The best way for now to learn what the module does and expects is to look at the tests,
however here is a short write up

```javascript
var DepDiff = require('dependency-diff')

var left = require('package.json')
var right = require('other_package.json');

DepDiff().left(left).right(right).toObject()
/*
returns obj e.g.

{ dependencies: 
   [ { operation: 'edit',
       name: 'package-1',
       version: '1.0.1',
       cmd: 'npm install "package-1@1.0.1" --spin=false --color=true --production --loglevel=http' },
     { operation: 'new',
       name: 'extra-package',
       version: '1.0.0',
       cmd: 'npm install "extra-package@1.0.0" --spin=false --color=true --production --loglevel=http' } ],
  devDependencies: 
   [ { operation: 'edit',
       name: 'dev-package-1',
       version: '1.0.1',
       cmd: 'npm install "dev-package-1@1.0.1" --spin=false --color=true --production --loglevel=http' },
     { operation: 'new',
       name: 'extra-package',
       version: '1.0.0',
       cmd: 'npm install "extra-package@1.0.0" --spin=false --color=true --production --loglevel=http' } ],
  optionalDependencies: 
   [ { operation: 'edit',
       name: 'optional-package-1',
       version: '1.0.1',
       cmd: 'npm install "optional-package-1@1.0.1" --spin=false --color=true --production --loglevel=http' },
     { operation: 'delete',
       name: 'only-left',
       version: '1.0.0',
       cmd: 'npm uninstall only-left' },
     { operation: 'new',
       name: 'extra-package',
       version: '1.0.0',
       cmd: 'npm install "extra-package@1.0.0" --spin=false --color=true --production --loglevel=http' } ] }
*/


DepDiff().left(left).right(right).toCmdList()
/*

[ 'npm install "package-1@1.0.1" --spin=false --color=true --production --loglevel=http',
  'npm install "extra-package@1.0.0" --spin=false --color=true --production --loglevel=http',
  'npm install "dev-package-1@1.0.1" --spin=false --color=true --production --loglevel=http',
  'npm install "extra-package@1.0.0" --spin=false --color=true --production --loglevel=http',
  'npm install "optional-package-1@1.0.1" --spin=false --color=true --production --loglevel=http',
  'npm uninstall only-left',
  'npm install "extra-package@1.0.0" --spin=false --color=true --production --loglevel=http' ]
*/


```