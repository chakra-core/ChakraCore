'use strict';

var url = require('url');
var util = require('util');
var utils = require('./utils');
var gitConfigCache = {};

/**
 * Expose `repoUtils`
 * @type {Object}
 */

var repo = exports = module.exports;

/**
 * Get the `name` for a repository from:
 * - github repository path (`owner/project-name`)
 * - github URL
 * - absolute file path to a directory on the local file system (`.` and `''` may be used as aliases for the current working directory)
 *
 * ```js
 * repo.name(process.cwd());
 * //=> 'repo-utils'
 * repo.name('.');
 * //=> 'repo-utils'
 * repo.name();
 * //=> 'repo-utils'
 *
 * repo.name('https://github.com/jonschlinkert/repo-utils');
 * //=> 'repo-utils'
 * repo.name('jonschlinkert/repo-utils');
 * //=> 'repo-utils'
 * ```
 *
 * @param {String} `cwd` Absolute file path or github URL
 * @return {String} Project name
 * @api public
 */

repo.name = function(cwd) {
  if (cwd === '.' || cwd === '') {
    cwd = process.cwd();
  }
  if (utils.isObject(cwd) || (utils.isString(cwd) && !utils.isAbsolute(cwd))) {
    var repository = repo.repository.apply(repo, arguments);
    return repository.split('/').pop();
  }
  return utils.project(cwd);
};

/**
 * Create a github repository string in the form of `owner/name`, from:
 * - full github repository URL
 * - object returned from `url.parse`
 * - list of arguments in the form of `owner, name`
 *
 * ```js
 * repo.repository('jonschlinkert', 'micromatch');
 * //=> 'jonschlinkert/micromatch'
 *
 * repo.repository({owner: 'jonschlinkert', repository: 'micromatch'});
 * //=> 'jonschlinkert/micromatch'
 *
 * repo.repository('https://github.com/jonschlinkert/micromatch');
 * //=> 'jonschlinkert/micromatch'
 * ```
 * @param {String} `owner` Repository owner
 * @param {String} `name` Repository name
 * @return {String} Reps
 * @api public
 */

repo.repository = function(owner, name) {
  if (utils.isObject(owner)) {
    var obj = owner;
    // support config object from parsed URL

    owner = obj.owner;
    name = obj.name || name;

    if (!utils.isString(owner)) {
      var str = obj.repository || obj.repo || obj.pathname;
      if (utils.isString(str)) {
        var parsed = repo.parseUrl(str);
        owner = parsed.owner;
        name = name || parsed.name;
      }
    }
  }

  if (/\W/.test(owner)) {
    var res = repo.parseUrl(owner);
    if (utils.isString(res.repository)) {
      owner = res.owner;
      name = res.name;
    }
  }

  if (!utils.isString(owner)) {
    throw new TypeError('expected owner to be a string');
  }

  return owner + (name ? '/' + name : '');
};

/**
 * Create a `homepage` URL from a github repository path or github
 * repository URL.
 *
 * ```js
 * repo.homepage('jonschlinkert/repo-utils');
 * //=> 'https://github.com/jonchlinkert/repo-utils'
 * ```
 * @param {String} `repository` Repository in the form of `owner/project-name`
 * @param {Object} `options`
 * @return {String} Formatted github homepage url.
 * @api public
 */

