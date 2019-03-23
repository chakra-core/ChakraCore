'use strict';

const expect     = require('chai').expect;
const writeError = require('../../lib/write-error');
const MockUI     = require('../../lib/mock');
const BuildError = require('../helpers/build-error');
const EOL        = require('os').EOL;
const chalk      = require('chalk');

describe('writeError', function() {
  let ui;

  beforeEach(function() {
    ui = new MockUI();
  });

  it('no error', function() {
    let report = writeError(ui);

    expect(ui.output).to.equal('');
    expect(ui.errors).to.equal('');
    expect(report).to.equal(null);
  });

  it('error with message', function() {
    let report = writeError(ui, new BuildError({
      message: 'build error'
    }));

    expect(ui.output).to.equal('');
    expect(ui.errors).to.equal(chalk.red('build error') + EOL + EOL);

    expect(report).to.contain('ENV Summary:');
    expect(report).to.contain('ERROR Summary:');
    expect(report).to.contain('message: build error');
    expect(report).to.contain('stack: [undefined]');
    expect(report).to.contain('TIME: ');
  });

  it('error with stack', function() {
    ui.setWriteLevel('DEBUG');
    let report = writeError(ui, new BuildError({
      stack: 'the stack'
    }));

    expect(ui.output).to.equal('the stack' + EOL);
    expect(ui.errors).to.equal(chalk.red('Error') + EOL + EOL);
    expect(report).to.contain('message: [undefined]');
    expect(report).to.contain('stack: the stack');
  });

  it('error with file', function() {
    let report = writeError(ui, new BuildError({
      file: 'the file'
    }));

    expect(ui.output).to.equal('');
    expect(ui.errors).to.equal(chalk.red('File: the file') + EOL + EOL + chalk.red('Error') + EOL + EOL);

    expect(report).to.contain('file: the file');
  });

  it('error with code', function() {
    let error = new Error();
    error.code = 'the code';
    let report = writeError(ui, error);

    expect(ui.output).to.equal('');
    expect(ui.errors).to.contain('Error');

    expect(report).to.contain('code: the code');
  });

  it('error with filename (as from Uglify)', function() {
    let report = writeError(ui, new BuildError({
      filename: 'the file'
    }));

    expect(ui.output).to.equal('');
    expect(ui.errors).to.equal(chalk.red('File: the file') + EOL + EOL + chalk.red('Error') + EOL + EOL);
    expect(report).to.contain('file: the file');
  });

  it('error with file + line', function() {
    let report = writeError(ui, new BuildError({
      file: 'the file',
      line: 'the line'
    }));

    expect(ui.output).to.equal('');
    expect(ui.errors).to.equal(chalk.red('File: the file:the line') + EOL + EOL + chalk.red('Error') + EOL + EOL);
    expect(report).to.contain('file: the file');
    expect(report).to.contain('line: the line');
  });

  it('error with file + col', function() {
    let report = writeError(ui, new BuildError({
      file: 'the file',
      col: 'the col'
    }));

    expect(ui.output).to.equal('');
    expect(ui.errors).to.equal(chalk.red('File: the file') + EOL + EOL + chalk.red('Error') + EOL + EOL);

    expect(report).to.contain('file: the file');
    expect(report).to.contain('column: the col');
  });

  it('error with file + line + col', function() {
    let report = writeError(ui, new BuildError({
      file: 'the file',
      line: 'the line',
      col:  'the col'
    }));

    expect(ui.output).to.equal('');
    expect(ui.errors).to.equal(chalk.red('File: the file:the line:the col') + EOL + EOL + chalk.red('Error') + EOL + EOL);
    expect(report).to.contain('file: the file');
    expect(report).to.contain('line: the line');
    expect(report).to.contain('column: the col');
  });

  it('error title: file + line + column + errorType + nodeName', function() {
    let report = writeError(ui, new BuildError({
      file: 'index.js',
      line: '10',
      col:  '15',
      broccoliPayload: {
        broccoliNode: {
          nodeName: 'Babel'
        },
        error: {
          errorType: 'Compile Error'
        }
      },
    }));

    expect(ui.output).to.equal('');
    expect(ui.errors).to.equal(chalk.red('Compile Error (Babel) in index.js:10:15') + EOL + EOL + chalk.red('Error') + EOL + EOL);


    expect(report).to.contain('file: index.js');
    expect(report).to.contain('line: 10');
    expect(report).to.contain('column: 15');
    expect(report).to.contain('Babel');
    expect(report).to.contain('Compile Error');
  });

  it('error title: errorType + nodeName', function() {
    let report = writeError(ui, new BuildError({
      broccoliPayload: {
        broccoliNode: {
          nodeName: 'Babel'
        },
        error: {
          errorType: 'Compile Error'
        }
      },
    }));

    expect(ui.output).to.equal('');
    expect(ui.errors).to.equal(chalk.red('Compile Error (Babel)') + EOL + EOL + chalk.red('Error') + EOL + EOL);

    expect(report).to.contain('Babel');
    expect(report).to.contain('Compile Error');
  });

  it('error title: errorType', function() {
    let report = writeError(ui, new BuildError({
      broccoliPayload: {
        error: {
          errorType: 'Compile Error'
        }
      },
    }));

    expect(ui.output).to.equal('');
    expect(ui.errors).to.equal(chalk.red('Compile Error') + EOL + EOL + chalk.red('Error') + EOL + EOL);

    expect(report).to.contain('Compile Error');
  });

  it('error codeFrame', function() {
    let report = writeError(ui, new BuildError({
      broccoliPayload: {
        error: {
          errorType: 'Compile Error',
          codeFrame: 'codeFrame'
        }
      },
    }));

    expect(ui.output).to.equal('');
    expect(ui.errors).to.equal(
      chalk.red('Compile Error') + EOL + EOL +
      chalk.red('codeFrame') + EOL + EOL);

    expect(report).to.contain('Compile Error');
    expect(report).to.contain('codeFrame');
  });

  it('error codeFrame with same error message', function() {
    let report = writeError(ui, new BuildError({
      broccoliPayload: {
        error: {
          errorType: 'Compile Error',
          codeFrame: 'codeFrame',
          message: 'codeFrame'
        }
      },
    }));

    expect(ui.output).to.equal('');
    expect(ui.errors).to.equal(
      chalk.red('Compile Error') + EOL + EOL +
      chalk.red('codeFrame') + EOL + EOL);

    expect(report).to.contain('Compile Error');
    expect(report).to.contain('codeFrame');
  });

  it('error codeFrame with different error message', function() {
    let report = writeError(ui, new BuildError({
      broccoliPayload: {
        error: {
          errorType: 'Compile Error',
          codeFrame: 'codeFrame',
          message: 'broccoli error message'
        }
      },
    }));

    expect(ui.output).to.equal('');
    expect(ui.errors).to.equal(
      chalk.red('Compile Error') + EOL + EOL +
      chalk.red('broccoli error message') + EOL + EOL +
      chalk.red('codeFrame') + EOL + EOL);

    expect(report).to.contain('Compile Error');
    expect(report).to.contain('codeFrame');
    expect(report).to.contain('broccoli error message');
  });
});
