module.exports = function transformer (file, api) {
  const j = api.jscodeshift;
  const ast = j(file.source);
  const { statement } = j.template;
  const expressImport = statement`const express = require('@feathersjs/express');`;
  const expressImports = ast.find(j.Literal)
    .filter(node => /feathersjs\/express/.test(node.value.value));

  if (expressImports.length > 0) {
    ast.find(j.Literal)
      .filter(node => node.value.value === '@feathersjs/feathers' &&
        node.parent.value.callee &&
        node.parent.value.callee.name === 'require'
      ).forEach(node => {
        const declarator = j(node).closest(j.VariableDeclarator);
        const varName = declarator.get().value.id.name;
        const needsImport = ast.find(j.Literal)
          .filter(node => node.value.value === '@feathersjs/express')
          .length === 0;

        if (needsImport) {
          declarator.closest(j.VariableDeclaration)
            .insertAfter(expressImport);
        }

        ast.find(j.MemberExpression).filter(node => {
          const { object, property } = node.value;

          return object.name === 'feathers' && property.name === 'static';
        }).forEach(node => {
          node.value.object.name = 'express';
        });

        ast.find(j.CallExpression).filter(node =>
          node.value.callee.name === varName
        ).forEach(node => {
          j(node).replaceWith(
            j.callExpression(
              j.identifier('express'), [node.value]
            )
          );
        });
      });
  }

  return ast.toSource({ quote: 'single' });
};
