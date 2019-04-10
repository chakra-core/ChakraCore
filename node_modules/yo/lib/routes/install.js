'use strict';
/* eslint-disable promise/no-callback-in-promise */
const _ = require('lodash');
const async = require('async');
const chalk = require('chalk');
const inquirer = require('inquirer');
const spawn = require('cross-spawn');
const sortOn = require('sort-on');
const figures = require('figures');
const npmKeyword = require('npm-keyword');
const packageJson = require('package-json');
const got = require('got');

const OFFICIAL_GENERATORS = [
  'generator-angular',
  'generator-backbone',
  'generator-bootstrap',
  'generator-chrome-extension',
  'generator-chromeapp',
  'generator-commonjs',
  'generator-generator',
  'generator-gruntplugin',
  'generator-gulp-webapp',
  'generator-jasmine',
  'generator-jquery',
  'generator-karma',
  'generator-mobile',
  'generator-mocha',
  'generator-node',
  'generator-polymer',
  'generator-webapp'
];

module.exports = app => {
  app.insight.track('yoyo', 'install');

  return inquirer.prompt([{
    name: 'searchTerm',
    message: 'Search npm for generators:'
  }]).then(answers => searchNpm(app, answers.searchTerm));
};

const generatorMatchTerm = (generator, term) => `${generator.name} ${generator.description}`.includes(term);
const getAllGenerators = _.memoize(() => npmKeyword('yeoman-generator'));

function searchMatchingGenerators(app, term, cb) {
  function handleBlacklist(blacklist) {
    const installedGenerators = app.env.getGeneratorNames();

    getAllGenerators().then(allGenerators => {
      cb(null, allGenerators.filter(generator => {
        if (blacklist.includes(generator.name)) {
          return false;
        }

        if (installedGenerators.includes(generator.name)) {
          return false;
        }

        return generatorMatchTerm(generator, term);
      }));
    }, cb);
  }
  got('http://yeoman.io/blacklist.json', {json: true})
    .then(response => handleBlacklist(response.body))
    .catch(() => handleBlacklist([]));
}

function fetchGeneratorInfo(generator, cb) {
  packageJson(generator.name, {fullMetadata: true}).then(pkg => {
    const official = OFFICIAL_GENERATORS.includes(pkg.name);
    const mustache = official ? chalk.green(` ${figures.mustache} `) : '';

    cb(null, {
      name: generator.name.replace(/^generator-/, '') + mustache + ' ' + chalk.dim(pkg.description),
      value: generator.name,
      official: -official
    });
  }).catch(cb);
}

function searchNpm(app, term) {
  const promise = new Promise((resolve, reject) => {
    searchMatchingGenerators(app, term, (err, matches) => {
      if (err) {
        reject(err);
        return;
      }

      async.map(matches, fetchGeneratorInfo, (err2, choices) => {
        if (err2) {
          reject(err2);
          return;
        }

        resolve(choices);
      });
    });
  });

  return promise.then(choices => promptInstallOptions(app, sortOn(choices, ['official', 'name'])));
}

function promptInstallOptions(app, choices) {
  let introMessage = 'Sorry, no results matches your search term';

  if (choices.length > 0) {
    introMessage = 'Here\'s what I found. ' + chalk.gray('Official generator → ' + chalk.green(figures.mustache)) + '\n  Install one?';
  }

  const resultsPrompt = [{
    name: 'toInstall',
    type: 'list',
    message: introMessage,
    choices: choices.concat([{
      name: 'Search again',
      value: 'install'
    }, {
      name: 'Return home',
      value: 'home'
    }])
  }];

  return inquirer.prompt(resultsPrompt).then(answer => {
    if (answer.toInstall === 'home' || answer.toInstall === 'install') {
      return app.navigate(answer.toInstall);
    }

    installGenerator(app, answer.toInstall);
  });
}

function installGenerator(app, pkgName) {
  app.insight.track('yoyo', 'install', pkgName);

  return spawn('npm', ['install', '--global', pkgName], {stdio: 'inherit'})
    .on('error', err => {
      app.insight.track('yoyo:err', 'install', pkgName);
      throw err;
    })
    .on('close', () => {
      app.insight.track('yoyo', 'installed', pkgName);

      console.log(
        '\nI just installed a generator by running:\n' +
        chalk.blue.bold('\n    npm install -g ' + pkgName + '\n')
      );

      app.env.lookup(() => {
        app.updateAvailableGenerators();
        app.navigate('home');
      });
    });
}
