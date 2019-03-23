var recast = require('recast');

function isRoute(node, identifier) {
  var callee = node.expression.callee;
  return node.expression.type === 'CallExpression' &&
    callee.type === 'MemberExpression' &&
    (callee.property.name === 'route' || callee.property.name === identifier);
}

function isTopLevelExpression(path) {
  return path.scope === null;
}

module.exports = function findRoutes(name, routes, identifier) {
  var nodePaths = [];

  recast.visit(routes, {
    visitExpressionStatement: function(path) {
      var node = path.node;

      if (isRoute(node, identifier) &&
          isTopLevelExpression(path) &&
          node.expression.arguments[0].value === name) {
        nodePaths.push(path);

        return false;
      } else {
        this.traverse(path);
      }
    }
  });

  return nodePaths;
};
