// Removes all service .filters. requires
// and the `service.filter` call
module.exports = function transformer (file, api) {
  const j = api.jscodeshift;
  const ast = j(file.source);
  const filterRequire = /\.filters/;

  ast.find(j.Literal)
    .filter(node => filterRequire.test(node.value.value) &&
      node.parent.value.callee &&
      node.parent.value.callee.name === 'require'
    ).forEach(node =>
      j(node.parentPath.parentPath.parentPath).remove()
    );

  ast.find(j.IfStatement).filter(node => {
    const test = node.value.test;

    return test.type === 'MemberExpression' &&
      test.object.name === 'service' &&
      test.property.name === 'filter';
  }).forEach(node => j(node).remove());

  return ast.toSource({ quote: 'single' });
};
