'use strict';
var Generator = require('yeoman-generator');
var originUrl = require('git-remote-origin-url');

module.exports = class extends Generator {
  constructor(args, options) {
    super(args, options);

    this.option('name', {
      type: String,
      required: true,
      desc: 'Module name'
    });

    this.option('github-account', {
      type: String,
      required: true,
      desc: 'GitHub username or organization'
    });
    // Set a gitHub uRL if being passed
    this.githubUrl = this.options.githubUrl || "https://github.com";
  }

  initializing() {
    this.fs.copy(
      this.templatePath('gitattributes'),
      this.destinationPath('.gitattributes')
    );

    this.fs.copy(
      this.templatePath('gitignore'),
      this.destinationPath('.gitignore')
    );

    return originUrl()
      .then(function (url) {
        this.originUrl = url;
      }.bind(this), function () {
        this.originUrl = '';
      }.bind(this));
  }

  writing() {
    this.pkg = this.fs.readJSON(this.destinationPath('package.json'), {});

    var repository = '';
    if (this.originUrl) {
      repository = this.originUrl;
    } else {
      repository = this.options.githubAccount + '/' + this.options.name;
    }

    this.pkg.repository = this.pkg.repository || {};
    if (!this.pkg.repository.url) {
      this.pkg.repository.type = 'git';
      this.pkg.repository.url = repository;
    }

    //overwrite with given repo path if present
    if (this.options.githubUrl) {
      this.pkg.repository.url = [this.options.githubUrl, this.options.githubAccount, this.options.name].filter((x) => x).join("/");
    }
    this.fs.writeJSON(this.destinationPath('package.json'), this.pkg);
  }

  end() {
    this.spawnCommandSync('git', ['init'], {
      cwd: this.destinationPath()
    });

    // prioritize given repo URL if present
    if (this.options.githubUrl) {
      this.spawnCommandSync('git', ['remote', 'add', 'origin', this.pkg.repository.url], {
        cwd: this.destinationPath()
      });
    } else if (!this.originUrl) {
      var repoSSH = this.pkg.repository;
      var url = this.pkg.repository && this.pkg.repository.url;
      if (url && url.indexOf('.git') === -1) {
        repoSSH = 'git@github.com:' + this.pkg.repository.url + '.git';
      }
      this.spawnCommandSync('git', ['remote', 'add', 'origin', repoSSH], {
        cwd: this.destinationPath()
      });
    }
  }
};
