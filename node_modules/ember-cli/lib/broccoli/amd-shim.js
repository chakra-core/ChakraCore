'use strict';
const stew = require('broccoli-stew');

module.exports = function shimAmd(tree, nameMapping) {
  return stew.map(tree, (content, relativePath) => {
    let name = nameMapping[relativePath];
    if (name) {
      return [
        '(function(define){\n',
        content,
        '\n})((function(){ function newDefine(){ var args = Array.prototype.slice.call(arguments); args.unshift("',
        name,
        '"); return define.apply(null, args); }; newDefine.amd = true; return newDefine; })());',
      ].join('');
    } else {
      return content;
    }
  });
};
