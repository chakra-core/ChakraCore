'use strict';

module.exports = function pathinfo(path, options) {
  //  discuss at: http://locutus.io/php/pathinfo/
  // original by: Nate
  //  revised by: Kevin van Zonneveld (http://kvz.io)
  // improved by: Brett Zamir (http://brett-zamir.me)
  // improved by: Dmitry Gorelenkov
  //    input by: Timo
  //      note 1: Inspired by actual PHP source: php5-5.2.6/ext/standard/string.c line #1559
  //      note 1: The way the bitwise arguments are handled allows for greater flexibility
  //      note 1: & compatability. We might even standardize this
  //      note 1: code and use a similar approach for
  //      note 1: other bitwise PHP functions
  //      note 1: Locutus tries very hard to stay away from a core.js
  //      note 1: file with global dependencies, because we like
  //      note 1: that you can just take a couple of functions and be on your way.
  //      note 1: But by way we implemented this function,
  //      note 1: if you want you can still declare the PATHINFO_*
  //      note 1: yourself, and then you can use:
  //      note 1: pathinfo('/www/index.html', PATHINFO_BASENAME | PATHINFO_EXTENSION);
  //      note 1: which makes it fully compliant with PHP syntax.
  //   example 1: pathinfo('/www/htdocs/index.html', 1)
  //   returns 1: '/www/htdocs'
  //   example 2: pathinfo('/www/htdocs/index.html', 'PATHINFO_BASENAME')
  //   returns 2: 'index.html'
  //   example 3: pathinfo('/www/htdocs/index.html', 'PATHINFO_EXTENSION')
  //   returns 3: 'html'
  //   example 4: pathinfo('/www/htdocs/index.html', 'PATHINFO_FILENAME')
  //   returns 4: 'index'
  //   example 5: pathinfo('/www/htdocs/index.html', 2 | 4)
  //   returns 5: {basename: 'index.html', extension: 'html'}
  //   example 6: pathinfo('/www/htdocs/index.html', 'PATHINFO_ALL')
  //   returns 6: {dirname: '/www/htdocs', basename: 'index.html', extension: 'html', filename: 'index'}
  //   example 7: pathinfo('/www/htdocs/index.html')
  //   returns 7: {dirname: '/www/htdocs', basename: 'index.html', extension: 'html', filename: 'index'}

  var basename = require('../filesystem/basename');
  var opt = '';
  var realOpt = '';
  var optName = '';
  var optTemp = 0;
  var tmpArr = {};
  var cnt = 0;
  var i = 0;
  var haveBasename = false;
  var haveExtension = false;
  var haveFilename = false;

  // Input defaulting & sanitation
  if (!path) {
    return false;
  }
  if (!options) {
    options = 'PATHINFO_ALL';
  }

  // Initialize binary arguments. Both the string & integer (constant) input is
  // allowed
  var OPTS = {
    'PATHINFO_DIRNAME': 1,
    'PATHINFO_BASENAME': 2,
    'PATHINFO_EXTENSION': 4,
    'PATHINFO_FILENAME': 8,
    'PATHINFO_ALL': 0
  };
  // PATHINFO_ALL sums up all previously defined PATHINFOs (could just pre-calculate)
  for (optName in OPTS) {
    if (OPTS.hasOwnProperty(optName)) {
      OPTS.PATHINFO_ALL = OPTS.PATHINFO_ALL | OPTS[optName];
    }
  }
  if (typeof options !== 'number') {
    // Allow for a single string or an array of string flags
    options = [].concat(options);
    for (i = 0; i < options.length; i++) {
      // Resolve string input to bitwise e.g. 'PATHINFO_EXTENSION' becomes 4
      if (OPTS[options[i]]) {
        optTemp = optTemp | OPTS[options[i]];
      }
    }
    options = optTemp;
  }

  // Internal Functions
  var _getExt = function _getExt(path) {
    var str = path + '';
    var dotP = str.lastIndexOf('.') + 1;
    return !dotP ? false : dotP !== str.length ? str.substr(dotP) : '';
  };

  // Gather path infos
  if (options & OPTS.PATHINFO_DIRNAME) {
    var dirName = path.replace(/\\/g, '/').replace(/\/[^/]*\/?$/, ''); // dirname
    tmpArr.dirname = dirName === path ? '.' : dirName;
  }

  if (options & OPTS.PATHINFO_BASENAME) {
    if (haveBasename === false) {
      haveBasename = basename(path);
    }
    tmpArr.basename = haveBasename;
  }

  if (options & OPTS.PATHINFO_EXTENSION) {
    if (haveBasename === false) {
      haveBasename = basename(path);
    }
    if (haveExtension === false) {
      haveExtension = _getExt(haveBasename);
    }
    if (haveExtension !== false) {
      tmpArr.extension = haveExtension;
    }
  }

  if (options & OPTS.PATHINFO_FILENAME) {
    if (haveBasename === false) {
      haveBasename = basename(path);
    }
    if (haveExtension === false) {
      haveExtension = _getExt(haveBasename);
    }
    if (haveFilename === false) {
      haveFilename = haveBasename.slice(0, haveBasename.length - (haveExtension ? haveExtension.length + 1 : haveExtension === false ? 0 : 1));
    }

    tmpArr.filename = haveFilename;
  }

  // If array contains only 1 element: return string
  cnt = 0;
  for (opt in tmpArr) {
    if (tmpArr.hasOwnProperty(opt)) {
      cnt++;
      realOpt = opt;
    }
  }
  if (cnt === 1) {
    return tmpArr[realOpt];
  }

  // Return full-blown array
  return tmpArr;
};
//# sourceMappingURL=pathinfo.js.map