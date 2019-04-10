'use strict';

const stew = require('broccoli-stew');

class AmdTransformAddon {
  /**
   * This addon is used to register a custom AMD transform for app and addons to use.
   *
   * @class AmdTransformAddon
   * @constructor
   */
  constructor(project) {
    this.project = project;
    this.name = 'amd-transform';
  }

  importTransforms() {
    return {
      'amd': {
        transform: (tree, options) => {
          let amdTransform = stew.map(tree, (content, relativePath) => {
            const name = options[relativePath].as;
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

          return amdTransform;
        },
        processOptions: (assetPath, entry, options) => {
          if (!entry.as) {
            throw new Error(`while importing ${assetPath}: amd transformation requires an \`as\` argument that specifies the desired module name`);
          }

          // If the import is specified to be a different name we must break because of the broccoli rewrite behavior.
          if (Object.keys(options).indexOf(assetPath) !== -1 && options[assetPath].as !== entry.as) {
            throw new Error(`Highlander error while importing ${assetPath}. You may not import an AMD transformed asset at different module names.`);
          }

          options[assetPath] = {
            as: entry.as,
          };

          return options;
        },
      },
    };
  }
}

module.exports = AmdTransformAddon;
