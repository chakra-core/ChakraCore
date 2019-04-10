'use strict';

const path = require('path');
const Macros = require('./utils/macros');
const normalizeOptions = require('./utils/normalize-options').normalizeOptions;

function macros(babel) {
  let t = babel.types;

  function buildIdentifier(value, name) {
    let replacement = t.booleanLiteral(value);

    // when we only support babel@7 we should change this
    // to `path.addComment` or `t.addComment`
    let comment = {
      type: 'CommentBlock',
      value: ` ${name} `,
      leading: false,
      trailing: true,
    };
    replacement.trailingComments = [comment];

    return replacement;
  }

  return {
    name: 'babel-feature-flags-and-debug-macros',
    visitor: {
      ImportSpecifier(path, state) {
        let importPath = path.parent.source.value;
        let flagsForImport = state.opts.flags[importPath];

        if (flagsForImport) {
          let flagName = path.node.imported.name;
          let localBindingName = path.node.local.name;

          if (!(flagName in flagsForImport)) {
            throw new Error(
              `Imported ${flagName} from ${importPath} which is not a supported flag.`
            );
          }

          let flagValue = flagsForImport[flagName];
          if (flagValue === null) {
            return;
          }

          let binding = path.scope.getBinding(localBindingName);

          binding.referencePaths.forEach(p => {
            p.replaceWith(buildIdentifier(flagValue, flagName));
          });

          path.remove();
          path.scope.removeOwnBinding(localBindingName);
        }
      },

      ImportDeclaration: {
        exit(path, state) {
          let importPath = path.node.source.value;
          let flagsForImport = state.opts.flags[importPath];

          // remove flag source imports when no specifiers are left
          if (flagsForImport && path.get('specifiers').length === 0) {
            path.remove();
          }
        },
      },

      Program: {
        enter(path, state) {
          state.opts = normalizeOptions(state.opts);
          this.macroBuilder = new Macros(babel, state.opts);

          let body = path.get('body');

          body.forEach(item => {
            if (item.isImportDeclaration()) {
              let importPath = item.node.source.value;

              let debugToolsImport = state.opts.debugTools.debugToolsImport;

              if (debugToolsImport && debugToolsImport === importPath) {
                if (!item.node.specifiers.length) {
                  item.remove();
                } else {
                  this.macroBuilder.collectDebugToolsSpecifiers(item.get('specifiers'));
                }
              }
            }
          });
        },

        exit(path) {
          this.macroBuilder.expand(path);
        },
      },

      ExpressionStatement(path) {
        this.macroBuilder.build(path);
      },
    },
  };
}

macros.baseDir = function() {
  return path.dirname(__dirname);
};

module.exports = macros;
