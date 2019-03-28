/*!
 * ansi-colors <https://github.com/doowb/ansi-colors>
 *
 * Copyright (c) 2015, Brian Woodward.
 * Licensed under the MIT License.
 */

'use strict';

/**
 * Module dependencies
 */

var colors = require('lazy-cache')(require);

/**
 * Temporarily re-assign `require` to trick browserify and
 * webpack into reconizing lazy dependencies.
 *
 * This tiny bit of ugliness has the huge dual advantage of
 * only loading modules that are actually called at some
 * point in the lifecycle of the application, whilst also
 * allowing browserify and webpack to find modules that
 * are depended on but never actually called.
 */

var fn = require;
require = colors;

/**
 * Lazily required module dependencies
 */

/**
 * Wrap a string with ansi codes to create a black background.
 *
 * ```js
 * console.log(colors.bgblack('some string'));
 * ```
 *
 * @param  {String} `str` String to wrap with ansi codes.
 * @return {String} Wrapped string
 * @api public
 * @name  bgblack
 */

require('ansi-bgblack', 'bgblack');

/**
 * Wrap a string with ansi codes to create a blue background.
 *
 * ```js
 * console.log(colors.bgblue('some string'));
 * ```
 *
 * @param  {String} `str` String to wrap with ansi codes.
 * @return {String} Wrapped string
 * @api public
 * @name  bgblue
 */

require('ansi-bgblue', 'bgblue');

/**
 * Wrap a string with ansi codes to create a cyan background.
 *
 * ```js
 * console.log(colors.bgcyan('some string'));
 * ```
 *
 * @param  {String} `str` String to wrap with ansi codes.
 * @return {String} Wrapped string
 * @api public
 * @name  bgcyan
 */

require('ansi-bgcyan', 'bgcyan');

/**
 * Wrap a string with ansi codes to create a green background.
 *
 * ```js
 * console.log(colors.bggreen('some string'));
 * ```
 *
 * @param  {String} `str` String to wrap with ansi codes.
 * @return {String} Wrapped string
 * @api public
 * @name  bggreen
 */

require('ansi-bggreen', 'bggreen');

/**
 * Wrap a string with ansi codes to create a magenta background.
 *
 * ```js
 * console.log(colors.bgmagenta('some string'));
 * ```
 *
 * @param  {String} `str` String to wrap with ansi codes.
 * @return {String} Wrapped string
 * @api public
 * @name  bgmagenta
 */

require('ansi-bgmagenta', 'bgmagenta');

/**
 * Wrap a string with ansi codes to create a red background.
 *
 * ```js
 * console.log(colors.bgred('some string'));
 * ```
 *
 * @param  {String} `str` String to wrap with ansi codes.
 * @return {String} Wrapped string
 * @api public
 * @name  bgred
 */

require('ansi-bgred', 'bgred');

/**
 * Wrap a string with ansi codes to create a white background.
 *
 * ```js
 * console.log(colors.bgwhite('some string'));
 * ```
 *
 * @param  {String} `str` String to wrap with ansi codes.
 * @return {String} Wrapped string
 * @api public
 * @name  bgwhite
 */

require('ansi-bgwhite', 'bgwhite');

/**
 * Wrap a string with ansi codes to create a yellow background.
 *
 * ```js
 * console.log(colors.bgyellow('some string'));
 * ```
 *
 * @param  {String} `str` String to wrap with ansi codes.
 * @return {String} Wrapped string
 * @api public
 * @name  bgyellow
 */

require('ansi-bgyellow', 'bgyellow');

/**
 * Wrap a string with ansi codes to create black text.
 *
 * ```js
 * console.log(colors.black('some string'));
 * ```
 *
 * @param  {String} `str` String to wrap with ansi codes.
 * @return {String} Wrapped string
 * @api public
 * @name  black
 */

require('ansi-black', 'black');

/**
 * Wrap a string with ansi codes to create blue text.
 *
 * ```js
 * console.log(colors.blue('some string'));
 * ```
 *
 * @param  {String} `str` String to wrap with ansi codes.
 * @return {String} Wrapped string
 * @api public
 * @name  blue
 */

require('ansi-blue', 'blue');

/**
 * Wrap a string with ansi codes to create bold text.
 *
 * ```js
 * console.log(colors.bold('some string'));
 * ```
 *
 * @param  {String} `str` String to wrap with ansi codes.
 * @return {String} Wrapped string
 * @api public
 * @name  bold
 */

require('ansi-bold', 'bold');

/**
 * Wrap a string with ansi codes to create cyan text.
 *
 * ```js
 * console.log(colors.cyan('some string'));
 * ```
 *
 * @param  {String} `str` String to wrap with ansi codes.
 * @return {String} Wrapped string
 * @api public
 * @name  cyan
 */

