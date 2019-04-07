const path = require('path');
const fs = require('fs');

const pkgFile = path.join(process.cwd(), 'package.json');
const pkg = require(pkgFile);
const { devDependencies, scripts, semistandard } = pkg;

console.log(`Running in folder ${process.cwd()}`);

Object.keys(devDependencies).forEach(dependency => {
  if (dependency.indexOf('babel-') === 0 || dependency === 'rimraf' || dependency === 'eslint-if-supported') {
    console.log(`Removing dependency ${dependency}`);
    delete devDependencies[dependency];
  }
});

if (semistandard && semistandard.ignore) {
  delete semistandard.ignore;
}

delete scripts.compile;
delete scripts.watch;
delete scripts.prepublish;

pkg.scripts.test = 'npm run lint && npm run coverage';
pkg.scripts.lint = 'semistandard --fix';
pkg.engines.node = '>= 6';

fs.writeFileSync(pkgFile, JSON.stringify(pkg, null, '  '));

console.log('Updated package.json');
