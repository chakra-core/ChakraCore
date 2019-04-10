'use strict';
const _ = require('lodash');
const extend = _.merge;
const Generator = require('yeoman-generator');
const parseAuthor = require('parse-author');
const githubUsername = require('github-username');
const path = require('path');
const askName = require('inquirer-npm-name');
const chalk = require('chalk');
const validatePackageName = require('validate-npm-package-name');
const pkgJson = require('../../package.json');

module.exports = class extends Generator {
  constructor(args, options) {
    super(args, options);

    this.option('travis', {
      type: Boolean,
      required: false,
      default: true,
      desc: 'Include travis config'
    });

    this.option('boilerplate', {
      type: Boolean,
      required: false,
      default: true,
      desc: 'Include boilerplate files'
    });

    this.option('cli', {
      type: Boolean,
      required: false,
      default: false,
      desc: 'Add a CLI'
    });

    this.option('coveralls', {
      type: Boolean,
      required: false,
      desc: 'Include coveralls config'
    });

    this.option('editorconfig', {
      type: Boolean,
      required: false,
      default: true,
      desc: 'Include a .editorconfig file'
    });

    this.option('license', {
      type: Boolean,
      required: false,
      default: true,
      desc: 'Include a license'
    });

    this.option('name', {
      type: String,
      required: false,
      desc: 'Project name'
    });

    this.option('githubAccount', {
      type: String,
      required: false,
      desc: 'GitHub username or organization'
    });

    this.option('repositoryName', {
      type: String,
      required: false,
      desc: 'Name of the GitHub repository'
    });

    this.option('projectRoot', {
      type: String,
      required: false,
      default: 'lib',
      desc: 'Relative path to the project code root'
    });

    this.option('readme', {
      type: String,
      required: false,
      desc: 'Content to insert in the README.md file'
    });
  }

  initializing() {
    this.pkg = this.fs.readJSON(this.destinationPath('package.json'), {});

    // Pre set the default props from the information we have at this point
    this.props = {
      name: this.pkg.name,
      description: this.pkg.description,
      version: this.pkg.version,
      homepage: this.pkg.homepage,
      repositoryName: this.options.repositoryName
    };

    if (this.options.name) {
      const name = this.options.name;
      const packageNameValidity = validatePackageName(name);

      if (packageNameValidity.validForNewPackages) {
        this.props.name = name;
      } else {
        this.emit(
          'error',
          new Error(
            packageNameValidity.errors[0] ||
              'The name option is not a valid npm package name.'
          )
        );
      }
    }

    if (_.isObject(this.pkg.author)) {
      this.props.authorName = this.pkg.author.name;
      this.props.authorEmail = this.pkg.author.email;
      this.props.authorUrl = this.pkg.author.url;
    } else if (_.isString(this.pkg.author)) {
      const info = parseAuthor(this.pkg.author);
      this.props.authorName = info.name;
      this.props.authorEmail = info.email;
      this.props.authorUrl = info.url;
    }
  }

  _getModuleNameParts(name) {
    const moduleName = {
      name,
      repositoryName: this.props.repositoryName
    };

    if (moduleName.name.startsWith('@')) {
      const nameParts = moduleName.name.slice(1).split('/');

      Object.assign(moduleName, {
        scopeName: nameParts[0],
        localName: nameParts[1]
      });
    } else {
      moduleName.localName = moduleName.name;
    }

    if (!moduleName.repositoryName) {
      moduleName.repositoryName = moduleName.localName;
    }

    return moduleName;
  }

  _askForModuleName() {
    let askedName;

    if (this.props.name) {
      askedName = Promise.resolve({
        name: this.props.name
      });
    } else {
      askedName = askName(
        {
          name: 'name',
          default: path.basename(process.cwd())
        },
        this
      );
    }

    return askedName.then(answer => {
      const moduleNameParts = this._getModuleNameParts(answer.name);

      Object.assign(this.props, moduleNameParts);
    });
  }

  _askFor() {
    const prompts = [
      {
        name: 'description',
        message: 'Description',
        when: !this.props.description
      },
      {
        name: 'homepage',
        message: 'Project homepage url',
        when: !this.props.homepage
      },
      {
        name: 'authorName',
        message: "Author's Name",
        when: !this.props.authorName,
        default: this.user.git.name(),
        store: true
      },
      {
        name: 'authorEmail',
        message: "Author's Email",
        when: !this.props.authorEmail,
        default: this.user.git.email(),
        store: true
      },
      {
        name: 'authorUrl',
        message: "Author's Homepage",
        when: !this.props.authorUrl,
        store: true
      },
      {
        name: 'keywords',
        message: 'Package keywords (comma to split)',
        when: !this.pkg.keywords,
        filter(words) {
          return words.split(/\s*,\s*/g);
        }
      },
      {
        name: 'includeCoveralls',
        type: 'confirm',
        message: 'Send coverage reports to coveralls',
        when: this.options.coveralls === undefined
      }
    ];

    return this.prompt(prompts).then(props => {
      this.props = extend(this.props, props);
    });
  }

  _askForTravis() {
    const prompts = [
      {
        name: 'node',
        message: 'Enter Node versions (comma separated)',
        when: this.options.travis
      }
    ];

    return this.prompt(prompts).then(props => {
      this.props = extend(this.props, props);
    });
  }

  _askForGithubAccount() {
    if (this.options.githubAccount) {
      this.props.githubAccount = this.options.githubAccount;
      return Promise.resolve();
    }

    let usernamePromise;
    if (this.props.scopeName) {
      usernamePromise = Promise.resolve(this.props.scopeName);
    } else {
      usernamePromise = githubUsername(this.props.authorEmail).then(
        username => username,
        () => ''
      );
    }

    return usernamePromise.then(username => {
      return this.prompt({
        name: 'githubAccount',
        message: 'GitHub username or organization',
        default: username
      }).then(prompt => {
        this.props.githubAccount = prompt.githubAccount;
      });
    });
  }

  prompting() {
    return this._askForModuleName()
      .then(this._askFor.bind(this))
      .then(this._askForTravis.bind(this))
      .then(this._askForGithubAccount.bind(this));
  }

  writing() {
    // Re-read the content at this point because a composed generator might modify it.
    const currentPkg = this.fs.readJSON(this.destinationPath('package.json'), {});

    const pkg = extend(
      {
        name: this.props.name,
        version: '0.0.0',
        description: this.props.description,
        homepage: this.props.homepage,
        author: {
          name: this.props.authorName,
          email: this.props.authorEmail,
          url: this.props.authorUrl
        },
        files: [this.options.projectRoot],
        main: path.join(this.options.projectRoot, 'index.js').replace(/\\/g, '/'),
        keywords: [],
        devDependencies: {},
        engines: {
          npm: '>= 4.0.0'
        }
      },
      currentPkg
    );

    if (this.props.includeCoveralls) {
      pkg.devDependencies.coveralls = pkgJson.devDependencies.coveralls;
    }

    // Combine the keywords
    if (this.props.keywords && this.props.keywords.length) {
      pkg.keywords = _.uniq(this.props.keywords.concat(pkg.keywords));
    }

    // Let's extend package.json so we're not overwriting user previous fields
    this.fs.writeJSON(this.destinationPath('package.json'), pkg);
  }

  default() {
    if (this.options.travis) {
      let options = { config: {} };

      if (this.props.node) {
        // eslint-disable-next-line camelcase
        options.config.node_js = this.props.node.split(',');
      }

      if (this.props.includeCoveralls) {
        options.config.after_script = 'cat ./coverage/lcov.info | coveralls'; // eslint-disable-line camelcase
      }
      this.composeWith(require.resolve('generator-travis/generators/app'), options);
    }

    if (this.options.editorconfig) {
      this.composeWith(require.resolve('../editorconfig'));
    }

    this.composeWith(require.resolve('../eslint'));

    this.composeWith(require.resolve('../git'), {
      name: this.props.name,
      githubAccount: this.props.githubAccount,
      repositoryName: this.props.repositoryName
    });

    this.composeWith(require.resolve('generator-jest/generators/app'), {
      testEnvironment: 'node',
      coveralls: false
    });

    if (this.options.boilerplate) {
      this.composeWith(require.resolve('../boilerplate'), {
        name: this.props.name
      });
    }

    if (this.options.cli) {
      this.composeWith(require.resolve('../cli'));
    }

    if (this.options.license && !this.pkg.license) {
      this.composeWith(require.resolve('generator-license/app'), {
        name: this.props.authorName,
        email: this.props.authorEmail,
        website: this.props.authorUrl
      });
    }

    if (!this.fs.exists(this.destinationPath('README.md'))) {
      this.composeWith(require.resolve('../readme'), {
        name: this.props.name,
        description: this.props.description,
        githubAccount: this.props.githubAccount,
        repositoryName: this.props.repositoryName,
        authorName: this.props.authorName,
        authorUrl: this.props.authorUrl,
        coveralls: this.props.includeCoveralls,
        content: this.options.readme
      });
    }
  }

  installing() {
    this.npmInstall();
  }

  end() {
    this.log('Thanks for using Yeoman.');

    if (this.options.travis) {
      let travisUrl = chalk.cyan(
        `https://travis-ci.org/profile/${this.props.githubAccount || ''}`
      );
      this.log(`- Enable Travis integration at ${travisUrl}`);
    }

    if (this.props.includeCoveralls) {
      let coverallsUrl = chalk.cyan('https://coveralls.io/repos/new');
      this.log(`- Enable Coveralls integration at ${coverallsUrl}`);
    }
  }
};
