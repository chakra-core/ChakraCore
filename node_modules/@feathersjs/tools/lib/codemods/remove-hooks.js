// Removes the require and configure for `feathers-hooks`
// only for `app.configure(hooks())`
module.exports = function transformer (file, api) {
  const j = api.jscodeshift;
  const ast = j(file.source);

  ast.find(j.Literal)
    .filter(node => node.value.value === 'feathers-hooks' &&
      node.parent.value.callee &&
      node.parent.value.callee.name === 'require'
    ).forEach(node => {
      const variableName = node.parentPath.parentPath.parentPath.value.id.name;

      ast.find(j.CallExpression)
        .filter(node => node.value.callee.name === variableName)
        .forEach(node => {
          const parent = node.parentPath.parentPath;

          if (parent.value.callee.object.type === 'Identifier' &&
            parent.parentPath.value.type === 'ExpressionStatement') {
            j(parent).remove();
          } else if (parent.value.callee.object.type === 'CallExpression') {
            j(parent).replaceWith(parent.value.callee.object);
          }
        });

      j(node.parentPath.parentPath.parentPath).remove();
    });

  ast.find(j.Literal).filter(node => node.value.value === 'feathers-hooks' &&
    node.parent.value.type === 'ImportDeclaration'
  ).forEach(node => j(node.parent).remove());

  return ast.toSource({ quote: 'single' });
};
