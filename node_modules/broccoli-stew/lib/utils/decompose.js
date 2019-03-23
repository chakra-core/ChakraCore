'use strict';

const Minimatch = require('minimatch').Minimatch;
const ensurePosixPath = require('ensure-posix-path');

function isExpansion(entry) {
  return /[\{\}]/.test(entry);
}

module.exports = function(_path) {
  let path = ensurePosixPath(_path);
  let match = new Minimatch(path);

  let set = match.set;
  let position = 0;
  let root = [], include = [];
  let matcher = false;

  path.split('/').forEach((entry, index) => {
    matcher = matcher || typeof set[0][index]!== 'string';

    if (!matcher) {
      let prev = set[0][index];
      if (isExpansion(prev)) {
        matcher = true;
      } else {
        let x;
        for (let i = 0; i < set.length; i++) {
          x = set[i][index];
          if (prev !== x || isExpansion(entry)) {
            matcher = true;
            break;
          }
        }
      }
    }

    if (index === 0 && matcher) {
      throw Error("top level glob or expansion not currently supported: `" + path + "`");
    } else if (matcher) {
      include.push(expandSingle(entry));
    } else {
      root.push(entry);
    }
  });


  let _include = [include.join('/')].filter(Boolean);

  return {
    root:  root.join('/'),
    include: _include.length > 0 ? _include : undefined,
    exclude: undefined
  };
};

function expandSingle(maybeSingle) {
  return ensurePosixPath(maybeSingle).replace(/\{([^,\}]+)\}/g,'$1');
}

module.exports.expandSingle = expandSingle;
