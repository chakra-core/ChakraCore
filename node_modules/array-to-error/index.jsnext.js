/*!
 * array-to-error | MIT (c) Shinnosuke Watanabe
 * https://github.com/shinnn/array-to-error
*/
import arrayToSentence from 'array-to-sentence';

function isNotString(item) {
  return typeof item !== 'string';
}

export default function arrayToError(arr, ErrorConstructor) {
  if (!Array.isArray(arr)) {
    throw new TypeError(
      arr +
      ' is not an array. Expected an array of error messages.'
    );
  }

  var nonStringValues = arr.filter(isNotString);
  if (nonStringValues.length !== 0) {
    var isPrural = nonStringValues.length > 1;
    throw new TypeError(
      arrayToSentence(nonStringValues) +
      ' ' +
      (isPrural ? 'are' : 'is') +
      ' not ' +
      (isPrural ? 'strings' : 'a string') +
      '. Expected every item in the array is an error message string.'
    );
  }

  if (ErrorConstructor !== undefined) {
    if (typeof ErrorConstructor !== 'function') {
      throw new TypeError(
        ErrorConstructor +
        ' is not a function. Expected an error constructor.'
      );
    }
  } else {
    ErrorConstructor = Error;
  }

  var error = new ErrorConstructor(arr.join('\n'));
  error.reasons = arr;

  return error;
}
