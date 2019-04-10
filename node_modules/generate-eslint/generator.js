'use strict';

var path = require('path');
var templates = path.join(__dirname, 'templates');
var isValid = require('is-valid-app');

module.exports = function(app) {
  if (!isValid(app, 'generate-eslint')) return;

  /**
   * Generate a `.eslintrc.json` file to the current working directory. Described in more
   * detail in the [usage](#usage) section.
   *
   * ```sh
   * $ gen eslint
   * ```
   * @name default
   * @api public
   */

  app.task('default', {silent: true}, ['eslintrc']);

  /**
   * Alias for the [default](#default) task, to provide a semantic task name for
   * when this generator is used as a plugin or sub-generator.
   *
   * ```sh
   * $ gen eslint:eslintrc
   * ```
   * @name eslintrc
   * @api public
   */

  app.task('eslintrc', {silent: true}, function() {
    return app.src('_eslintrc.json', {cwd: templates})
      .pipe(app.dest(function(file) {
        file.basename = '.eslintrc.json';
        return app.cwd;
      }));
  });

  /**
   * Generate a `.eslintignore` file to the current working directory. This task
   * is also aliased as `eslintignore` to provide a more semantic task name for
   * when this generator is used as a plugin or sub-generator.
   *
   * ```sh
   * $ gen eslint:ignore
   * ```
   * @name ignore
   * @api public
   */

  app.task('ignore', {silent: true}, ['eslintignore']);
  app.task('eslintignore', {silent: true}, function() {
    return app.src('_eslintignore', {cwd: templates})
      .pipe(app.dest(function(file) {
        file.basename = '.eslintignore';
        return app.cwd;
      }));
  });
};
