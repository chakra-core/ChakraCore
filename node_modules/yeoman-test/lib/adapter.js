'use strict';
var events = require('events');
var _ = require('lodash');
var inquirer = require('inquirer');
var sinon = require('sinon');
var Promise = require('pinkie-promise');

function DummyPrompt(answers, q) {
  this.answers = answers;
  this.question = q;
}

DummyPrompt.prototype.run = function () {
  var answer = this.answers[this.question.name];
  var isSet;

  switch (this.question.type) {
    case 'list':
      // list prompt accepts any answer value including null
      isSet = answer !== undefined;
      break;
    case 'confirm':
      // ensure that we don't replace `false` with default `true`
      isSet = answer || answer === false;
      break;
    default:
      // other prompts treat all falsy values to default
      isSet = !!answer;
  }

  if (!isSet) {
    answer = this.question.default;

    if (answer === undefined && this.question.type === 'confirm') {
      answer = true;
    }
  }

  return Promise.resolve(answer);
};

function TestAdapter(answers) {
  answers = answers || {};
  this.promptModule = inquirer.createPromptModule();

  Object.keys(this.promptModule.prompts).forEach(function (promptName) {
    this.promptModule.registerPrompt(promptName, DummyPrompt.bind(DummyPrompt, answers));
  }, this);

  this.diff = sinon.spy();
  this.log = sinon.spy();
  _.extend(this.log, events.EventEmitter.prototype);

  // make sure all log methods are defined
  [
    'write',
    'writeln',
    'ok',
    'error',
    'skip',
    'force',
    'create',
    'invoke',
    'conflict',
    'identical',
    'info',
    'table'
  ].forEach(function (methodName) {
    this.log[methodName] = sinon.stub().returns(this.log);
  }, this);
}

TestAdapter.prototype.prompt = function (questions, cb) {
  var promise = this.promptModule(questions);
  promise.then(cb || _.noop);

  return promise;
};

module.exports = {
  DummyPrompt: DummyPrompt,
  TestAdapter: TestAdapter
};
