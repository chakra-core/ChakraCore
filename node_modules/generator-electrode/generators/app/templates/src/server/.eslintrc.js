var path = require("path");
var archetype = require("electrode-archetype-react-app/config/archetype");
var archetypeEslint = path.join(archetype.config.eslint, ".eslintrc-node");

function dotify(p) {
  return path.isAbsolute(p) ? p : "." + path.sep + p;
}

module.exports = {
  extends: dotify(path.relative(__dirname, archetypeEslint))
};
