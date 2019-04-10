'use strict';

const Command = require('../models/command');
const RSVP = require('rsvp');
const SilentError = require('silent-error');
const PortFinder = require('portfinder');
const Win = require('../utilities/windows-admin');
const EOL = require('os').EOL;

const Promise = RSVP.Promise;

PortFinder.basePort = 7020;

let getPort = RSVP.denodeify(PortFinder.getPort);
let defaultPort = process.env.PORT || 4200;

module.exports = Command.extend({
  name: 'serve',
  description: 'Builds and serves your app, rebuilding on file changes.',
  aliases: ['server', 's'],

  availableOptions: [
    { name: 'port',                 type: Number,  default: defaultPort,   aliases: ['p'],                                                     description: `To use a port different than ${defaultPort}. Pass 0 to automatically pick an available port.` },
    { name: 'host',                 type: String,                          aliases: ['H'],                                                     description: 'Listens on all interfaces by default' },
    { name: 'proxy',                type: String,                          aliases: ['pr', 'pxy'] },
    { name: 'proxy-in-timeout',     type: Number,  default: 120000,        aliases: ['pit'],                                                   description: 'When using --proxy: timeout (in ms) for incoming requests' },
    { name: 'proxy-out-timeout',    type: Number,  default: 0,             aliases: ['pot'],                                                   description: 'When using --proxy: timeout (in ms) for outgoing requests' },
    { name: 'secure-proxy',         type: Boolean, default: true,          aliases: ['spr'],                                                   description: 'Set to false to proxy self-signed SSL certificates' },
    { name: 'transparent-proxy',    type: Boolean, default: true,          aliases: ['transp'],                                                description: 'Set to false to omit x-forwarded-* headers when proxying' },
    { name: 'watcher',              type: String,  default: 'events',      aliases: ['w'] },
    { name: 'live-reload',          type: Boolean, default: true,          aliases: ['lr'] },
    { name: 'live-reload-host',     type: String,                          aliases: ['lrh'],                                                   description: 'Defaults to host' },
    { name: 'live-reload-base-url', type: String,                          aliases: ['lrbu'],                                                  description: 'Defaults to baseURL' },
    { name: 'live-reload-port',     type: Number,                          aliases: ['lrp'],                                                   description: 'Defaults to same port as ember app' },
    { name: 'live-reload-prefix',   type: String,  default: '_lr',         aliases: ['lrprefix'],                                              description: 'Default to _lr' },
    { name: 'environment',          type: String,  default: 'development', aliases: ['e', { 'dev': 'development' }, { 'prod': 'production' }], description: 'Possible values are "development", "production", and "test".' },
    { name: 'output-path',          type: 'Path',  default: 'dist/',       aliases: ['op', 'out'] },
    { name: 'ssl',                  type: Boolean, default: false,                                                                             description: 'Set to true to configure Ember CLI to serve using SSL.' },
    { name: 'ssl-key',              type: String,  default: 'ssl/server.key',                                                                  description: 'Specify the private key to use for SSL.' },
    { name: 'ssl-cert',             type: String,  default: 'ssl/server.crt',                                                                  description: 'Specify the certificate to use for SSL.' },
    { name: 'path',                 type: 'Path',                                                                                              description: 'Reuse an existing build at given path.' },
  ],

  run(commandOptions) {
    commandOptions.liveReloadHost = commandOptions.liveReloadHost || commandOptions.host;

    return this._checkExpressPort(commandOptions)
      .then(commandOptions => {
        if (commandOptions.proxy) {
          if (!(/^(http:|https:)/).test(commandOptions.proxy)) {
            let message = `You need to include a protocol with the proxy URL.${EOL}Try --proxy http://${commandOptions.proxy}`;

            return Promise.reject(new SilentError(message));
          }
        }

        return Win.checkIfSymlinksNeedToBeEnabled(this.ui)
          .then(() => this.runTask('Serve', commandOptions));
      });
  },

  _checkExpressPort(commandOptions) {
    return getPort({ port: commandOptions.port, host: commandOptions.host }).then(foundPort => {

      if (commandOptions.port !== foundPort && commandOptions.port !== 0) {
        let message = `Port ${commandOptions.port} is already in use.`;
        return Promise.reject(new SilentError(message));
      }

      // otherwise, our found port is good
      commandOptions.port = foundPort;
      commandOptions.liveReloadPort = commandOptions.liveReloadPort || commandOptions.port;
      return commandOptions;
    });
  },
});
