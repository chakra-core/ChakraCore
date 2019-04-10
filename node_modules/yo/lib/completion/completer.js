'use strict';
const path = require('path');
const {execFile} = require('child_process');
const parseHelp = require('parse-help');

/**
 * The Completer is in charge of handling `yo-complete` behavior.
 * @constructor
 * @param  {Environment} env A yeoman environment instance
 */
class Completer {
  constructor(env) {
    this.env = env;
  }

  /**
   * Completion event done
   *
   * @param {String}   data   Environment object as parsed by tabtab
   * @param {Function} done   Callback to invoke with completion results
   */
  complete(data, done) {
    if (data.last !== 'yo' && !data.last.startsWith('-')) {
      return this.generator(data, done);
    }

    this.env.lookup(err => {
      if (err) {
        done(err);
        return;
      }

      const meta = this.env.getGeneratorsMeta();
      const results = Object.keys(meta).map(this.item('yo'), this);
      done(null, results);
    });
  }

  /**
   * Generator completion event done
   *
   * @param {String}   data   Environment object as parsed by tabtab
   * @param {Function} done   Callback to invoke with completion results
   */
  generator(data, done) {
    const {last} = data;
    const bin = path.resolve(__dirname, '../cli.js');

    execFile('node', [bin, last, '--help'], (err, out) => {
      if (err) {
        done(err);
        return;
      }

      const results = this.parseHelp(last, out);
      done(null, results);
    });
  }

  /**
   * Helper to format completion results into { name, description } objects
   *
   * @param {String}   data   Environment object as parsed by tabtab
   * @param {Function} done   Callback to invoke with completion results
   */
  item(desc, prefix) {
    prefix = prefix || '';
    return item => {
      const name = typeof item === 'string' ? item : item.name;
      desc = typeof item !== 'string' && item.description ? item.description : desc;
      desc = desc.replace(/^#?\s*/g, '');
      desc = desc.replace(/:/g, '->');
      desc = desc.replace(/'/g, ' ');

      return {
        name: prefix + name,
        description: desc
      };
    };
  }

  /**
   * Parse-help wrapper. Invokes parse-help with stdout result, returning the
   * list of completion items for flags / alias.
   *
   * @param {String}   last  Last word in COMP_LINE (completed line in command line)
   * @param {String}   out   Help output
   */
  parseHelp(last, out) {
    const help = parseHelp(out);
    const alias = [];

    let results = Object.keys(help.flags).map(key => {
      const flag = help.flags[key];

      if (flag.alias) {
        alias.push(Object.assign({}, flag, {name: flag.alias}));
      }

      flag.name = key;

      return flag;
    }).map(this.item(last, '--'), this);

    results = results.concat(alias.map(this.item(last.replace(':', '_'), '-'), this));
    results = results.filter(r => r.name !== '--help' && r.name !== '-h');

    return results;
  }
}

module.exports = Completer;
