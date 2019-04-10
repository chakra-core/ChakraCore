'use strict';
const chalk = require('chalk');
const pad = require('pad-component');
const wrap = require('wrap-ansi');
const stringWidth = require('string-width');
const stripAnsi = require('strip-ansi');
const ansiStyles = require('ansi-styles');
const ansiRegex = require('ansi-regex')();
const cliBoxes = require('cli-boxes');

const border = cliBoxes.round;
const leftOffset = 17;
const defaultGreeting =
  '\n     _-----_     ' +
  '\n    |       |    ' +
  '\n    |' + chalk.red('--(o)--') + '|    ' +
  '\n   `---------´   ' +
  '\n    ' + chalk.yellow('(') + ' _' + chalk.yellow('´U`') + '_ ' + chalk.yellow(')') + '    ' +
  '\n    /___A___\\   /' +
  '\n     ' + chalk.yellow('|  ~  |') + '     ' +
  '\n   __' + chalk.yellow('\'.___.\'') + '__   ' +
  '\n ´   ' + chalk.red('`  |') + '° ' + chalk.red('´ Y') + ' ` ';

module.exports = (message, options) => {
  message = (message || 'Welcome to Yeoman, ladies and gentlemen!').trim();
  options = options || {};

  /*
   * What you're about to see may confuse you. And rightfully so. Here's an
   * explanation.
   *
   * When yosay is given a string, we create a duplicate with the ansi styling
   * sucked out. This way, the true length of the string is read by `pad` and
   * `wrap`, so they can correctly do their job without getting tripped up by
   * the "invisible" ansi. Along with the duplicated, non-ansi string, we store
   * the character position of where the ansi was, so that when we go back over
   * each line that will be printed out in the message box, we check the
   * character position to see if it needs any styling, then re-insert it if
   * necessary.
   *
   * Better implementations welcome :)
   */

  let maxLength = 24;
  const styledIndexes = {};
  let completedString = '';
  let topOffset = 4;

  // Amount of characters of the yeoman character »column«      → `    /___A___\   /`
  const YEOMAN_CHARACTER_WIDTH = 17;

  // Amount of characters of the default top frame of the speech bubble → `╭──────────────────────────╮`
  const DEFAULT_TOP_FRAME_WIDTH = 28;

  // Amount of characters of a total line
  let TOTAL_CHARACTERS_PER_LINE = YEOMAN_CHARACTER_WIDTH + DEFAULT_TOP_FRAME_WIDTH;

  // The speech bubble will overflow the Yeoman character if the message is too long.
  const MAX_MESSAGE_LINES_BEFORE_OVERFLOW = 7;

  if (options.maxLength) {
    maxLength = stripAnsi(message).toLowerCase().split(' ').sort()[0].length;

    if (maxLength < options.maxLength) {
      maxLength = options.maxLength;
      TOTAL_CHARACTERS_PER_LINE = maxLength + YEOMAN_CHARACTER_WIDTH + topOffset;
    }
  }

  const regExNewLine = new RegExp(`\\s{${maxLength}}`);
  const borderHorizontal = border.horizontal.repeat(maxLength + 2);

  const frame = {
    top: border.topLeft + borderHorizontal + border.topRight,
    side: ansiStyles.reset.open + border.vertical + ansiStyles.reset.open,
    bottom: ansiStyles.reset.open + border.bottomLeft + borderHorizontal + border.bottomRight
  };

  message.replace(ansiRegex, (match, offset) => {
    Object.keys(styledIndexes).forEach(key => {
      offset -= styledIndexes[key].length;
    });

    styledIndexes[offset] = styledIndexes[offset] ? styledIndexes[offset] + match : match;
  });

  const strippedMessage = stripAnsi(message);
  const spacesIndex = [];

  strippedMessage.split(' ').reduce((accu, cur) => {
    spacesIndex.push(accu + cur.length);
    return spacesIndex[spacesIndex.length - 1] + 1;
  }, 0);

  return wrap(strippedMessage, maxLength, {hard: true})
    .split(/\n/)
    .reduce((greeting, str, index, array) => {
      if (!regExNewLine.test(str)) {
        str = str.trim();
      }

      completedString += str;

      let offset = 0;

      for (let i = 0; i < spacesIndex.length; i++) {
        let char = completedString[spacesIndex[i] - offset];
        if (char) {
          if (char !== ' ') {
            offset += 1;
          }
        } else {
          break;
        }
      }

      str = completedString
        .substr(completedString.length - str.length)
        .replace(/./g, (char, charIndex) => {
          charIndex += completedString.length - str.length + offset;

          let hasContinuedStyle = 0;
          let continuedStyle;

          Object.keys(styledIndexes).forEach(offset => {
            if (charIndex > offset) {
              hasContinuedStyle++;
              continuedStyle = styledIndexes[offset];
            }

            if (hasContinuedStyle === 1 && charIndex < offset) {
              hasContinuedStyle++;
            }
          });

          if (styledIndexes[charIndex]) {
            return styledIndexes[charIndex] + char;
          } else if (hasContinuedStyle >= 2) {
            return continuedStyle + char;
          }

          return char;
        })
        .trim();

      const paddedString = pad({
        length: stringWidth(str),
        valueOf() {
          return ansiStyles.reset.open + str + ansiStyles.reset.open;
        }
      }, maxLength);

      if (index === 0) {
        // Need to adjust the top position of the speech bubble depending on the
        // amount of lines of the message.
        if (array.length === 2) {
          topOffset -= 1;
        }

        if (array.length >= 3) {
          topOffset -= 2;
        }

        // The speech bubble will overflow the Yeoman character if the message
        // is too long. So we vertically center the bubble by adding empty lines
        // on top of the greeting.
        if (array.length > MAX_MESSAGE_LINES_BEFORE_OVERFLOW) {
          const emptyLines = Math.ceil((array.length - MAX_MESSAGE_LINES_BEFORE_OVERFLOW) / 2);

          for (let i = 0; i < emptyLines; i++) {
            greeting.unshift('');
          }

          frame.top = pad.left(frame.top, TOTAL_CHARACTERS_PER_LINE);
        }

        greeting[topOffset - 1] += frame.top;
      }

      greeting[index + topOffset] =
        (greeting[index + topOffset] || pad.left('', leftOffset)) +
        frame.side + ' ' + paddedString + ' ' + frame.side;

      if (array.length === index + 1) {
        greeting[index + topOffset + 1] =
          (greeting[index + topOffset + 1] || pad.left('', leftOffset)) +
          frame.bottom;
      }

      return greeting;
    }, defaultGreeting.split(/\n/))
    .join('\n') + '\n';
};
