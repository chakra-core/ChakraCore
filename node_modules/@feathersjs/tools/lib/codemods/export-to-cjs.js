// Replaces all `export function x() {}` and `export const x`
// With the equivalent `exports.x =`
module.exports = function transformer (file, api) {
  const j = api.jscodeshift;
  const ast = j(file.source);
  const isFunction = node => node.type === 'FunctionExpression' ||
    node.type === 'FunctionDeclaration';

  ast.find(j.ExportDefaultDeclaration)
    .forEach(path => {
      // export default
      const declaration = path.value.declaration;
      let right = declaration;
      const exports = j.memberExpression(
        j.identifier('module'), j.identifier('exports')
      );

      if (isFunction(declaration)) {
        right = j.functionExpression(declaration.id, declaration.params, declaration.body);
      }

      if (declaration.type === 'ClassDeclaration') {
        const { name } = declaration.id;

        right = j.classExpression(j.identifier(name), declaration.body);
      }

      const expr = j.expressionStatement(
        j.assignmentExpression('=', exports, right)
      );

      j(path).replaceWith(expr);
    });

  ast.find(j.ExportNamedDeclaration)
    .forEach(path => {
      const {
        value
      } = path;

      if (isFunction(value.declaration)) {
        // export function()
        const name = value.declaration.id.name;
        const fn = j.functionExpression(
          j.identifier(name),
          value.declaration.params,
          value.declaration.body
        );
        const exports = j.memberExpression(
          j.identifier('exports'), j.identifier(name)
        );
        const expr = j.expressionStatement(
          j.assignmentExpression('=', exports, fn)
        );

        j(path).replaceWith(expr);
      } else if (value.declaration.type === 'ClassDeclaration') {
        // export class Test {}
        const name = j.identifier(value.declaration.id.name);
        const me = j.memberExpression(j.identifier('exports'), name);
        const ce = j.classExpression(name, value.declaration.body);

        const expr = j.expressionStatement(
          j.assignmentExpression('=', me, ce)
        );

        j(path).replaceWith(expr);
      } else if (value.declaration.declarations) {
        // export const anything
        const dec = value.declaration.declarations[0];
        const name = dec.id.name;
        const me = j.memberExpression(j.identifier('exports'), j.identifier(name));

        const expr = j.expressionStatement(
          j.assignmentExpression('=', me, dec.init)
        );

        j(path).replaceWith(expr);
      }
    });

  return ast.toSource({ quote: 'single' });
};
