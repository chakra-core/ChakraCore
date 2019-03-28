#!/usr/bin/env node

var util = require('util');
var utils = require('./utils');
var config = require('./');
var cli = new utils.Composer();
var argv = require('yargs-parser')(process.argv.slice(2), {
  alias: {
    del: 'd',
    get: 'g',
    init: 'i',
    set: 's',
    show: 'get'
  }
});

/**
 * Commands
 */

cli.task('help', function(cb) {
  console.log([
    '',
    `${utils.log.gray('$ common-config --help')}`,
    ``,
    `  Usage: ${info('common-config <command> [value]')}`,
    '',
    '  Commands:',
    '    --init, -i  Initialize a prompt to store common values',
    '    --set,  -s  Save a value to the common-config store',
    '    --get,  -g  Show all values, or a specific value from the common-config store',
    '    --del,  -d  Delete a value from the common-config store',
    '    --help      Display this menu',
    ''
  ].join('\n'));
  cb();
});

/**
 * Initialize a prompt session
 */

cli.task('init', function(cb) {
  var questions = new utils.Questions();
  questions.on('ask', function(val, key, question, answers) {
    if (typeof val === 'undefined') {
      val = config.get(key);
    }

    if (key === 'author.twitter') {
      question.default = val || utils.get(answers, 'author.username');
      return;
    }

    if (key === 'author.url' && typeof val === 'undefined') {
      var address = utils.get(answers, 'author.username');
      question.default = `https://github.com/${address}`;
      return;
    }

    question.default = val;
  });

  questions.set('author.name', 'Full name?');
  questions.set('author.username', 'GitHub username?');
  questions.set('author.twitter', 'Twitter username?');
  questions.set('author.url', 'URL?');
  questions.set('license', 'Preferred license?', {default: 'MIT'});
  questions.ask(function(err, answers) {
    if (err) {
      cb(err);
      return;
    }

    answers = utils.omitEmpty(answers);
    config.set(answers);
    console.log();
    console.log('The following values were saved:');
    console.log();
    var data = utils.tableize(answers);
    var keys = Object.keys(data);
    show(keys, data, 'green');
    console.log();
    cb();
  });
});

/**
 * Set a value
 */

cli.task('set', function(cb) {
  config.set(argv.set);
  console.log(utils.log.success, 'saved', success(argv.set));
  cb();
});

/**
 * Show a value
 */

cli.task('get', function(cb) {
  var data = utils.tableize(config.data);
  var keys = argv.get;
  if (keys === true) {
    keys = Object.keys(data);
  } else if (keys.length === 0) {
    return cli.build('all', cb);
  }
  show(keys, data, 'cyan');
  cb();
});

/**
 * Show all values
 */

cli.task('all', function(cb) {
  console.log(bold('common-config:'));
  console.log(info(config.data));
  cb();
});

/**
 * Delete a value
 */

cli.task('del', function(cb) {
  var data = utils.tableize(config.data);
  var keys = Object.keys(data);

  if (keys.length === 0) {
    console.log(utils.log.error, 'nothing to delete');
    cb();
    return;
  }

  if (argv.del === true) {
    var questions = new utils.Questions();
    questions.confirm('del', 'Are you sure you want to delete the entire store?', {default: false});
    questions.ask('del', function(err, answers) {
      if (err) {
        cb(err);
        return;
      }
      if (answers.del === true) {
        config.del(keys);
        console.log(utils.log.success, 'deleted', error(data));
      } else {
        console.log(utils.log.info, 'Got it. No values were deleted.');
      }
      cb();
    });
    return;
  }

  keys = argv.del;
  keys.forEach(function(key) {
    config.del(key);
  });

  console.log(utils.log.success, 'deleted', error(keys.join(', ')));
  cb();
});

/**
 * Default
 */

cli.task('default', function(cb) {
  argv = normalize(argv);
  cli.build(argv._, function(err) {
    if (err) return console.log(err);
    cb();
  });
});

/**
 * Run the given command(s)
 */

cli.build('default', function(err) {
  if (err) return console.log(err);
});

/**
 * Logging utils
 */

function info(val) {
  return utils.log.cyan(inspect(val));
}

function success(val) {
  return utils.log.green(inspect(val));
}

function bold(val) {
  return utils.log.bold(val);
}

function error(val) {
  return utils.log.red(inspect(val));
}

function warn(val) {
  return utils.log.yellow(inspect(val));
}

function heading(val) {
  return utils.log.heading(val);
}

function inspect(val) {
  return typeof val === 'string' ? val : util.inspect(val);
}

function show(keys, data, color) {
  color = color || 'cyan';
  var val = utils.pick(data, keys);
  var headings = [heading('property'), heading('value')];
  var rows = [];
  for (var key in val) {
    rows.push([key, utils.log[color](inspect(val[key]))]);
  }
  var list = [headings].concat(rows);
  var table = utils.table(list, {
    stringLength: function(str) {
      return utils.stripColor(str).length;
    }
  });
  console.log(table);
}

/**
 * Normalize arguments
 */

function normalize(argv) {
  var keys = Object.keys(cli.tasks);
  keys.forEach(function(key) {
    if (argv.hasOwnProperty(key) && key !== 'default') {
      argv._.push(key);
      parse(argv[key], key, argv);
    }
  });

  if (argv._.length === 0) {
    argv._.push('help');
  }
  return argv;
}

function parse(val, key, argv) {
  if ((key === 'del' || key === 'get') && val === true) {
    return;
  }

  if ((key === 'del' || key === 'get') && typeof val === 'string') {
    argv[key] = val.split(',');
    return;
  }

  var obj = {};
  if (typeof val === 'string' && /[:=]/.test(val)) {
    var segs = val.split(/[:=]/);
    utils.set(obj, segs.shift(), segs.pop());
    argv[key] = obj;
  } else {
    utils.set(obj, val, true);
    argv[key] = obj;
  }
}
