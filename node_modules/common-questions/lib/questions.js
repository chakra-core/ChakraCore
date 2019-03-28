'use strict';

var hints = require('./hints');
var utils = require('./utils');

module.exports = function(app, options) {
  if (!utils.isValid(app, 'common-questions-messages')) return;

  app.questions.disable('save')
    .set('author.name', 'Author\'s name?', {
      default: hints['author.name'](app)
    })
    .set('author.username', 'Author\'s username?', {
      default: hints['author.username'](app)
    })
    .set('author.twitter', 'Author\'s twitter username?', {
      default: hints['author.twitter'](app)
    })
    .set('author.email', 'Author\'s email address?', {
      default: hints['author.email'](app)
    })
    .set('author.url', 'Author\'s URL?', {
      default: hints['author.url'](app)
    });

  app.questions.disable('save')
    .set('name', 'Project name?', {
      default: hints.name(app)
    })
    .set('description', 'Project description?', {
      default: hints.description(app)
    })
    .set('version', 'Project version?', {
      default: hints.version(app)
    })
    .set('license', 'Project license?', {
      default: hints.license(app)
    })
    .set('alias', 'Project alias (for variables)?', {
      default: hints.alias(app)
    })
    .set('owner', 'Project owner (GitHub username)?', {
      default: hints.owner(app)
    });

  app.questions
    .set('project.name', 'Project name?', {
      default: hints.name(app)
    })
    .set('project.description', 'Project description?', {
      default: hints.description(app)
    })
    .set('project.version', 'Project version?', {
      default: hints.version(app)
    })
    .set('project.license', 'Project license?', {
      default: hints.license(app)
    })
    .set('project.alias', 'Project alias (for variables)?', {
      default: hints.alias(app)
    })
    .set('project.owner', 'Project owner (GitHub username)?', {
      default: hints.owner(app)
    });
};
