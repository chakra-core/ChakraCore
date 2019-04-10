#! /usr/bin/env node

var tabtab = require('../..')({
  name: 'yo'
});

tabtab.on('yo', require('./complete-generators'));

tabtab.start();
