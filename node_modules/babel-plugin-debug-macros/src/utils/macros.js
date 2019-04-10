'use strict';

const Builder = require('./builder');

const SUPPORTED_MACROS = ['assert', 'deprecate', 'warn', 'log'];

module.exports = class Macros {
  constructor(babel, options) {
    this.babel = babel;
    this.localDebugBindings = [];

    this.debugHelpers = options.externalizeHelpers || {};
    this.builder = new Builder(babel.types, {
      module: this.debugHelpers.module,
      global: this.debugHelpers.global,
      assertPredicateIndex: options.debugTools.assertPredicateIndex,
      isDebug: options.debugTools.isDebug,
    });
  }

  /**
   * Injects the either the env-flags module with the debug binding or
   * adds the debug binding if missing from the env-flags module.
   */
  expand(path) {
    this.builder.expandMacros();

    this._cleanImports(path);
  }

  /**
   * Collects the import bindings for the debug tools.
   */
  collectDebugToolsSpecifiers(specifiers) {
    specifiers.forEach(specifier => {
      if (specifier.node.imported && SUPPORTED_MACROS.indexOf(specifier.node.imported.name) > -1) {
        this.localDebugBindings.push(specifier.get('local'));
      }
    });
  }

  /**
   * Builds the expressions that the CallExpression will expand into.
   */
  build(path) {
    let expression = path.node.expression;

    if (
      this.builder.t.isCallExpression(expression) &&
      this.localDebugBindings.some(b => b.node.name === expression.callee.name)
    ) {
      let imported = path.scope.getBinding(expression.callee.name).path.node.imported.name;
      this.builder[`${imported}`](path);
    }
  }

  _cleanImports() {
    if (!this.debugHelpers.module) {
      if (this.localDebugBindings.length > 0) {
        this.localDebugBindings[0].parentPath.parentPath;
        let importPath = this.localDebugBindings[0].findParent(p => p.isImportDeclaration());
        let specifiers = importPath.get('specifiers');

        if (specifiers.length === this.localDebugBindings.length) {
          this.localDebugBindings[0].parentPath.parentPath.remove();
        } else {
          this.localDebugBindings.forEach(binding => binding.parentPath.remove());
        }
      }
    }
  }
};
