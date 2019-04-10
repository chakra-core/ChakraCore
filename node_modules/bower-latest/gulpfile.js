'use strict';

var gulp   = require('gulp');
var jshint = require('gulp-jshint');
var jscs = require('gulp-jscs');
var istanbul = require('gulp-istanbul');
var mocha  = require('gulp-mocha');
var bump   = require('gulp-bump');
var _ = require('lodash');

var paths = {
  lint: ['./gulpfile.js', './lib/**/*.js'],
  tests: ['./test/**/*.js', '!test/{temp,temp/**}'],
  source: ['./lib/*.js']
};

gulp.task('lint', function () {
  return gulp.src(paths.lint)
    .pipe(jshint('.jshintrc'))
    .pipe(jscs())
    .pipe(jshint.reporter('jshint-stylish'));
});

gulp.task('istanbul', function (cb) {
  gulp.src(paths.source)
    .pipe(istanbul()) // Covering files
    .on('end', function () {
      gulp.src(paths.tests)
        .pipe(mocha())
        .pipe(istanbul.writeReports()) // Creating the reports after tests runned
        .on('end', cb);
    });
});

gulp.task('bump', ['test'], function () {
  var bumpType = process.env.BUMP || 'patch'; // major.minor.patch

  return gulp.src(['./package.json'])
    .pipe(bump({ type: bumpType }))
    .pipe(gulp.dest('./'));
});

gulp.task('watch', function () {
  gulp.run('istanbul');
  gulp.watch(_.union(paths.lint, paths.tests), ['istanbul']);
});

gulp.task('test', ['lint', 'istanbul']);
gulp.task('release', ['bump']);
