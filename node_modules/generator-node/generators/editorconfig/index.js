'use strict';
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

  initializing() {
    this.fs.copy(
      this.templatePath('editorconfig'),
      this.destinationPath(this.options.generateInto, '.editorconfig')
    );
  }
};
