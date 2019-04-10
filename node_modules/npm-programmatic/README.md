# npm-programmatic [![Build Status](https://travis-ci.org/Manak/npm-programmatic.svg?branch=master)](https://travis-ci.org/Manak/npm-programmatic)   
[![NPM](https://nodei.co/npm/npm-programmatic.png?downloads=true&downloadRank=true&stars=true)](https://nodei.co/npm/npm-programmatic/)    

npm-programmatic is a library that allows you to access npm commands programmatically from javascript
## Usage
Every function returns a Bluebird promise.   
CWD refers to current working directory, allowing you to ensure the command executes in a certain folder in the filesystem.
If output is set, the output of npm will be shown in the console.

## Installation of Packages

``` 
    npm.install(packages, opts).then(function)
```
| Name        | Type           | Value  |
| ------------- |:-------------:| -----:|
| packages      | Array      |   packages to be installed |
| opts      | Object | save:true/false; global:true/false; cwd:string; saveDev:true/false; noOptional:true/false; legacyBundling: true/false; output:true/false|

### Example
``` 
    var npm = require('npm-programmatic');
    npm.install(['left-pad'], {
        cwd:'/path/to/my/project',
        save:true
    })
    .then(function(){
        console.log("SUCCESS!!!");
    })
    .catch(function(){
        console.log("Unable to install package");
    });
```

## Unistallation of Packages

``` 
    npm.uninstall(packages, opts).then(function)
```
| Name        | Type           | Value  |
| ------------- |:-------------:| -----:|
| packages      | Array      |   packages to be uninstalled |
| opts      | Object | save:true/false; global:true/false; cwd:string; saveDev:true/false; output:true/false|

### Example
``` 
    var npm = require('npm-programmatic');
    npm.uninstall(['left-pad'], {
        cwd:'/path/to/my/project',
        save:true
    })
    .then(function(){
        console.log("SUCCESS!!!");
    })
    .catch(function(){
        console.log("Unable to uninstall package");
    });
```


## List Installed Packages

``` 
    npm.list(path).then(function)
```
| Name        | Type           | Value  |
| ------------- |:-------------:| -----:|
| path      | String      |   path at which to look |

### Example
``` 
    var npm = require('npm-programmatic');
    npm.list('/path/to/project')
    .then(function(arrayOfPackages){
        console.log(arrayOfPackages);
    })
    .catch(function(){
        console.log("Unable to uninstall package");
    });
```

## Tests
install mocha and dev dependencies. Then run 
``` npm test    ```
