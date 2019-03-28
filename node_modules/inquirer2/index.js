'use strict';

/**
 * Expose `inquirer`
 */

module.exports = function(options) {
  var inquirer = {};

  /**
   * Client interfaces
   */

  inquirer.prompts = {};
  inquirer.Separator = require('./lib/objects/separator');
  inquirer.ui = {
    BottomBar: require('./lib/ui/bottom-bar'),
    Prompt: require('./lib/ui/prompt')
  };

  /**
   * Create a new self-contained prompt module.
   */

  inquirer.createPromptModule = function(opts) {
    var promptModule = function(questions, allDone) {
      var ui = new inquirer.ui.Prompt(promptModule.prompts, opts);
      ui.run(questions, allDone);
      return ui;
    };

    promptModule.prompts = {};

    /**
     * Register a prompt type
     * @param {String} name     Prompt type name
     * @param {Function} prompt Prompt constructor
     * @return {inquirer}
     */

    promptModule.registerPrompt = function(name, prompt) {
      promptModule.prompts[name] = prompt;
      return this;
    };

    /**
     * Register the defaults provider prompts
     */

    promptModule.restoreDefaultPrompts = function() {
      this.registerPrompt('checkbox', require('./lib/prompts/checkbox'));
      this.registerPrompt('confirm', require('./lib/prompts/confirm'));
      this.registerPrompt('expand', require('./lib/prompts/expand'));
      this.registerPrompt('input', require('./lib/prompts/input'));
      this.registerPrompt('list', require('./lib/prompts/list'));
      this.registerPrompt('password', require('./lib/prompts/password'));
      this.registerPrompt('rawlist', require('./lib/prompts/rawlist'));
    };

    promptModule.restoreDefaultPrompts();
    return promptModule;
  };

  /**
   * Public CLI helper interface
   * @param  {Array|Object|rx.Observable} questions - Questions settings array
   * @param  {Function} cb - Callback being passed the user answers
   * @return {inquirer.ui.Prompt}
   */

  inquirer.prompt = inquirer.createPromptModule(options);

  // Expose helper functions on the top level for easiest usage by common users
  inquirer.registerPrompt = function(name, prompt) {
    inquirer.prompt.registerPrompt(name, prompt);
  };

  inquirer.restoreDefaultPrompts = function() {
    inquirer.prompt.restoreDefaultPrompts();
  };

  return inquirer;
};
