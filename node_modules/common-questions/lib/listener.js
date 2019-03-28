
var utils = require('./utils');
var hints = require('./hints');
var answered = {};

module.exports = function listener(app, options) {
  if (!utils.isValid(app, 'common-questions-listener')) return;

  app.on('ask', function(val, key, question, answers) {
    if (app.option('hints') !== false) {
      question.default = app.questions.hints.get(key);
    }

    var answer = utils.get(answered, key);
    var hint = utils.get(hints, key);

    if (typeof answer === 'undefined' && typeof hint === 'function') {
      answer = hint(app);
    }

    if (typeof answer !== 'undefined') {
      app.base.set(['options.question.skip', key], true);
      app.base.set(['cache.answers', key], answer);
      utils.set(answered, key, answer);
      utils.set(answers, key, answer);
      question.default = answer;
    }
  });

  app.on('answer', function(val, key, question, answers) {
    if (typeof val !== 'undefined') {
      if (app.option('hints') !== false && typeof app.questions.hints !== 'undefined') {
        app.questions.hints.set(key, val);
      }
      app.base.set(['cache.answers', key], val);
      utils.set(answered, key, val);
      utils.set(answers, key, val);
      question.default = val;
    }
  });
};
