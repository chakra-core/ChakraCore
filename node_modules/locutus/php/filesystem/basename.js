'use strict';

module.exports = function basename(path, suffix) {
  //  discuss at: http://locutus.io/php/basename/
  // original by: Kevin van Zonneveld (http://kvz.io)
  // improved by: Ash Searle (http://hexmen.com/blog/)
  // improved by: Lincoln Ramsay
  // improved by: djmix
  // improved by: Dmitry Gorelenkov
  //   example 1: basename('/www/site/home.htm', '.htm')
  //   returns 1: 'home'
  //   example 2: basename('ecra.php?p=1')
  //   returns 2: 'ecra.php?p=1'
  //   example 3: basename('/some/path/')
  //   returns 3: 'path'
  //   example 4: basename('/some/path_ext.ext/','.ext')
  //   returns 4: 'path_ext'

  var b = path;
  var lastChar = b.charAt(b.length - 1);

  if (lastChar === '/' || lastChar === '\\') {
    b = b.slice(0, -1);
  }

  b = b.replace(/^.*[/\\]/g, '');

  if (typeof suffix === 'string' && b.substr(b.length - suffix.length) === suffix) {
    b = b.substr(0, b.length - suffix.length);
  }

  return b;
};
//# sourceMappingURL=basename.js.map