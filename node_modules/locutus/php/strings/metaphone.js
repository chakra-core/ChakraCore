'use strict';

var _typeof = typeof Symbol === "function" && typeof Symbol.iterator === "symbol" ? function (obj) { return typeof obj; } : function (obj) { return obj && typeof Symbol === "function" && obj.constructor === Symbol && obj !== Symbol.prototype ? "symbol" : typeof obj; };

module.exports = function metaphone(word, maxPhonemes) {
  //  discuss at: http://locutus.io/php/metaphone/
  // original by: Greg Frazier
  // improved by: Brett Zamir (http://brett-zamir.me)
  // improved by: Rafał Kukawski (http://blog.kukawski.pl)
  //   example 1: metaphone('Gnu')
  //   returns 1: 'N'
  //   example 2: metaphone('bigger')
  //   returns 2: 'BKR'
  //   example 3: metaphone('accuracy')
  //   returns 3: 'AKKRS'
  //   example 4: metaphone('batch batcher')
  //   returns 4: 'BXBXR'

  var type = typeof word === 'undefined' ? 'undefined' : _typeof(word);

  if (type === 'undefined' || type === 'object' && word !== null) {
    // weird!
    return null;
  }

  // infinity and NaN values are treated as strings
  if (type === 'number') {
    if (isNaN(word)) {
      word = 'NAN';
    } else if (!isFinite(word)) {
      word = 'INF';
    }
  }

  if (maxPhonemes < 0) {
    return false;
  }

  maxPhonemes = Math.floor(+maxPhonemes) || 0;

  // alpha depends on locale, so this var might need an update
  // or should be turned into a regex
  // for now assuming pure a-z
  var alpha = 'ABCDEFGHIJKLMNOPQRSTUVWXYZ';
  var vowel = 'AEIOU';
  var soft = 'EIY';
  var leadingNonAlpha = new RegExp('^[^' + alpha + ']+');

  word = typeof word === 'string' ? word : '';
  word = word.toUpperCase().replace(leadingNonAlpha, '');

  if (!word) {
    return '';
  }

  var is = function is(p, c) {
    return c !== '' && p.indexOf(c) !== -1;
  };

  var i = 0;
  var cc = word.charAt(0); // current char. Short name because it's used all over the function
  var nc = word.charAt(1); // next char
  var nnc; // after next char
  var pc; // previous char
  var l = word.length;
  var meta = '';
  // traditional is an internal param that could be exposed for now let it be a local var
  var traditional = true;

  switch (cc) {
    case 'A':
      meta += nc === 'E' ? nc : cc;
      i += 1;
      break;
    case 'G':
    case 'K':
    case 'P':
      if (nc === 'N') {
        meta += nc;
        i += 2;
      }
      break;
    case 'W':
      if (nc === 'R') {
        meta += nc;
        i += 2;
      } else if (nc === 'H' || is(vowel, nc)) {
        meta += 'W';
        i += 2;
      }
      break;
    case 'X':
      meta += 'S';
      i += 1;
      break;
    case 'E':
    case 'I':
    case 'O':
    case 'U':
      meta += cc;
      i++;
      break;
  }

  for (; i < l && (maxPhonemes === 0 || meta.length < maxPhonemes); i += 1) {
    // eslint-disable-line no-unmodified-loop-condition,max-len
    cc = word.charAt(i);
    nc = word.charAt(i + 1);
    pc = word.charAt(i - 1);
    nnc = word.charAt(i + 2);

    if (cc === pc && cc !== 'C') {
      continue;
    }

    switch (cc) {
      case 'B':
        if (pc !== 'M') {
          meta += cc;
        }
        break;
      case 'C':
        if (is(soft, nc)) {
          if (nc === 'I' && nnc === 'A') {
            meta += 'X';
          } else if (pc !== 'S') {
            meta += 'S';
          }
        } else if (nc === 'H') {
          meta += !traditional && (nnc === 'R' || pc === 'S') ? 'K' : 'X';
          i += 1;
        } else {
          meta += 'K';
        }
        break;
      case 'D':
        if (nc === 'G' && is(soft, nnc)) {
          meta += 'J';
          i += 1;
        } else {
          meta += 'T';
        }
        break;
      case 'G':
        if (nc === 'H') {
          if (!(is('BDH', word.charAt(i - 3)) || word.charAt(i - 4) === 'H')) {
            meta += 'F';
            i += 1;
          }
        } else if (nc === 'N') {
          if (is(alpha, nnc) && word.substr(i + 1, 3) !== 'NED') {
            meta += 'K';
          }
        } else if (is(soft, nc) && pc !== 'G') {
          meta += 'J';
        } else {
          meta += 'K';
        }
        break;
      case 'H':
        if (is(vowel, nc) && !is('CGPST', pc)) {
          meta += cc;
        }
        break;
      case 'K':
        if (pc !== 'C') {
          meta += 'K';
        }
        break;
      case 'P':
        meta += nc === 'H' ? 'F' : cc;
        break;
      case 'Q':
        meta += 'K';
        break;
      case 'S':
        if (nc === 'I' && is('AO', nnc)) {
          meta += 'X';
        } else if (nc === 'H') {
          meta += 'X';
          i += 1;
        } else if (!traditional && word.substr(i + 1, 3) === 'CHW') {
          meta += 'X';
          i += 2;
        } else {
          meta += 'S';
        }
        break;
      case 'T':
        if (nc === 'I' && is('AO', nnc)) {
          meta += 'X';
        } else if (nc === 'H') {
          meta += '0';
          i += 1;
        } else if (word.substr(i + 1, 2) !== 'CH') {
          meta += 'T';
        }
        break;
      case 'V':
        meta += 'F';
        break;
      case 'W':
      case 'Y':
        if (is(vowel, nc)) {
          meta += cc;
        }
        break;
      case 'X':
        meta += 'KS';
        break;
      case 'Z':
        meta += 'S';
        break;
      case 'F':
      case 'J':
      case 'L':
      case 'M':
      case 'N':
      case 'R':
        meta += cc;
        break;
    }
  }

  return meta;
};
//# sourceMappingURL=metaphone.js.map