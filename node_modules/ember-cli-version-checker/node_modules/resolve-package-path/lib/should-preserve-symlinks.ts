'use strict';

function includes(array: [string], entry: string) {
  let result = false;

  for (let i =0; i < array.length; i++) {
    if (array[i] === entry) {
      return true;
    }
  }
  return false;
}
/*
 * utility to detect if node is respective symlinks or not
 */
export = function(process: any) {
  return !!process.env.NODE_PRESERVE_SYMLINKS || includes(process.execArgv, '--preserve-symlinks');
};
