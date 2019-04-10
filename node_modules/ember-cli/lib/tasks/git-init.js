'use strict';

const RSVP = require('rsvp');
const Task = require('../models/task');
const path = require('path');
const pkg = require('../../package.json');
const fs = require('fs');
const execa = require('../utilities/execa');

const Promise = RSVP.Promise;

module.exports = class GitInitTask extends Task {

  run(_commandOptions) {
    let commandOptions = _commandOptions || {};
    const chalk = require('chalk');
    let ui = this.ui;

    if (commandOptions.skipGit) {
      return Promise.resolve();
    }

    return this._gitVersion()
      .then(() => true, () => false)
      .then(hasGit => {
        if (hasGit) {
          return this._gitInit()
            .then(() => this._gitAdd())
            .then(() => this._gitCommit())
            .then(() => ui.writeLine(chalk.green('Successfully initialized git.')));
        }
      });
  }

  _gitVersion() {
    return execa('git', ['--version']);
  }

  _gitInit() {
    return execa('git', ['init']);
  }

  _gitAdd() {
    return execa('git', ['add', '.']);
  }

  _gitCommit() {
    const template = require('lodash.template');
    let commitTemplate = fs.readFileSync(path.join(__dirname, '../utilities/COMMIT_MESSAGE.txt'));
    let commitMessage = template(commitTemplate)(pkg);
    let env = this.buildGitEnvironment();

    return execa('git', ['commit', '-m', commitMessage], { env })
      .catch(error => {
        if (isError(error) && error.message.indexOf('git config --global user') > -1) {
          env.GIT_COMMITTER_NAME = 'Tomster';
          env.GIT_COMMITTER_EMAIL = 'tomster@emberjs.com';
          return execa('git', ['commit', '-m', commitMessage], { env });
        }

        throw error;
      });
  }

  buildGitEnvironment() {
    // Make sure we merge in the current environment so that git has access to
    // important environment variables like $HOME.
    return Object.assign({}, process.env, {
      GIT_AUTHOR_NAME: 'Tomster',
      GIT_AUTHOR_EMAIL: 'tomster@emberjs.com',
    });
  }
};

function isError(error) {
  return typeof error === 'object' && error !== null && typeof error.message === 'string';
}
