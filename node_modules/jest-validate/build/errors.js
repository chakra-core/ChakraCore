/**
 * Copyright (c) 2014-present, Facebook, Inc. All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 *
 * 
 */

'use strict';



const chalk = require('chalk');var _require =
require('./utils');const format = _require.format,ValidationError = _require.ValidationError,ERROR = _require.ERROR;var _require2 =
require('jest-matcher-utils');const getType = _require2.getType;

const errorMessage = (
option,
received,
defaultValue,
options) =>
{
  const message =
  `  Option ${chalk.bold(`"${option}"`)} must be of type:
    ${chalk.bold.green(getType(defaultValue))}
  but instead received:
    ${chalk.bold.red(getType(received))}

  Example:
  {
    ${chalk.bold(`"${option}"`)}: ${chalk.bold(format(defaultValue))}
  }`;

  const comment = options.comment;
  const name = options.title && options.title.error || ERROR;

  throw new ValidationError(name, message, comment);
};

module.exports = {
  ValidationError,
  errorMessage };