'use strict';
const path = require('path');
const Generator = require('yeoman-generator');
const askName = require('inquirer-npm-name');
const _ = require('lodash');
const extend = require('deep-extend');
const mkdirp = require('mkdirp');

function makeGeneratorName(name) {
  name = _.kebabCase(name);
  name = name.indexOf('generator-') === 0 ? name : 'generator-' + name;
  return name;
}

module.exports = class extends Generator {
  initializing() {
    this.props = {};
  }

  prompting() {
    return askName({
      name: 'name',
      message: 'Your generator name',
      default: makeGeneratorName(path.basename(process.cwd())),
      filter: makeGeneratorName,
      validate: str => {
        return str.length > 'generator-'.length;
      }
    }, this).then(props => {
      this.props.name = props.name;
    });
  }

  default() {
    if (path.basename(this.destinationPath()) !== this.props.name) {
      this.log(
        'Your generator must be inside a folder named ' + this.props.name + '\n' +
        'I\'ll automatically create this folder.'
      );
      mkdirp(this.props.name);
      this.destinationRoot(this.destinationPath(this.props.name));
    }

    const readmeTpl = _.template(this.fs.read(this.templatePath('README.md')));

    this.composeWith(require.resolve('generator-node/generators/app'), {
      boilerplate: false,
      name: this.props.name,
      projectRoot: 'generators',
      skipInstall: this.options.skipInstall,
      readme: readmeTpl({
        generatorName: this.props.name,
        yoName: this.props.name.replace('generator-', '')
      })
    });

    this.composeWith(require.resolve('../subgenerator'), {
      arguments: ['app']
    });
  }

  writing() {
    const pkg = this.fs.readJSON(this.destinationPath('package.json'), {});
    const generatorGeneratorPkg = require('../package.json');

    extend(pkg, {
      dependencies: {
        'yeoman-generator': generatorGeneratorPkg.dependencies['yeoman-generator'],
        chalk: generatorGeneratorPkg.dependencies.chalk,
        yosay: generatorGeneratorPkg.dependencies.yosay
      },
      devDependencies: {
        'yeoman-test': generatorGeneratorPkg.devDependencies['yeoman-test'],
        'yeoman-assert': generatorGeneratorPkg.devDependencies['yeoman-assert']
      },
      jest: {
        testPathIgnorePatterns: ['templates']
      }
    });
    pkg.keywords = pkg.keywords || [];
    pkg.keywords.push('yeoman-generator');

    this.fs.writeJSON(this.destinationPath('package.json'), pkg);
  }

  conflicts() {
    this.fs.append(this.destinationPath('.eslintignore'), '**/templates\n');
  }

  install() {
    this.installDependencies({ bower: false });
  }
};
