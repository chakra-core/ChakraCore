'use strict';

const execa = require('execa');
const log = require('npmlog');
const Bluebird = require('bluebird');
const EventEmitter = require('events').EventEmitter;

const isWin = require('./is-win')();

module.exports = class Process extends EventEmitter {
  constructor(name, options, process) {
    super();

    this.name = name;
    this.killTimeout = options.killTimeout;
    this.process = process;
    this.stdout = '';
    this.stderr = '';
    this.process.stdout.on('data', chunk => {
      this.stdout += chunk;
      this.emit('out');
    });
    this.process.stderr.on('data', chunk => {
      if (chunk.indexOf('[warn] kevent: Invalid argument') >= 0) {
        return;
      }
      this.stderr += chunk;
    });

    this.process.on('close', this.onClose.bind(this));
    this.process.on('error', this.onError.bind(this));
  }

  onKillTimeout() {
    log.warn('Process ' + this.name + ' not terminated in ' + this.killTimeout + 'ms.');
    kill(this.process, 'SIGKILL');
  }

  onClose(code) {
    if (!this.process) {
      return;
    }
    log.warn(this.name + ' closed', code);
    this.process = null;
    this.exitCode = code;
    this.emit('processExit', code, this.stdout, this.stderr);
  }

  onError(error) {
    log.warn(this.name + ' errored', error);
    this.process = null;
    this.exitCode = 1;
    this.emit('processError', error, this.stdout, this.stderr);
  }

  onStdOut(pattern, fn, timeout) {
    let self = this;
    let timeoutID;

    let listener = () => {
      if (self.patternMatches(pattern)) {
        if (timeoutID) {
          clearTimeout(timeoutID);
        }
        return fn(null, self.stdout, self.stderr);
      }
    };

    this.on('out', listener);

    if (timeout) {
      timeoutID = setTimeout(() => {
        self.removeListener('out', listener);
        return fn(new Error('Timed out without seeing "' + pattern + '"'), self.stdout, self.stderr);
      }, timeout);
    }
  }

  patternMatches(pattern) {
    if (typeof pattern === 'string') {
      return this.stdout.indexOf(pattern) !== -1;
    } else { // regex
      return !!this.stdout.match(pattern);
    }
  }

  kill(sig) {
    if (!this.process) {
      log.info('Process ' + this.name + ' already killed.');

      return Bluebird.resolve(this.exitCode);
    }

    sig = sig || 'SIGTERM';

    let self = this;

    return new Bluebird.Promise(resolve => {
      self.process.once('close', (code, sig) => {
        self.process = null;
        if (self._killTimer) {
          clearTimeout(self._killTimer);
          self._killTimer = null;
        }
        log.info('Process ' + self.name + ' terminated.', code, sig);

        resolve(code);
      });
      self.process.on('error', err => {
        log.error('Error killing process ' + self.name + '.', err);
      });
      self._killTimer = setTimeout(self.onKillTimeout.bind(self), self.killTimeout);
      kill(self.process, sig);
    });
  }
};

// Kill process and all child processes cross platform
function kill(p, sig) {
  if (isWin) {
    let command = 'taskkill.exe';
    let args = ['/t', '/pid', p.pid];
    if (sig === 'SIGKILL') {
      args.push('/f');
    }

    execa(command, args).then(result => {
      // Processes without windows can't be killed without /F, detect and force
      // kill them directly
      if (result.stderr.indexOf('can only be terminated forcefully') !== -1) {
        kill(p, 'SIGKILL');
      }
    }).catch(err => {
      log.error(err);
    });
  } else {
    p.kill(sig);
  }
}
