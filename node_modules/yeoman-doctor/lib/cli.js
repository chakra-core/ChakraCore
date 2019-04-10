#!/usr/bin/env node
'use strict';
require('.')();

// Override http networking to go through a proxy if one is configured.
require('global-tunnel-ng').initialize();
