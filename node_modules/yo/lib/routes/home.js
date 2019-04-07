'use strict';
const _ = require('lodash');
const chalk = require('chalk');
const fullname = require('fullname');
const inquirer = require('inquirer');
const {isString} = require('lodash');
const {namespaceToName} = require('yeoman-environment');
const globalConfigHasContent = require('../utils/global-config').hasContent;

module.exports = app => {
  const defaultChoices = [{
    name: 'Install a generator',
    value: 'install'
  }, {
    name: 'Find some help',
    value: 'help'
  }, {
    name: 'Get me out of here!',
    value: 'exit'
  }];

  if (globalConfigHasContent()) {
    defaultChoices.splice(defaultChoices.length - 1, 0, {
      name: 'Clear global config',
      value: 'clearConfig'
    });
  }

  const generatorList = _.chain(app.generators).map(generator => {
    if (!generator.appGenerator) {
      return null;
    }

    const updateInfo = generator.updateAvailable ? chalk.dim.yellow(' ♥ Update Available!') : '';

    return {
      name: generator.prettyName + updateInfo,
      value: {
        method: 'run',
        generator: generator.namespace
      }
    };
  }).compact().sortBy(el => {
    const generatorName = namespaceToName(el.value.generator);
    return -app.conf.get('generatorRunCount')[generatorName] || 0;
  }).value();

  if (generatorList.length > 0) {
    defaultChoices.unshift({
      name: 'Update your generators',
      value: 'update'
    });
  }

  app.insight.track('yoyo', 'home');

  return fullname().then(name => {
    const allo = (name && isString(name)) ? `'Allo ${name.split(' ')[0]}! ` : '\'Allo! ';

    return inquirer.prompt([{
      name: 'whatNext',
      type: 'list',
      message: `${allo}What would you like to do?`,
      choices: _.flatten([
        new inquirer.Separator('Run a generator'),
        generatorList,
        new inquirer.Separator(),
        defaultChoices,
        new inquirer.Separator()
      ])
    }]).then(answer => {
      if (answer.whatNext.method === 'run') {
        app.navigate('run', answer.whatNext.generator);
        return;
      }

      if (answer.whatNext === 'exit') {
        return;
      }

      app.navigate(answer.whatNext);
    });
  });
};
