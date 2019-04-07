'use strict';
var npmName = require('npm-name');
var validatePackageName = require('validate-npm-package-name');
var upperCaseFirst = require('upper-case-first');

var defaults = {
  message: 'Module Name',
  validate: function () {
    return true;
  }
};

/**
 * This function will use the given prompt config and will then check if the value is
 * available as a name on npm. If the name is already picked, we'll ask the user to
 * confirm or pick another name.
 * @param  {(Object|String)} prompt   Inquirer prompt configuration or the name to be
 *                                    used in it.
 * @param  {inquirer}        inquirer Object with a `prompt` method. Usually `inquirer`
                                      or a yeoman-generator.
 */
module.exports = function askName(prompt, inquirer) {
  if (typeof prompt === 'string') {
    prompt = {
      name: prompt
    };
  }

  var prompts = [
    Object.assign({}, defaults, prompt, {
      validate: function (val) {
        var packageNameValidity = validatePackageName(val);

        if (packageNameValidity.validForNewPackages) {
          var validate = prompt.validate || defaults.validate;

          return validate.apply(this, arguments);
        }

        return upperCaseFirst(packageNameValidity.errors[0]) ||
          'The provided value is not a valid npm package name';
      }
    }),
    {
      type: 'confirm',
      name: 'askAgain',
      message: 'The name above already exists on npm, choose another?',
      default: true,
      when: function (answers) {
        return npmName(answers[prompt.name]).then(function (available) {
          return !available;
        });
      }
    }
  ];

  return inquirer.prompt(prompts).then(function (props) {
    if (props.askAgain) {
      return askName(prompt, inquirer);
    }

    return props;
  });
};
