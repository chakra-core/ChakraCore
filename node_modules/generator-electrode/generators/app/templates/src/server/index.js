"use strict";

/* eslint-disable global-require */

process.on("SIGINT", () => {
  process.exit(0);
});

const electrodeConfippet = require("electrode-confippet");
const support = require("electrode-archetype-react-app/support");

//<% if (isHapi) { %>
const staticPathsDecor = require("electrode-static-paths");
const electrodeServer = require("electrode-server");

//
// sample to show electrode server startup events
// https://github.com/electrode-io/electrode-server#listener-function
//
function setupElectrodeServerEvents(emitter) {
  emitter.on("config-composed", (data, next) => next());
  emitter.on("server-created", (data, next) => next());
  emitter.on("connections-set", (data, next) => next());
  emitter.on("plugins-sorted", (data, next) => next());
  emitter.on("plugins-registered", (data, next) => next());
  emitter.on("server-started", (data, next) => next());
  emitter.on("complete", (data, next) => next());
}
//<% } %>

const startServer = config => {
  //<% if (isHapi) { %>
  const decor = staticPathsDecor();
  if (!config.listener) config.listener = setupElectrodeServerEvents;
  return electrodeServer(config, [decor]);
  //<% } else if (isExpress) { %>
  return require("./express-server")(config);
  //<% } else { %>
  return require("./koa-server")(config);
  //<% } %>
};

module.exports = () =>
  support.load().then(() => {
    const config = electrodeConfippet.config;
    return startServer(config);
  });

if (require.main === module) {
  module.exports();
}
