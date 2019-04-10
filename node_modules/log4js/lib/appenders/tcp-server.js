const debug = require('debug')('log4js:tcp-server');
const net = require('net');
const clustering = require('../clustering');
const LoggingEvent = require('../LoggingEvent');

const DELIMITER = '__LOG4JS__';

exports.configure = (config) => {
  debug('configure called with ', config);
  // dummy shutdown if we're not master
  let shutdown = (cb) => { cb(); };

  clustering.onlyOnMaster(() => {
    const server = net.createServer((socket) => {
      let dataSoFar = '';
      const send = (data) => {
        if (data) {
          dataSoFar += data;
          if (dataSoFar.indexOf(DELIMITER)) {
            const events = dataSoFar.split(DELIMITER);
            if (!dataSoFar.endsWith(DELIMITER)) {
              dataSoFar = events.pop();
            } else {
              dataSoFar = '';
            }
            events.filter(e => e.length).forEach((e) => {
              clustering.send(LoggingEvent.deserialise(e));
            });
          }
        }
      };

      socket.setEncoding('utf8');
      socket.on('data', send);
      socket.on('end', send);
    });

    server.listen(config.port || 5000, config.host || 'localhost', () => {
      debug(`listening on ${config.host || 'localhost'}:${config.port || 5000}`);
      server.unref();
    });

    shutdown = (cb) => {
      debug('shutdown called.');
      server.close(cb);
    };
  });

  return {
    shutdown
  };
};
