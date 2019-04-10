# yeoman-option-or-prompt
Provides a helper for yeoman generators that wraps the prompt method with code that will exclude any prompts for data that has been supplied via options.

To install use

``` bash
npm install yeoman-option-or-prompt --save
```

To use it, just add optionOrPrompt as an internal function on your generator and call it in place of the yeoman-generator `prompt` method:

``` javascript
var yeoman = require('yeoman-generator');
var optionOrPrompt = require('yeoman-option-or-prompt');

module.exports = yeoman.generators.Base.extend({

  _optionOrPrompt: optionOrPrompt,

  prompting: function () {
    var done = this.async();
    // Instead of calling prompt, call _optionOrPrompt to allow parameters to be passed as command line or composeWith options.
    this._optionOrPrompt([{
      type    : 'input',
      name    : 'name',
      message : 'Your project name',
      default : this.appname // Default to current folder name
    }], function (answers) {
      this.log(answers.name);
      done();
    }.bind(this));
  }
})
```

You can also pass options to subgenerators, with:

``` javascript
  compose: function () {
    this.composeWith('node-webkit:download', { options: this.options });
    if(this.examples){
      this.composeWith('node-webkit:examples', { options: this.options });
    }
  }
```

Now if another app calls your generator, they can supply values for each of your prompts with the options parameter. Alternatively, a user can supply values for your prompts on the command line:

``` bash
yo single-page-app --name="App that saves the world."
```
