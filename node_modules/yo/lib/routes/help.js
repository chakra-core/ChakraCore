'use strict';
const inquirer = require('inquirer');
const opn = require('opn');

module.exports = app => {
  app.insight.track('yoyo', 'help');

  return inquirer.prompt([{
    name: 'whereTo',
    type: 'list',
    message: 'Here are a few helpful resources.\n\nI will open the link you select in your browser for you',
    choices: [{
      name: 'Take me to the documentation',
      value: 'http://yeoman.io/learning/'
    }, {
      name: 'View Frequently Asked Questions',
      value: 'http://yeoman.io/learning/faq.html'
    }, {
      name: 'File an issue on GitHub',
      value: 'http://yeoman.io/contributing/opening-issues.html'
    }, {
      name: 'Take me back home, Yo!',
      value: 'home'
    }]
  }]).then(answer => {
    app.insight.track('yoyo', 'help', answer);

    if (answer.whereTo === 'home') {
      console.log('I get it, you like learning on your own. I respect that.');
      app.navigate('home');
      return;
    }

    opn(answer.whereTo);
  });
};
