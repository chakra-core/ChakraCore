'use strict';

// Creates a directory with the name directoryName in cwd and then sets cwd to
// this directory.

const RSVP = require('rsvp');
const fs = require('fs');
const Task = require('../models/task');
const SilentError = require('silent-error');

const Promise = RSVP.Promise;
const mkdir = RSVP.denodeify(fs.mkdir);

class CreateTask extends Task {
  // Options: String directoryName, Boolean: dryRun

  warnDirectoryAlreadyExists() {
    let message = `Directory '${this.directoryName}' already exists.`;
    return new SilentError(message);
  }

  run(options) {
    let directoryName = this.directoryName = options.directoryName;
    if (options.dryRun) {
      return new Promise((resolve, reject) => {
        if (fs.existsSync(directoryName) && fs.readdirSync(directoryName).length) {
          return reject(this.warnDirectoryAlreadyExists());
        }
        resolve();
      });
    }

    return mkdir(directoryName)
      .catch(err => {
        if (err.code === 'EEXIST') {
          // Allow using directory if it is empty.
          if (fs.readdirSync(directoryName).length) {
            throw this.warnDirectoryAlreadyExists();
          }
        } else {
          throw err;
        }
      })
      .then(() => {
        let cwd = process.cwd();
        process.chdir(directoryName);
        return { initialDirectory: cwd };
      });
  }
}

module.exports = CreateTask;
