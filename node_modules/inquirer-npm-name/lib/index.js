'use strict';
var npmName = require('npm-name');

/**
 * This function will use the given prompt config and will then check if the value is
 * available as a name on npm. If the name is already picked, we'll ask the user to
 * confirm or pick another name.
 * @param  {Object}   prompt   Inquirer prompt configuration
 * @param  {inquirer} inquirer Object with a `prompt` method. Usually `inquirer` or a
 *                             yeoman-generator.
 */
module.exports = function askName(prompt, inquirer) {
  var prompts = [prompt, {
    type: 'confirm',
    name: 'askAgain',
    message: 'The name above already exists on npm, choose another?',
    default: true,
    when: function (answers) {
      return npmName(answers[prompt.name]).then(function (available) {
        return !available;
      });
    }
  }];

  return inquirer.prompt(prompts).then(function (props) {
    if (props.askAgain) {
      return askName(prompt, inquirer);
    }

    return props;
  });
};
