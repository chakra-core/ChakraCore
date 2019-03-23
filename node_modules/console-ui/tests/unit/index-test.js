'use strict';

const expect = require('chai').expect;
const MockUI = require('../../lib/mock');
const EOL    = require('os').EOL;
const chalk  = require('chalk');
const fs     = require('fs');

describe('UI', function() {
  let ui;

  beforeEach(function() {
    ui = new MockUI();
  });

  describe('writeDebugLine', function() {
    it('does not write at the default level', function() {
      ui.writeDebugLine('foo');
      expect(ui.output).to.equal('');
    });

    it('writes in the correct chalk', function() {
      ui.writeLevel = 'DEBUG';
      ui.writeDebugLine('foo');
      expect(ui.output).to.equal(chalk.gray('foo') + EOL);
    });
  });

  describe('writeInfoLine', function() {
    it('writes in the correct chalk', function() {
      ui.writeInfoLine('foo');
      expect(ui.output).to.equal(chalk.cyan('foo') + EOL);
    });
  });

  describe('writeWarningLine', function() {
    it('does not write when the test is truthy', function() {
      ui.writeWarnLine('foo', true);
      expect(ui.output).to.equal('');
    });

    it('writes a prepended message when the test is falsy', function() {
      ui.writeWarnLine('foo', false);
      expect(ui.output).to.equal(chalk.yellow('WARNING: foo') + EOL);
    });

    it('writes an un-prepended message if prepend is false', function() {
      ui.writeWarnLine('foo', false, false);
      expect(ui.output).to.equal(chalk.yellow('foo') + EOL);
    });
  });

  describe('writeDeprecateLine', function() {
    it('does not write when the test is truthy', function() {
      ui.writeDeprecateLine('foo', true);
      expect(ui.output).to.equal('');
    });

    it('writes a prepended message when the test is falsy', function() {
      ui.writeDeprecateLine('foo', false);
      expect(ui.output).to.equal(chalk.yellow('DEPRECATION: foo') + EOL);
    });

    it('writes an un-prepended message if prepend is false', function() {
      ui.writeDeprecateLine('foo', false, false);
      expect(ui.output).to.equal(chalk.yellow('foo') + EOL);
    });
  });

  describe('prependLine', function() {
    it('prepends the data when prepend is undefined', function() {
      let result = ui.prependLine('foo', 'bar');
      expect(result).to.equal('foo: bar');
    });

    it('prepends the data when prepend is true', function() {
      let result = ui.prependLine('foo', 'bar', true);
      expect(result).to.equal('foo: bar');
    });

    it('returns the original data when prepend is falsy (but not undefined)', function() {
      let result = ui.prependLine('foo', 'bar', false);
      expect(result).to.equal('bar');
    });
  });

  describe('writeError', function() {
    let originalCI;

    beforeEach(function() {
      originalCI = process.env.CI;
      delete process.env.CI;
    });

    afterEach(function() {
      process.env.CI = originalCI;
    });

    function errorLogToReportPath(log) {
      return log.match(/([^\s]+error\.dump\.\w+\.log)/)[0];
    }

    it('empty error', function() {
      ui.writeError({});
      expect(ui.output).to.eql('');
      expect(ui.errors).to.contain('[object Object]');
      expect(ui.errors).to.contain('Stack Trace and Error Report');
      expect(ui.errors).to.contain('error\.dump\.');
      expect(ui.errorLog).to.deep.eql([]);

      const filepath = errorLogToReportPath(ui.errors);
      const report = fs.readFileSync(filepath, 'UTF8')

      expect(report).to.contain('ENV Summary:');
      expect(report).to.contain('ERROR Summary:');
    });

    it('real error', function() {
      ui.writeError(new Error('I AM ERROR MESSAGE'));
      expect(ui.output).to.eql('');
      expect(ui.errors).to.contain('I AM ERROR MESSAGE');
      expect(ui.errors).to.contain('Stack Trace and Error Report');
      expect(ui.errors).to.contain('error\.dump\.');
      expect(ui.errorLog).to.deep.eql([]);

      const filepath = errorLogToReportPath(ui.errors);
      const report = fs.readFileSync(filepath, 'UTF8')

      expect(report).to.contain('ENV Summary:');
      expect(report).to.contain('ERROR Summary:');
      expect(report).to.contain('I AM ERROR MESSAGE');
    });

    it('SilentError', function() {
      let e = new Error('I AM ERROR MESSAGE');
      e.isSilentError = true;
      ui.writeError(e);
      expect(ui.output).to.eql('');
      expect(ui.errors).to.contain('I AM ERROR MESSAGE');
      expect(ui.errors).to.not.contain('Stack Trace and Error Report');
      expect(ui.errors).to.not.contain('error\.dump\.');
      expect(ui.errorLog).to.deep.eql([]);
    });
  });
});
