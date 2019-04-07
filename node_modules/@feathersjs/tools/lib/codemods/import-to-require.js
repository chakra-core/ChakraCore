// Press ctrl+space for code completion
module.exports = function transformer (file, api) {
  const j = api.jscodeshift;
  const ast = j(file.source);

  ast.find(j.ImportDeclaration)
    .forEach(path => {
      const val = path.value;
      const req = j.callExpression(j.identifier('require'), [
        j.literal(val.source.value)
      ]);

      const specs = val.specifiers;

      if (specs.length >= 1 &&
        (specs[0].type === 'ImportDefaultSpecifier' ||
        specs[0].type === 'ImportNamespaceSpecifier')
      ) {
        const name = j.identifier(specs[0].local.name);
        const res = j.variableDeclaration('const', [
          j.variableDeclarator(name, req)
        ]);

        j(path).replaceWith(res);
      } else {
        const pattern = j.objectPattern(specs.map(spec => {
          const id = j.identifier(spec.local.name);
          const result = j.property('init', id, id);

          result.shorthand = true;

          return result;
        }));
        const res = j.variableDeclaration('const', [
          j.variableDeclarator(pattern, req)
        ]);

        j(path).replaceWith(res);
      }
    });

  return ast.toSource({ quote: 'single' });
};
