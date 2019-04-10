'use strict';

module.exports = function rand(min, max) {
  //  discuss at: http://locutus.io/php/rand/
  // original by: Leslie Hoare
  // bugfixed by: Onno Marsman (https://twitter.com/onnomarsman)
  //      note 1: See the commented out code below for a version which
  //      note 1: will work with our experimental (though probably unnecessary)
  //      note 1: srand() function)
  //   example 1: rand(1, 1)
  //   returns 1: 1

  var argc = arguments.length;
  if (argc === 0) {
    min = 0;
    max = 2147483647;
  } else if (argc === 1) {
    throw new Error('Warning: rand() expects exactly 2 parameters, 1 given');
  }
  return Math.floor(Math.random() * (max - min + 1)) + min;
};
//# sourceMappingURL=rand.js.map