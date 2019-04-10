#! /usr/bin/env node

var fs     = require('fs');
var path   = require('path');
var read   = fs.readSync;
var exists = fs.existsSync

var tabtab  = require('../..')();
var parse   = require('parse-help');
var exec    = require('child_process').exec;
var reg     = /^\s+(\w+)\s+(.+)$/gim;
var regline = /^\s+(\w+)\s+(.+)$/i;
var debug = require('../../src/debug')('bower-complete');


function help(cmd, done) {
  cmd = cmd || '';
  var cache = path.join('/tmp', 'tabtab.' + (cmd || 'help') + '.json');
  if (exists(cache)) {
    var dt = require(cache);
    return done(null, dt);
  }

  debug('>bower ' + cmd + ' --help');
  exec('bower ' + cmd + ' --help', function(err, out) {
    if (err) return done(err);
    var data = parse(out);
    data.commands = out.match(reg).filter(function(line) {
      return !(/<command>/).test(line);
    }).map(function(m) {
      var parts = m.match(regline);
      return {
        name: parts[1],
        description: (parts[2] || 'tabtab').replace(/'/g, ' ')
      };
    });

    fs.writeFileSync(path.join('/tmp/', 'tabtab.' + (cmd || 'help') + '.json'), JSON.stringify(data));
    return done(null, data);
  });
}


tabtab.on('bower', function(compl, done) {
  var line = compl.line;
  var last = compl.last;
  if (last === 'bower') last = '';

  help(last + ' --help', function(err, data) {
    if (err) return done(err);

    var results = data.commands.concat();
    // var results = [];

    if (compl.last === 'bower') {
      results = results.concat(Object.keys(data.flags).map(function(key) {
        var flag = data.flags[key];
        return {
          name: '--' + key,
          description: (flag.description || 'tabtab').replace(/'/g, ' ')
        };
      }));

      results = results.concat(Object.keys(data.flags).map(function(key) {
        var flag = data.flags[key];
        var alias = flag.alias;
        if (!alias) return;

        return {
          name: '-' + flag.alias,
          description: (flag.description || 'tabtab').replace(/'/g, ' ')
        };
      }).filter(function(item) {
        return item;
      }));

      results = results.filter(function(res) {
        return !(/Additionally/).test(res.name)
      });
    }

    debug(results);
    done(null, results);
  });
});

tabtab.start();
