/*
 * bower-latest
 * https://github.com/kwakayama/bower-latest
 *
 * Copyright (c) 2014 Kentaro Wakayama
 * Licensed under the MIT license.
 */

'use strict';

var bowerLatest = require('../');

bowerLatest('backbone', function(compontent){
  console.log(compontent.name);
  console.log(compontent.version);
});
