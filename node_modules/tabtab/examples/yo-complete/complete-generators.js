const debug    = require('tabtab/lib/debug')('tabtab:yo:generators');
const env      = require('yeoman-environment').createEnv();
const { exec } = require('child_process');
const parse    = require('parse-help');

// Lookup installed generator in yeoman environment, respond completion results
// with each generator.

let npmroot = '';

module.exports = complete;

function complete(data, done) {
  debug('Init completion', data);

  if (npmroot) {
    process.env.NODE_PATH = npmroot;
    return lookup(data, done);
  }


  exec('npm root -g', (err, out) => {
    if (err) return done(err);
    process.env.NODE_PATH = npmroot = out.trim();
    lookup(data, done);
  });
}

function lookup(data, done) {
  debug('Lookup with', process.env.NODE_PATH);

  if (data.last !== 'yo' && !/^-/.test(data.last)) {
    return completeGenerator(data, done);
  }

  env.lookup((err) => {
    if (err) return done(err);
    var meta = env.getGeneratorsMeta();
    var results = Object.keys(meta).map(completionItem('yo'));
    debug('yor', results);
    done(null, results);
  });
}

function completeGenerator(data, done) {
  var last = data.last;

  exec(`yo ${last} --help`, (err, out) => {
    if (err) return done(err);
    var help = parse(out);
    var alias = [];
    var results = Object.keys(help.flags).map((key) => {
      var flag = help.flags[key];
      if (flag.alias) alias.push(Object.assign({}, flag, { name: flag.alias }));
      flag.name = key;
      return flag;
    }).map(completionItem(last, '--'));

    // results = results.concat(Object.keys(help.aliases)).map(completionItem(last.replace(':', '_'), '\-'));
    // results = results.concat(Object.keys(help.aliases)).map(completionItem('yo', '-'));
    results = results.concat(alias.map(completionItem(last.replace(':', '_'), '-')));

    // issue with `--help` that triggers help output of echo command in shell script :/
    results = results.filter((r) => {
      return r.name !== '--help' && r.name !== '-h';
    });


    debug('r', results, help, alias);
    done(null, results);
  });
}

function completionItem(desc, prefix) {
  prefix = prefix || '';
  return function(item) {
    var name = (typeof item === 'string' ? item : item.name);
    desc = typeof item !== 'string' && item.description ? item.description : desc;
    desc = desc.replace(/^#\s*/g, '');
    desc = desc.replace(/:/, '->');
    desc = desc.replace(/'/, ' ');

    debug('->', prefix, name, desc);
    return {
      name: prefix + name,
      description: desc
    };
  }
}
