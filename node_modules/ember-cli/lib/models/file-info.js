'use strict';

const fs = require('fs');
const RSVP = require('rsvp');
const chalk = require('chalk');
const EditFileDiff = require('./edit-file-diff');
const EOL = require('os').EOL;
const rxEOL = new RegExp(EOL, 'g');
const isBinaryFile = require('isbinaryfile').sync;
const canEdit = require('../utilities/open-editor').canEdit;
const processTemplate = require('../utilities/process-template');

const readFile = RSVP.denodeify(fs.readFile);
const lstat = RSVP.denodeify(fs.stat);

function diffHighlight(line) {
  if (line[0] === '+') {
    return chalk.green(line);
  } else if (line[0] === '-') {
    return chalk.red(line);
  } else if (/^@@/.test(line)) {
    return chalk.cyan(line);
  } else {
    return line;
  }
}

const NOOP = _ => _;
class FileInfo {
  constructor(options) {
    this.action = options.action;
    this.outputBasePath = options.outputBasePath;
    this.outputPath = options.outputPath;
    this.displayPath = options.displayPath;
    this.inputPath = options.inputPath;
    this.templateVariables = options.templateVariables;
    this.replacer = options.replacer || NOOP;
    this.ui = options.ui;
  }

  confirmOverwrite(path) {
    let promptOptions = {
      type: 'expand',
      name: 'answer',
      default: false,
      message: `${chalk.red('Overwrite')} ${path}?`,
      choices: [
        { key: 'y', name: 'Yes, overwrite', value: 'overwrite' },
        { key: 'n', name: 'No, skip', value: 'skip' },
      ],
    };

    let outputPathIsFile = false;
    try { outputPathIsFile = fs.statSync(this.outputPath).isFile(); } catch (err) { /* ignore */ }

    let canDiff = (
      !isBinaryFile(this.inputPath) && (
        !outputPathIsFile ||
        !isBinaryFile(this.outputPath)
      )
    );

    if (canDiff) {
      promptOptions.choices.push({ key: 'd', name: 'Diff', value: 'diff' });

      if (canEdit()) {
        promptOptions.choices.push({ key: 'e', name: 'Edit', value: 'edit' });
      }
    }

    return this.ui.prompt(promptOptions)
      .then(response => response.answer);
  }

  displayDiff() {
    let info = this,
        jsdiff = require('diff');
    return RSVP.hash({
      input: this.render(),
      output: readFile(info.outputPath),
    }).then(result => {
      let diff = jsdiff.createPatch(
        info.outputPath, result.output.toString().replace(rxEOL, '\n'), result.input.replace(rxEOL, '\n')
      );
      let lines = diff.split('\n');

      for (let i = 0; i < lines.length; i++) {
        info.ui.write(
          diffHighlight(lines[i] + EOL)
        );
      }
    });
  }

  render() {
    if (!this.rendered) {
      this.rendered = this._render().then(result => this.replacer(result, this));
    }

    return this.rendered;
  }

  _render() {
    let path = this.inputPath;
    let context = this.templateVariables;

    return readFile(path)
      .then(content => lstat(path)
        .then(fileStat => {
          if (isBinaryFile(content, fileStat.size)) {
            return content;
          } else {
            try {
              return processTemplate(content.toString(), context);
            } catch (err) {
              err.message += ` (Error in blueprint template: ${path})`;
              throw err;
            }
          }
        }));
  }

  checkForConflict() {
    return this.render().then(input => {
      input = input.toString().replace(rxEOL, '\n');

      return readFile(this.outputPath).then(output => {
        output = output.toString().replace(rxEOL, '\n');

        return input === output ? 'identical' : 'confirm';
      }).catch(e => {
        if (e.code === 'ENOENT') {
          return 'none';
        }

        throw e;
      });
    });
  }

  confirmOverwriteTask() {
    let info = this;

    return function() {
      function doConfirm() {
        return info.confirmOverwrite(info.displayPath)
          .then(action => {
            if (action === 'diff') {
              return info.displayDiff().then(doConfirm);
            } else if (action === 'edit') {
              let editFileDiff = new EditFileDiff({ info });
              return editFileDiff.edit()
                .then(() => info.action = action)
                .catch(() => doConfirm())
                .then(() => info);
            } else {
              info.action = action;
              return info;
            }
          });
      }

      return doConfirm();
    };
  }
}

module.exports = FileInfo;
