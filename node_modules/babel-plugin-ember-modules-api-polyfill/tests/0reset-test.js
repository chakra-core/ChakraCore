'use strict';

// work around https://github.com/qunitjs/qunit/issues/1182
const root = process.cwd();
for (let key in require.cache) {
  if (key.startsWith(root) && !key.startsWith(`${root}/node_modules`)) {
    delete require.cache[key];
  }
}
