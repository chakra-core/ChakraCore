'use strict';
const path = require('path');
const Generator = require('yeoman-generator');

module.exports = class extends Generator {
  configuring() {
    this.config.save();
  }

  writing() {
    this.fs.copy(
      this.templatePath('test.js'),
      this.destinationPath('test/spec/test.js')
    );

    this.fs.copy(
      this.templatePath('index.html'),
      this.destinationPath('test/index.html')
    );
  }

  install() {
    if (!this.options['skip-install']) {
      this.installDependencies({
        bower: false
      });
    }

    this.npmInstall([
      'jasmine'
    ], {saveDev: true});
  }
}
