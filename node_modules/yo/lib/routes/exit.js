'use strict';
const yosay = require('yosay');

module.exports = app => {
  app.insight.track('yoyo', 'exit');

  const PADDING = 5;
  const url = 'http://yeoman.io';
  const maxLength = url.length + PADDING;
  const newLine = ' '.repeat(maxLength);

  console.log(yosay(
    'Bye from us!' +
    newLine +
    'Chat soon.' +
    newLine +
    'Yeoman team ' + url,
    {maxLength}
  ));
};
