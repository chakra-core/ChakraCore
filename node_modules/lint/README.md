node-lint
--------

- [Node] is a [V8] based framework for writing Javascript applications outside
  the browser.

- [JSLint] is a code quality tool that checks for problems in Javascript programs.

- **node-lint** lets you run JSLint from the command line.

- node-lint currently supports node version 0.2.1

[Node]: http://nodejs.org/
[V8]: http://code.google.com/p/v8/
[JSLint]: http://www.jslint.com/lint.html


installation
------------

npm:

    $ npm install lint


usage
-----

You can use `node-lint` directly if you have `node` in your $PATH:

    $ ./node-lint path/to/your/file.js

Or if you installed it using npm:

    $ node-lint path/to/your/file.js

Otherwise, you need to run it with node:

    $ node node-lint path/to/your/file.js

You can also specify a directory param and node-lint will find all .js files under that directory and its subdirectories:

    $ node node-lint dir1/ dir2/

Enjoy!


config
------

You can set JSLint options by modifying the default `config.js` file or even
override the default config by passing another config file with the optional
`--config` parameter, e.g.

    $ node-lint file1 file2 dir1 dir2 --config path/to/your/config/file.json

For example, if the default config.json has:

    {
        "adsafe"       : false,
        "bitwise"      : true,
        "error_prefix" : "\u001b[1m",
        "error_suffix" : ":\u001b[0m "
    }

And your own path/to/your/config/file.json looks like:

    {
        "bitwise"      : false,
        "browser"      : false
    };

Then the final options used will be:

    {
        "adsafe"       : false,
        "bitwise"      : false,
        "browser"      : false,
        "error_prefix" : "\u001b[1m",
        "error_suffix" : ":\u001b[0m "
    };

Take a look at [JSLint's Options] to see what to put in the `options` variable.


reporters
---------

By default node-lint uses an internal `reporter` function to output it's results
to the console. For basic use it's possible to alter the `error_prefix` and
`error_suffix` colors within your `config.js` file. This will prepend or append
coloring information to the results when JSLint complains about your code. There
may be times when a more customizable reporting system might be needed (*i.e.
IDE/Text Editor integrations or customized console outputs*).

node-lint allows you to designate a custom reporter for outputting the results
from JSLint's run. This `reporter` function will override the default function
built into node-lint. To utilize a custom reporter first create a js file that
has a function in it named `reporter`:

`example-reporter.js`:

    var sys = require('sys');

    function reporter(results) {
        var len = results.length;
        sys.puts(len + ' error' + ((len === 1) ? '' : 's'));
    }

Then when you run node-lint from the command line, pass in the customized
reporter:

`$ ./node-lint path/to/file.js --reporter path/to/file/example-reporter.js`

For brevity sake, this is a fairly simple reporter. For more elaborate examples
see the `examples/reporters/` directory or `examples/textmate/`.

The sample XML reporter `examples\reporters\xml.js` produces reports which can
also be integrated with a Continuous Integration server like [Hudson] using the
[Violations Plugin].

Please see the [wiki][wiki] for integration with various editors.

[Hudson]: http://hudson-ci.org
[Violations Plugin]: http://wiki.hudson-ci.org/display/HUDSON/Violations

contribute
----------

To contribute any patches, simply fork this repository using GitHub and send a
pull request to me <<http://github.com/tav>>. Thanks!


credits
-------

- [tav], wrote node-lint

- [Felix Geisendörfer][felixge], clarified Node.js specific details

- [Douglas Crockford], wrote the original JSLint and rhino.js runner

- [Nathan Landis][my8bird], updated node-lint to Node's new API.

- [Oleg Efimov][Sannis], added support for overridable configurations, running
  node-lint from a symlink and updates to reflect Node.js API changes.

- [Matthew Kitt][mkitt], added support for configurable reporters, various code
  cleanups and improvements including updates to reflect Node.js API changes.

- [Corey Hart], updated node-lint with multiple files and config support.

- [Mamading Ceesay][evangineer], added support for using node-lint within Emacs.

- [Matt Ranney][mranney], updated node-lint to use sys.error.

- [Cliffano Subagio], added npm installation support, XML reporter, and directory param support.

[tav]: http://tav.espians.com
[felixge]: http://debuggable.com
[Douglas Crockford]: http://www.crockford.com
[my8bird]: http://github.com/my8bird
[Sannis]: http://github.com/Sannis
[mkitt]: http://github.com/mkitt
[Corey Hart]: http://www.codenothing.com
[evangineer]: http://github.com/evangineer
[mranney]: http://github.com/mranney
[Cliffano Subagio]: http://blog.cliffano.com

[JSLINT's Options]: http://www.jslint.com/lint.html#options
[wiki]: http://github.com/tav/node-lint/wiki
