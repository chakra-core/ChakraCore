#! /usr/bin/env node
'use strict';
const env = require('yeoman-environment').createEnv();
const tabtab = require('tabtab')({
  name: 'yo',
  cache: !process.env.YO_TEST
});
const Completer = require('./completer');

const completer = new Completer(env);

tabtab.completer = completer;

// Lookup installed generator in yeoman environment,
// respond completion results with each generator
tabtab.on('yo', completer.complete.bind(completer));

// Register complete command
tabtab.start();

module.exports = tabtab;