repo.homepage = function(repository, options) {
  options = options || {};

  if (utils.isString(repository)) {
    if (utils.isString(options)) {
      repository += '/' + options;
      options = {};
    }
    options.repository = repository;
  }

  if (utils.isObject(repository)) {
    options = utils.merge({}, repository, options);
  }

  var opts = utils.omitEmpty(options);
  opts.repository = opts.repository || opts.pathname;

  if (!utils.isString(opts.repository)) {
    throw new TypeError('expected a repository string in the form of "owner/repo"');
  }

  if (!utils.isString(opts.hostname)) {
    opts.hostname = 'github.com';
  }
  if (!utils.isString(opts.protocol)) {
    opts.protocol = 'https:';
  }

  if (/[:.#]/.test(opts.repository)) {
    var parsed = repo.parseUrl(opts.repository, opts);
    // merge opts last, but override `repository` with parsed value
    opts = utils.merge({}, parsed, opts);
    opts.repository = parsed.repository;
  }

  opts.pathname = opts.repository;
  delete opts.hash;

  return url.format(opts);
};

/**
 * Create a GitHub `issues` URL.
 *
 * ```js
 * repo.isses('jonschlinkert/micromatch');
 * //=> 'https://github.com/jonchlinkert/micromatch/issues'
 * ```
 * @param {String} `repository` Repository in the form of `owner/project-name` or full github project URL.
 * @param {Object} `options`
 * @return {String}
 * @api public
 */

repo.issues = function(repository, options) {
  return repo.homepage(repository, options) + '/issues';
};

/**
 * Create a GitHub `bugs` URL. Alias for [.issues](#issues).
 *
 * ```js
 * repo.bugs('jonschlinkert/micromatch');
 * //=> 'https://github.com/jonchlinkert/micromatch/issues'
 * ```
 * @param {String} `repository` Repository in the form of `owner/project-name`
 * @param {Object} `options`
 * @return {String}
 * @api public
 */

repo.bugs = function(repository, options) {
  return repo.issues(repository, options);
};

/**
 * Create a github `https` URL.
 *
 * ```js
 * repo.https('jonschlinkert/micromatch');
 * //=> 'https://github.com/jonchlinkert/micromatch'
 * ```
 * @param  {String} `repository` Repository in the form of `owner/project-name`
 * @param  {Object|String} `options` Options object or optional branch
 * @param {String} `branch` Optionally specify a branch
 * @return {String}
 * @api public
 */

repo.https = function(repository, options, branch) {
  if (utils.isString(options)) {
    branch = options;
    options = {};
  }
  var homepage = repo.homepage(repository, options);
  branch = !repo.isMaster(branch) ? '/blob/' + branch : '';
  return homepage + branch;
};

/**
 * Create a travis URL.
 *
 * ```js
 * repo.travis('jonschlinkert/micromatch');
 * //=> 'https://travis-ci.org/jonschlinkert/micromatch'
 * ```
 * @param  {String} `repository` Repository in the form of `owner/project-name`
 * @param  {Object|String} `options` Options object or optional branch
 * @param {String} `branch` Optionally specify a branch
 * @return {String}
 * @api public
 */

repo.travis = function(repository, branch) {
  branch = !repo.isMaster(branch) ? '?branch=' + branch : '';
  return 'https://travis-ci.org/' + repository + branch;
};

/**
 * Create a URL for a file in a github repository.
 *
 * ```js
 * repo.file('https://github.com/jonschlinkert/micromatch', 'README.md');
 * //=> 'https://raw.githubusercontent.com/jonschlinkert/micromatch/master/README.md'
 *
 * repo.raw('jonschlinkert/micromatch', 'README.md');
 * //=> 'https://raw.githubusercontent.com/jonschlinkert/micromatch/master/README.md'
 * ```
 * @param  {String} `repository` Repository in the form of `owner/project-name` or full GitHub repository URL.
 * @param {String} `branch` Optionally specify a branch
 * @param {String} `path` Path to the file, relative to the repository root.
 * @return {String}
 * @api public
 */

repo.file = function(repository, branch, path) {
  if (!utils.isString(path)) {
    path = branch;
    branch = 'master';
  }
  var prefix = 'https://github.com/%s/blob/%s/%s';
  return util.format(prefix, repo.repository(repository), branch, path);
};

/**
 * Create a github "raw" content URL.
 *
 * ```js
 * repo.raw('https://github.com/jonschlinkert/micromatch', 'README.md');
 * //=> 'https://raw.githubusercontent.com/jonschlinkert/micromatch/master/README.md'
 *
 * repo.raw('jonschlinkert/micromatch', 'README.md');
 * //=> 'https://raw.githubusercontent.com/jonschlinkert/micromatch/master/README.md'
 * ```
 * @param  {String} `repository` Repository in the form of `owner/project-name`
 * @param  {Object|String} `options` Options object or optional branch
 * @param {String} `branch` Optionally specify a branch
 * @return {String}
 * @api public
 */

repo.raw = function(repository, branch, file) {
  if (!utils.isString(file)) {
    file = branch;
    branch = 'master';
  }
  var str = 'https://raw.githubusercontent.com/%s/%s/%s';
  return util.format(str, repo.repository(repository), branch, file);
};

/**
 * Return true if the given string looks like a github URL.
 *
 * ```js
 * utils.isGithubUrl('https://github.com/whatever');
 * //=> true
 * utils.isGithubUrl('https://foo.com/whatever');
 * //=> false
 * ```
 * @param {String} `str` URL to test
 * @return {Boolean}
 * @api public
 */

repo.isGithubUrl = function(str) {
  return [
    'api.github.com',
    'gist.github.com',
    'github.com',
    'github.io',
    'raw.githubusercontent.com'
  ].indexOf(url.parse(str).host) > -1;
};

/**
 * Parse a GitHub repository URL or repository `owner/project-name` into
 * an object.
 *
 * ```js
 * // see the tests for supported formats
 * repo.parse('https://raw.githubusercontent.com/jonschlinkert/micromatch/master/README.md');
 *
 * // results in:
 * { protocol: 'https:',
 *   slashes: true,
 *   hostname: 'raw.githubusercontent.com',
 *   host: 'raw.githubusercontent.com',
 *   pathname: 'https://raw.githubusercontent.com/foo/bar/master/README.md',
 *   path: '/foo/bar/master/README.md',
 *   href: 'https://raw.githubusercontent.com/foo/bar/master/README.md',
 *   owner: 'foo',
 *   name: 'bar',
 *   repo: 'foo/bar',
 *   repository: 'foo/bar',
 *   branch: 'master' }
 * ```
 * @param {String} `repositoryURL` Full repository URL, or repository path in the form of `owner/project-name`
 * @param {Object} `options`
 * @return {Boolean}
 * @api public
 */

repo.parseUrl = function(repoUrl, options) {
  if (!utils.isString(repoUrl)) {
    throw new TypeError('expected repository URL to be a string');
  }

  var defaults = {protocol: 'https:', slashes: true, hostname: 'github.com'};
  options = utils.omitEmpty(utils.extend({}, options));

  // if `foo/bar` is passed, use github.com as default host
  if (!/:\/\//.test(repoUrl)) {
    repoUrl = (options.host || 'https://github.com/') + repoUrl;
  }

  var obj = utils.omitEmpty(url.parse(repoUrl));
  var opts = utils.merge(defaults, options, obj);
  opts.pathname = repoUrl;

  var res = utils.omitEmpty(utils.parseGithubUrl(repoUrl));
  return utils.merge({}, opts, res);
};

/**
 * Parse a GitHub `repository` path or URL by calling `repo.parseUrl()`,
 * then expands it into an object of URLs. (the object also includes
 * properties returned from `.parse()`). A file path maybe be passed
 * as the second argument to include `raw` and `file` properties in the
 * result.
 *
 * ```js
 * // see the tests for supported formats
 * repo.expand('https://github.com/abc/xyz.git', 'README.md');
 *
 * // results in:
 * { protocol: 'https:',
 *   slashes: true,
 *   hostname: 'github.com',
 *   host: 'github.com',
 *   pathname: 'https://github.com/abc/xyz.git',
 *   path: '/abc/xyz.git',
 *   href: 'https://github.com/abc/xyz.git',
 *   owner: 'abc',
 *   name: 'xyz',
 *   repo: 'abc/xyz',
 *   repository: 'abc/xyz',
 *   branch: 'master',
 *   host_api: 'api.github.com',
 *   host_raw: 'https://raw.githubusercontent.com',
 *   api: 'https://api.github.com/repos/abc/xyz',
 *   tarball: 'https://api.github.com/repos/abc/xyz/tarball/master',
 *   clone: 'https://github.com/abc/xyz',
 *   zip: 'https://github.com/abc/xyz/archive/master.zip',
 *   https: 'https://github.com/abc/xyz',
 *   travis: 'https://travis-ci.org/abc/xyz',
 *   file: 'https://github.com/abc/xyz/blob/master/README.md',
 *   raw: 'https://raw.githubusercontent.com/abc/xyz/master/README.md' }
 * ```
 * @param {String} `repository`
 * @param {String} `file` Optionally pass a repository file path.
 * @return {String}
 * @api public
 */

repo.expandUrl = function(repository, file) {
  var config = repo.parseUrl(repository);

  repository = config.repository;
  var github = 'https://github.com';
  var branch = config.branch;

  var raw = 'https://raw.githubusercontent.com';
  var api = config.host !== 'github.com'
    ? config.host + '/api/v3'
    : 'api.github.com';

  var urls = config;
  urls.host_api = api;
  urls.host_raw = raw;
  urls.api = 'https://' + api + '/repos/' + repository;
  urls.tarball = urls.api + '/tarball/' + branch;
  urls.clone = github + '/' + repository;
  urls.zip = urls.clone + '/archive/' + branch + '.zip';

  urls.https = repo.https(config, branch);
  urls.travis = repo.travis(repository, branch);

  if (utils.isString(file)) {
    urls.file = repo.file(repository, branch, file);
    urls.raw = repo.raw(repository, branch, file);
  }
  urls.name = urls.name && urls.name.split('%20').join(' ');
  return urls;
};

/**
 * Get the local git config path, or global if a local `.git` repository does not exist.
 *
 * ```js
 * console.log(repo.gitConfigPath());
 * //=> /Users/jonschlinkert/dev/repo-utils/.git/config
 *
 * // if local .git repo does not exist
 * console.log(repo.gitConfigPath());
 * /Users/jonschlinkert/.gitconfig
 *
 * // get global path
 * console.log(repo.gitConfigPath('global'));
 * /Users/jonschlinkert/.gitconfig
 * ```
 * @param {String} `type` Pass `global` to get the global git config path regardless of whether or not a local repository exists.
 * @return {String} Returns the local or global git path
 * @api public
 */

repo.gitConfigPath = function(type) {
  return utils.gitConfigPath(type);
};

/**
 * Get and parse global git config.
 *
 * ```js
 * console.log(repo.gitConfig());
 * ```
 * @param {Object} `options` To get a local `.git` config, pass `{type: 'local'}`
 * @return {Object}
 * @api public
 */

repo.gitConfig = function(options) {
  var cwd = process.cwd();
  if (gitConfigCache[cwd]) {
    return gitConfigCache[cwd];
  }

  options = options || {};
  if (typeof options.type === 'undefined') {
    options.type = 'global';
  }

  var configPath = repo.gitConfigPath(options.type);
  if (utils.isString(configPath)) {
    var git = utils.parseGitConfig.sync({path: configPath});
    if (git && git.user) {
      utils.merge(git, utils.parseGitConfig.keys(git));
    }

    gitConfigCache[cwd] = git;
    return git;
  }
};

repo.gitUser = function(options) {
  return utils.get(repo.gitConfig(options), 'user');
};

repo.gitUserName = function(options) {
  return utils.get(repo.gitUser(options), 'name');
};
repo.gitUserEmail = function(options) {
  return utils.get(repo.gitUser(options), 'email');
};

/**
 * Get an owner string from the given object or string.
 *
 * ```js
 * console.log(repo.owner(require('./package.json')));
 * //=> 'jonschlinkert'
 * ```
 * @param {String|Object} `config` If an object is passed, it must have a `repository`, `url` or `author` propert (looked for in that order), otherwise if a string is passed it must be parse-able by the [parseUrl]() method.
 * @return {String}
 * @api public
 */

repo.owner = function(config) {
  var owner;
  if (utils.isString(config)) {
    config = repo.parseUrl(config);
  }
  if (utils.isObject(config)) {
    owner = utils.get(config, 'owner');
    if (typeof owner === 'undefined' && config.repository) {
      var repository = config.repository;
      if (utils.isObject(repository)) {
        repository = repository.url;
      }
      config = repo.parseUrl(repository);
      owner = utils.get(config, 'owner');
    }

    if (typeof owner === 'undefined' && config.url) {
      config = repo.parseUrl(config.url);
      owner = utils.get(config, 'owner');
    }

    if (typeof owner === 'undefined' && config.author) {
      var author = repo.parseAuthor(config.author);
      config = repo.parseUrl(author.url);
      owner = utils.get(config, 'owner');
    }
  }
  return owner;
};

/**
 * Normalize a "person" object. If a "person" string is passed (like `author`, `contributor` etc)
 * it is parsed into an object, otherwise the object is returned.
 *
 * ```js
 * console.log(repo.person('Brian Woodward (https://github.com/doowb)'));
 * //=> { name: 'Brian Woodward', url: 'https://github.com/doowb' }
 * console.log(repo.person({ name: 'Brian Woodward', url: 'https://github.com/doowb' }));
 * //=> { name: 'Brian Woodward', url: 'https://github.com/doowb' }
 * ```
 * @param {String|Object} `val`
 * @return {Object}
 * @api public
 */

repo.person = function(val) {
  var person = val;
  if (utils.isString(val)) {
    person = utils.parseAuthor(val);
  }
  if (utils.isObject(person)) {
    person = utils.omitEmpty(person);
  }
  return person;
};

/**
 * Returns an `author` object from the given given config object. If `config.author` is
 * a string it will be parsed into an object.
 *
 * ```js
 * console.log(repo.author({
 *   author: 'Brian Woodward (https://github.com/doowb)'
 * }));
 * //=> { name: 'Brian Woodward', url: 'https://github.com/doowb' }
 *
 * console.log(repo.author({
 *   name: 'Brian Woodward',
 *   url: 'https://github.com/doowb'
 * }));
 * //=> { name: 'Brian Woodward', url: 'https://github.com/doowb' }
 * ```
 * @param {Object} `config` Object with an `author` property
 * @return {Object}
 * @api public
 */

repo.author = function(config) {
  if (!utils.isObject(config)) {
    throw new TypeError('expected an object');
  }
  return repo.person(config.author);
};

/**
 * Returns the `author.name` from the given config object. If `config.author` is
 * a string it will be parsed into an object first.
 *
 * ```js
 * console.log(repo.authorName({
 *   author: 'Brian Woodward (https://github.com/doowb)'
 * }));
 * //=> 'Brian Woodward'
 *
 * console.log(repo.authorName({
 *   name: 'Brian Woodward',
 *   url: 'https://github.com/doowb'
 * }));
 * //=> 'Brian Woodward'
 * ```
 * @param {Object} `config` Object with an `author` property
 * @return {Object}
 * @api public
 */

repo.authorName = function(config, options) {
  return utils.get(repo.author(config, options), 'name');
};

/**
 * Returns the `author.url` from the given config object. If `config.author` is
 * a string it will be parsed into an object first.
 *
 * ```js
 * console.log(repo.authorUrl({
 *   author: 'Brian Woodward (https://github.com/doowb)'
 * }));
 * //=> 'https://github.com/doowb'
 *
 * console.log(repo.authorUrl({
 *   name: 'Brian Woodward',
 *   url: 'https://github.com/doowb'
 * }));
 * //=> 'https://github.com/doowb'
 * ```
 * @param {Object} `config` Object with an `author` property
 * @return {Object}
 * @api public
 */

repo.authorUrl = function(config, options) {
  return utils.get(repo.author(config, options), 'url');
};

/**
 * Returns the `author.email` from the given config object. If `config.author` is
 * a string it will be parsed into an object first.
 *
 * ```js
 * console.log(repo.authorEmail({
 *   author: 'Brian Woodward <brian.woodward@sellside.com> (https://github.com/doowb)'
 * }));
 * //=> 'brian.woodward@sellside.com'
 *
 * console.log(repo.authorEmail({
 *   name: 'Brian Woodward',
 *   url: 'https://github.com/doowb',
 *   email: 'brian.woodward@sellside.com'
 * }));
 * //=> 'brian.woodward@sellside.com'
 * ```
 * @param {Object} `config` Object with an `author` property
 * @return {Object}
 * @api public
 */

repo.authorEmail = function(config, options) {
  return utils.get(repo.author(config, options), 'email');
};

/**
 * Returns the `author.username` from the given config object. If `config.author` is
 * a string it will be parsed into an object first.
 *
 * ```js
 * console.log(repo.authorUsername({
 *   author: 'Brian Woodward <brian.woodward@sellside.com> (https://github.com/doowb)'
 * }));
 * //=> 'doowb'
 *
 * console.log(repo.authorUsername({
 *   name: 'Brian Woodward',
 *   url: 'https://github.com/doowb',
 *   email: 'brian.woodward@sellside.com'
 * }));
 * //=> 'doowb'
 * ```
 * @param {Object} `config` Object with an `author` property
 * @return {Object}
 * @api public
 */

repo.authorUsername = function(config, options) {
  return utils.get(repo.author(config, options), 'username');
};

/**
 * Returns a `username` from the given config object, by first attempting to get
 * `author.username`, then
 *
 * ```js
 * console.log(repo.username({
 *   author: 'Brian Woodward <brian.woodward@sellside.com> (https://github.com/doowb)'
 * }));
 * //=> 'doowb'
 *
 * console.log(repo.username({
 *   name: 'Brian Woodward',
 *   url: 'https://github.com/doowb',
 *   email: 'brian.woodward@sellside.com'
 * }));
 * //=> 'doowb'
 * ```
 * @param {Object} `config` Object with an `author` property
 * @return {Object}
 * @api public
 */

repo.username = function(config, options) {
  if (!utils.isObject(config)) {
    throw new TypeError('expected an object');
  }

  var username = repo.authorUsername(config);
  if (!utils.isString(username)) {
    var authorUrl = repo.authorUrl(config, options);
    if (/github\.com/.test(authorUrl)) {
      username = utils.get(utils.parseGithubUrl(authorUrl), 'owner');
    }
  }

  if (!utils.isString(username)) {
    username = repo.gitUserName(options);
  }

  if (!utils.isString(username)) {
    var str = config.repository || config.homepage;
    username = utils.get(utils.parseGithubUrl(str), 'owner');
  }
  return username;
};

/**
 * Returns true if the given value is `master` or `undefined`. Used in other
 * methods.
 *
 * @param {String} `branch`
 * @return {Boolean}
 */

repo.isMaster = function(branch) {
  return typeof branch === 'undefined' || branch === 'master';
};

/**
 * Expose `parseAuthor`
 */

repo.parseAuthor = utils.parseAuthor;
