'use strict';
const summarizeProcess = require('../../lib/summarize-process');
const obj = summarizeProcess.obj;
const expect = require('chai').expect;

describe('summarizeProcess', function() {
  it('acceptance', function() {
    const summary = summarizeProcess(process).split(/\n/);

    expect(summary.shift()).to.match(/^$/);
    expect(summary.shift()).to.match(/^  TIME: (.*)$/);
    expect(summary.shift()).to.match(/^  TITLE: (.*)$/);
    expect(summary.shift()).to.match(/^  ARGV:$/);

    for (let x = 0 ; x < process.argv.length; x++) {
      expect(summary.shift()).to.match(/^  -.*/);
    }

    expect(summary.shift()).to.match(/^  EXEC_PATH: (.*)$/);
    expect(summary.shift()).to.match(/^  TMPDIR: (.*)$/);
    expect(summary.shift()).to.match(/^  SHELL: (.*)$/);
    expect(summary.shift()).to.match(/^  PATH:$/);

    for (let x = 0 ; x < process.env.PATH.split(':').length; x++) {
      expect(summary.shift()).to.match(/^  -.*/);
    }

    expect(summary.shift()).to.match(/^  PLATFORM: (.*)\s(.*)$/);
    expect(summary.shift()).to.match(/^  FREEMEM: \d+$/);
    expect(summary.shift()).to.match(/^  TOTALMEM: \d+$/);
    expect(summary.shift()).to.match(/^  UPTIME: \d+(\.\d+)?$/);
    expect(summary.shift()).to.match(/^  LOADAVG: \d+(\.\d+)?,\d+(\.\d+)?,\d+(\.\d+)?$/);
    expect(summary.shift()).to.match(/^  CPUS:$/);

    for (let i = 0; i < require('os').cpus().length; i++) {
      expect(summary.shift()).to.match(/^  - (.*)$/);
    }

    expect(summary.shift()).to.match(/^  ENDIANNESS: (.*)$/);
    expect(summary.shift()).to.match(/^  VERSIONS:$/);

    for (let x = 0 ; x < Object.keys(process.versions).length; x++) {
      expect(summary.shift()).to.match(/^  -.*/);
    }
    expect(summary.shift()).to.match(/^$/);
    expect(summary).to.be.empty;
  });

  it('handles an unexpected empty process', function() {
    const process = {};
    const summary = summarizeProcess(process).split(/\n/);
    // doesn't crash
    //
    // has expected: "shape"
    expect(summary.shift()).to.match(/^$/);
    expect(summary.shift()).to.match(/^  TIME: (.*)$/);
    expect(summary.shift()).to.match(/^  TITLE: $/);
    expect(summary.shift()).to.match(/^  ARGV:$/);

    expect(summary.shift()).to.match(/^  EXEC_PATH: $/);
    expect(summary.shift()).to.match(/^  TMPDIR: (.*)$/);
    expect(summary.shift()).to.match(/^  SHELL: (.*)$/);
    expect(summary.shift()).to.match(/^  PATH:$/);

    expect(summary.shift()).to.match(/^  PLATFORM:  $/);
    expect(summary.shift()).to.match(/^  FREEMEM: \d+$/);
    expect(summary.shift()).to.match(/^  TOTALMEM: \d+$/);
    expect(summary.shift()).to.match(/^  UPTIME: \d+(\.\d+)?$/);
    expect(summary.shift()).to.match(/^  LOADAVG: \d+(\.\d+)?,\d+(\.\d+)?,\d+(\.\d+)?$/);
    expect(summary.shift()).to.match(/^  CPUS:$/);

    for (let i = 0; i < require('os').cpus().length; i++) {
      expect(summary.shift()).to.match(/^  - (.*)$/);
    }

    expect(summary.shift()).to.match(/^  ENDIANNESS: (.*)$/);
    expect(summary.shift()).to.match(/^  VERSIONS:$/);
    expect(summary.shift()).to.match(/^$/);
    expect(summary).to.be.empty;
  });

  it('handles arch + platform', function() {
    const process = {};
    const summary = summarizeProcess({ arch: 'the-arc', platform: 'the-plat'}).split(/\n/);
    // doesn't crash
    //
    // has expected: "shape"
    expect(summary.shift()).to.match(/^$/);
    expect(summary.shift()).to.match(/^  TIME: (.*)$/);
    expect(summary.shift()).to.match(/^  TITLE: $/);
    expect(summary.shift()).to.match(/^  ARGV:$/);

    expect(summary.shift()).to.match(/^  EXEC_PATH: $/);
    expect(summary.shift()).to.match(/^  TMPDIR: (.*)$/);
    expect(summary.shift()).to.match(/^  SHELL: (.*)$/);
    expect(summary.shift()).to.match(/^  PATH:$/);

    expect(summary.shift()).to.match(/^  PLATFORM: the-plat the-arc$/);
    expect(summary.shift()).to.match(/^  FREEMEM: \d+$/);
    expect(summary.shift()).to.match(/^  TOTALMEM: \d+$/);
    expect(summary.shift()).to.match(/^  UPTIME: \d+(\.\d+)?$/);
    expect(summary.shift()).to.match(/^  LOADAVG: \d+(\.\d+)?,\d+(\.\d+)?,\d+(\.\d+)?$/);
    expect(summary.shift()).to.match(/^  CPUS:$/);

    for (let i = 0; i < require('os').cpus().length; i++) {
      expect(summary.shift()).to.match(/^  - (.*)$/);
    }

    expect(summary.shift()).to.match(/^  ENDIANNESS: (.*)$/);
    expect(summary.shift()).to.match(/^  VERSIONS:$/);
    expect(summary.shift()).to.match(/^$/);
    expect(summary).to.be.empty;
  });

  it('handles versions', function() {
    const process = {};
    const summary = summarizeProcess({
      versions: {
        o: 1,
        my: 2,
        gosh: 3,
      }
    }).split(/\n/);

    expect(summary.shift()).to.match(/^$/);
    expect(summary.shift()).to.match(/^  TIME: (.*)$/);
    expect(summary.shift()).to.match(/^  TITLE: $/);
    expect(summary.shift()).to.match(/^  ARGV:$/);

    expect(summary.shift()).to.match(/^  EXEC_PATH: $/);
    expect(summary.shift()).to.match(/^  TMPDIR: (.*)$/);
    expect(summary.shift()).to.match(/^  SHELL: (.*)$/);
    expect(summary.shift()).to.match(/^  PATH:$/);

    expect(summary.shift()).to.match(/^  PLATFORM:  $/);

    expect(summary.shift()).to.match(/^  FREEMEM: \d+$/);
    expect(summary.shift()).to.match(/^  TOTALMEM: \d+$/);
    expect(summary.shift()).to.match(/^  UPTIME: \d+(\.\d+)?$/);
    expect(summary.shift()).to.match(/^  LOADAVG: \d+(\.\d+)?,\d+(\.\d+)?,\d+(\.\d+)?$/);
    expect(summary.shift()).to.match(/^  CPUS:$/);

    for (let i = 0; i < require('os').cpus().length; i++) {
      expect(summary.shift()).to.match(/^  - (.*)$/);
    }

    expect(summary.shift()).to.match(/^  ENDIANNESS: (.*)$/);

    expect(summary.shift()).to.match(/^  VERSIONS:$/);
    expect(summary.shift()).to.match(/^  - gosh: 3$/);
    expect(summary.shift()).to.match(/^  - my: 2$/);
    expect(summary.shift()).to.match(/^  - o: 1$/);
    expect(summary.shift()).to.match(/^$/);
    expect(summary).to.be.empty;
  });

  it('handles PATH', function() {
    const process = {};
    const summary = summarizeProcess({
      env: {
        PATH: 'one:two:three'
      }
    }).split(/\n/);

    expect(summary.shift()).to.match(/^$/);
    expect(summary.shift()).to.match(/^  TIME: (.*)$/);
    expect(summary.shift()).to.match(/^  TITLE: $/);
    expect(summary.shift()).to.match(/^  ARGV:$/);

    expect(summary.shift()).to.match(/^  EXEC_PATH: $/);
    expect(summary.shift()).to.match(/^  TMPDIR: (.*)$/);
    expect(summary.shift()).to.match(/^  SHELL: (.*)$/);
    expect(summary.shift()).to.match(/^  PATH:$/);
    expect(summary.shift()).to.match(/^  - one$/);
    expect(summary.shift()).to.match(/^  - two$/);
    expect(summary.shift()).to.match(/^  - three$/);

    expect(summary.shift()).to.match(/^  PLATFORM:  $/);
    expect(summary.shift()).to.match(/^  FREEMEM: \d+$/);
    expect(summary.shift()).to.match(/^  TOTALMEM: \d+$/);
    expect(summary.shift()).to.match(/^  UPTIME: \d+(\.\d+)?$/);
    expect(summary.shift()).to.match(/^  LOADAVG: \d+(\.\d+)?,\d+(\.\d+)?,\d+(\.\d+)?$/);
    expect(summary.shift()).to.match(/^  CPUS:$/);


    for (let i = 0; i < require('os').cpus().length; i++) {
      expect(summary.shift()).to.match(/^  - (.*)$/);
    }

    expect(summary.shift()).to.match(/^  ENDIANNESS: (.*)$/);

    expect(summary.shift()).to.match(/^  VERSIONS:$/);
    expect(summary.shift()).to.match(/^$/);
    expect(summary).to.be.empty;
  });

  describe('obj', function() {
    it('nested obj', function() {
      expect(obj({
        nested: {
          obj: {
            obj: {
              obj: {
                obj: 2
              }
            }
          }
        }
      })).to.eql(`
  - nested:
    - obj:
      - obj:
        - obj:
          - obj: 2`
      );
    });

    it('nested array', function() {
      expect(obj({
        obj: {
          avalue: [1,2,3, { foo: 1}],
          bnested: {
            deeper: [1,2,3, { foo: 1}],
          },
        }
      })).to.eql(`
  - obj:
    - avalue:
      - 1
      - 2
      - 3
      - foo: 1
    - bnested:
      - deeper:
        - 1
        - 2
        - 3
        - foo: 1`);
    });

    it('complex', function() {
      expect(obj({
        ARGV: [1,2,3, {}, null],
        ENV: null,
        title: 'hi',
        PATH: 'hi:hi:hi',
        nested: {
          obj: {
            obj: 2,
          }
        }
      })).to.eql(`
  - ARGV:
    - 1
    - 2
    - 3
    - { }
    - [null]
  - ENV: [null]
  - PATH: hi:hi:hi
  - nested:
    - obj:
      - obj: 2
  - title: hi`
      );
    });
  });
});

