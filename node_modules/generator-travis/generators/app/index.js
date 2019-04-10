'use strict';
var Generator = require('yeoman-generator');
var yaml = require('yamljs');
var sort = require('sort-object');
var mergeAndConcat = require('merge-and-concat');
var travisConfigKeys = require('travis-config-keys');
var ramda = require('ramda');

function sortByKeys(a, b) {
  return travisConfigKeys.indexOf(a) < travisConfigKeys.indexOf(b) ? -1 : 1;
}

module.exports = class extends Generator {
  constructor(args, opts) {
    super(args, opts);

    this.option('generateInto', {
      type: String,
      required: false,
      defaults: '',
      desc: 'Relocate the location of the generated files.',
    });
  }

  writing() {
    var optional = this.options.config || {};
    var existing = this.fs.exists(
      this.destinationPath(this.options.generateInto, '.travis.yml')
    )
      ? yaml.parse(
          this.fs.read(
            this.destinationPath(this.options.generateInto, '.travis.yml')
          )
        )
      : {};
    var defaults = yaml.parse(this.fs.read(this.templatePath('travisyml')));
    var results = mergeAndConcat(existing, optional, defaults);
    var sortedResults = sort(results, { sort: sortByKeys });
    sortedResults.node_js = ramda.uniq(sortedResults.node_js);
    this.fs.write(
      this.destinationPath(this.options.generateInto, '.travis.yml'),
      yaml.stringify(sortedResults, 3, 2)
    );
  }
};
