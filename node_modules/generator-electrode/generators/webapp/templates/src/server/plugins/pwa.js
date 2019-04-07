"use strict";

/* eslint-env es6 */

const plugin = {
  pkg: {
    name: "PWAPlugin",
    version: "0.0.1"
  }
};

plugin.register = function(server) {
  server.route({
    method: "GET",
    path: "/sw.js",
    handler: {
      file: "dist/sw.js"
    }
  });
};

module.exports = plugin;