require('ansi-cyan', 'cyan');

/**
 * Wrap a string with ansi codes to create dim text.
 *
 * ```js
 * console.log(colors.dim('some string'));
 * ```
 *
 * @param  {String} `str` String to wrap with ansi codes.
 * @return {String} Wrapped string
 * @api public
 * @name  dim
 */

require('ansi-dim', 'dim');

/**
 * Wrap a string with ansi codes to create gray text.
 *
 * ```js
 * console.log(colors.gray('some string'));
 * ```
 *
 * @param  {String} `str` String to wrap with ansi codes.
 * @return {String} Wrapped string
 * @api public
 * @name  gray
 */

require('ansi-gray', 'gray');

/**
 * Wrap a string with ansi codes to create green text.
 *
 * ```js
 * console.log(colors.green('some string'));
 * ```
 *
 * @param  {String} `str` String to wrap with ansi codes.
 * @return {String} Wrapped string
 * @api public
 * @name  green
 */

require('ansi-green', 'green');

/**
 * Wrap a string with ansi codes to create grey text.
 *
 * ```js
 * console.log(colors.grey('some string'));
 * ```
 *
 * @param  {String} `str` String to wrap with ansi codes.
 * @return {String} Wrapped string
 * @api public
 * @name  grey
 */

require('ansi-grey', 'grey');

/**
 * Wrap a string with ansi codes to create hidden text.
 *
 * ```js
 * console.log(colors.hidden('some string'));
 * ```
 *
 * @param  {String} `str` String to wrap with ansi codes.
 * @return {String} Wrapped string
 * @api public
 * @name  hidden
 */

require('ansi-hidden', 'hidden');

/**
 * Wrap a string with ansi codes to create inverse text.
 *
 * ```js
 * console.log(colors.inverse('some string'));
 * ```
 *
 * @param  {String} `str` String to wrap with ansi codes.
 * @return {String} Wrapped string
 * @api public
 * @name  inverse
 */

require('ansi-inverse', 'inverse');

/**
 * Wrap a string with ansi codes to create italic text.
 *
 * ```js
 * console.log(colors.italic('some string'));
 * ```
 *
 * @param  {String} `str` String to wrap with ansi codes.
 * @return {String} Wrapped string
 * @api public
 * @name  italic
 */

require('ansi-italic', 'italic');

/**
 * Wrap a string with ansi codes to create magenta text.
 *
 * ```js
 * console.log(colors.magenta('some string'));
 * ```
 *
 * @param  {String} `str` String to wrap with ansi codes.
 * @return {String} Wrapped string
 * @api public
 * @name  magenta
 */

require('ansi-magenta', 'magenta');

/**
 * Wrap a string with ansi codes to create red text.
 *
 * ```js
 * console.log(colors.red('some string'));
 * ```
 *
 * @param  {String} `str` String to wrap with ansi codes.
 * @return {String} Wrapped string
 * @api public
 * @name  red
 */

require('ansi-red', 'red');

/**
 * Wrap a string with ansi codes to reset ansi colors currently on the string.
 *
 * ```js
 * console.log(colors.reset('some string'));
 * ```
 *
 * @param  {String} `str` String to wrap with ansi codes.
 * @return {String} Wrapped string
 * @api public
 * @name  reset
 */

require('ansi-reset', 'reset');

/**
 * Wrap a string with ansi codes to add a strikethrough to the text.
 *
 * ```js
 * console.log(colors.strikethrough('some string'));
 * ```
 *
 * @param  {String} `str` String to wrap with ansi codes.
 * @return {String} Wrapped string
 * @api public
 * @name  strikethrough
 */

require('ansi-strikethrough', 'strikethrough');

/**
 * Wrap a string with ansi codes to underline the text.
 *
 * ```js
 * console.log(colors.underline('some string'));
 * ```
 *
 * @param  {String} `str` String to wrap with ansi codes.
 * @return {String} Wrapped string
 * @api public
 * @name  underline
 */

require('ansi-underline', 'underline');

/**
 * Wrap a string with ansi codes to create white text.
 *
 * ```js
 * console.log(colors.white('some string'));
 * ```
 *
 * @param  {String} `str` String to wrap with ansi codes.
 * @return {String} Wrapped string
 * @api public
 * @name  white
 */

require('ansi-white', 'white');

/**
 * Wrap a string with ansi codes to create yellow text.
 *
 * ```js
 * console.log(colors.yellow('some string'));
 * ```
 *
 * @param  {String} `str` String to wrap with ansi codes.
 * @return {String} Wrapped string
 * @api public
 * @name  yellow
 */

require('ansi-yellow', 'yellow');

/**
 * Restore `require`
 */

require = fn;

/**
 * Expose `colors` modules
 */

module.exports = colors;
