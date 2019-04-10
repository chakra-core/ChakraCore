'use strict';
const _ = require('lodash');
const Generator = require('yeoman-generator');
const querystring = require('querystring');

module.exports = class extends Generator {
  constructor(args, options) {
    super(args, options);

    this.option('generateInto', {
      type: String,
      required: false,
      defaults: '',
      desc: 'Relocate the location of the generated files.'
    });

    this.option('name', {
      type: String,
      required: true,
      desc: 'Project name'
    });

    this.option('description', {
      type: String,
      required: true,
      desc: 'Project description'
    });

    this.option('githubAccount', {
      type: String,
      required: true,
      desc: 'GitHub username or organization'
    });

    this.option('repositoryName', {
      type: String,
      required: true,
      desc: 'Name of the GitHub repository'
    });

    this.option('authorName', {
      type: String,
      required: true,
      desc: 'Author name'
    });

    this.option('authorUrl', {
      type: String,
      required: true,
      desc: 'Author url'
    });

    this.option('coveralls', {
      type: Boolean,
      required: true,
      desc: 'Include coveralls badge'
    });

    this.option('content', {
      type: String,
      required: false,
      desc: 'Readme content'
    });
  }

  writing() {
    const pkg = this.fs.readJSON(
      this.destinationPath(this.options.generateInto, 'package.json'),
      {}
    );
    this.fs.copyTpl(
      this.templatePath('README.md'),
      this.destinationPath(this.options.generateInto, 'README.md'),
      {
        projectName: this.options.name,
        safeProjectName: _.camelCase(this.options.name),
        escapedProjectName: querystring.escape(this.options.name),
        repositoryName: this.options.repositoryName || this.options.name,
        description: this.options.description,
        githubAccount: this.options.githubAccount,
        author: {
          name: this.options.authorName,
          url: this.options.authorUrl
        },
        license: pkg.license,
        includeCoveralls: this.options.coveralls,
        content: this.options.content
      }
    );
  }
};
