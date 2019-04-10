'use strict';
const _ = require('lodash');
const extend = _.merge;
const Generator = require('yeoman-generator');

module.exports = class extends Generator {
  constructor(args, options) {
    super(args, options);

    this.option('generateInto', {
      type: String,
      required: false,
      defaults: '',
      desc: 'Relocate the location of the generated files.'
    });
  }

  writing() {
    const pkg = this.fs.readJSON(
      this.destinationPath(this.options.generateInto, 'package.json'),
      {}
    );

    extend(pkg, {
      bin: 'lib/cli.js',
      dependencies: {
        meow: '^3.7.0'
      },
      devDependencies: {
        lec: '^1.0.1'
      },
      scripts: {
        prepare: 'lec lib/cli.js -c LF'
      }
    });

    this.fs.writeJSON(
      this.destinationPath(this.options.generateInto, 'package.json'),
      pkg
    );

    this.fs.copyTpl(
      this.templatePath('cli.js'),
      this.destinationPath(this.options.generateInto, 'lib/cli.js'),
      {
        pkgName: pkg.name,
        pkgSafeName: _.camelCase(pkg.name)
      }
    );
  }
};
