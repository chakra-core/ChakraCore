'use strict';

const path = require('path');
const isValid = require('is-valid-app');

module.exports = function(app) {
  if (!isValid(app, 'generate-contributing')) return;

  /**
   * Register generate-defaults as a plugin
   */

  app.use(require('generate-defaults'));

  /**
   * Generate a `contributing.md` file.
   *
   * ```sh
   * $ gen contributing
   * $ gen contributing --dest ./docs
   * ```
   * @name contributing
   * @api public
   */

  task('default', ['contributing']);
  task('contributing', () => template('contributing.md'));

  /**
   * Generate an `issue_template.md` file to the `.github/` directory, or specified `--dest`.
   *
   * ```sh
   * $ gen contributing:it
   * $ gen contributing:it --dest ./docs
   * # also aliased as the following (mostly for API usage)
   * $ gen contributing:issue_template
   * ```
   * @name contributing:it
   * @api public
   */

  task('it', ['issue_template']);
  task('issue_template', () => template('issue_template.md'));

  /**
   * Generate a bare bones `issue_template.md` file to the `.github/` directory, or specified `--dest`.
   * Uses [this template](templates/issue_template_basic.md).
   *
   * ```sh
   * $ gen contributing:itb
   * $ gen contributing:itb --dest ./docs
   * # also aliased as the following (mostly for API usage)
   * $ gen contributing:issue_template_detailed
   * ```
   * @name contributing:itb
   * @api public
   */

  task('itb', ['issue_template_basic']);
  task('issue_template_basic', () => template('issue_template_basic.md'));

  /**
   * Generate a detailed `issue_template.md` file to the `.github/` directory, or specified `--dest`.
   * Uses [this template](templates/issue_template_detailed.md)
   *
   * ```sh
   * $ gen contributing:itd
   * $ gen contributing:itd --dest ./docs
   * # also aliased as the following (mostly for API usage)
   * $ gen contributing:issue_template_detailed
   * ```
   * @name contributing:itd
   * @api public
   */

  task('itd', ['issue_template_detailed']);
  task('issue_template_detailed', () => template('issue_template_detailed.md'));

  /**
   * Generate a `pull_request_template.md` file to the `.github/` directory, or specified `--dest`.
   * Uses [this template](templates/pull_request_template.md).
   *
   * ```sh
   * $ gen contributing:pr
   * $ gen contributing:pr --dest ./docs
   * # also aliased as the following (mostly for API usage)
   * $ gen contributing:pr_template
   * ```
   * @name contributing:pr
   * @api public
   */

  task('pr', ['pr_template']);
  task('pr_template', () => template('pull_request_template.md'));

  /**
   * Generate a detailed `pull_request_template.md` file to the `.github/` directory, or specified `--dest`.
   * Uses [this template](templates/pull_request_template_detailed.md).
   *
   * ```sh
   * $ gen contributing:prd
   * $ gen contributing:prd --dest ./docs
   * # also aliased as the following (for API usage, when it helps to be explicit in code)
   * $ gen contributing:pr_template_detailed
   * ```
   * @name contributing:prd
   * @api public
   */

  task('prd', ['pr_template_detailed']);
  task('pr_template_detailed', () => template('pull_request_template_detailed.md'));

  /**
   * Generate a file from the template that matches the given `pattern`
   */

  function template(pattern) {
    return app.src(pattern, { cwd: path.join(__dirname, 'templates') })
      .pipe(app.renderFile('*')).on('error', console.error)
      .pipe(app.conflicts(app.cwd))
      .pipe(app.dest(app.options.dest || app.cwd));
  }

  /**
   * Create a silent task
   */

  function task(name, ...deps) {
    app.task(name, { silent: true }, ...deps);
  }
};
