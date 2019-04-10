'use strict';

module.exports = {
  description: 'Generates a blueprint and definition.',

  files() {
    let files = this._super.files.apply(this, arguments);

    if (!this.hasJSHint()) {
      files = files.filter(file => file !== 'blueprints/.jshintrc');
    }

    return files;
  },

  hasJSHint() {
    if (this.project) {
      return 'ember-cli-jshint' in this.project.dependencies();
    }
  },
};
