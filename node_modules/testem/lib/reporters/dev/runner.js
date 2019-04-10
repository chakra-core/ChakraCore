'use strict';

const Backbone = require('backbone');
const TestResults = require('./test_results');

module.exports = Backbone.Model.extend({
  initialize(runner) {
    this.set({
      messages: new Backbone.Collection(),
      results: new TestResults(),
      runner: runner,
      name: runner.name()
    });
  },

  report(result) {
    this.get('results').addResult(result);
  },

  onStart() {
    this.get('results').reset();
    this.get('messages').reset();
    this.set('allPassed', undefined);
    this.trigger('tests-start');
  },

  onEnd() {
    this.get('results').set('all', true);
    this.trigger('tests-end');
  },

  hasResults() {
    return this.get('results').get('total') > 0;
  },

  hasMessages() {
    return this.get('messages').length > 0;
  }
});
