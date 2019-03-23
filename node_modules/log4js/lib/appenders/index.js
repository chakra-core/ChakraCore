const path = require('path');
const debug = require('debug')('log4js:appenders');
const configuration = require('../configuration');
const clustering = require('../clustering');
const levels = require('../levels');
const layouts = require('../layouts');
const adapters = require('./adapters');

// pre-load the core appenders so that webpack can find them
const coreAppenders = new Map();
coreAppenders.set('console', require('./console'));
coreAppenders.set('stdout', require('./stdout'));
coreAppenders.set('stderr', require('./stderr'));
coreAppenders.set('file', require('./file'));
coreAppenders.set('dateFile', require('./dateFile'));

const appenders = new Map();

const tryLoading = (modulePath, config) => {
  debug('Loading module from ', modulePath);
  try {
    return require(modulePath); //eslint-disable-line
  } catch (e) {
    // if the module was found, and we still got an error, then raise it
    configuration.throwExceptionIf(
      config,
      e.code !== 'MODULE_NOT_FOUND',
      `appender "${modulePath}" could not be loaded (error was: ${e})`
    );
    return undefined;
  }
};

const loadAppenderModule = (type, config) => coreAppenders.get(type) ||
  tryLoading(`./${type}`, config) ||
  tryLoading(type, config) ||
  (require.main && tryLoading(path.join(path.dirname(require.main.filename), type), config)) ||
  tryLoading(path.join(process.cwd(), type), config);

const createAppender = (name, config) => {
  const appenderConfig = config.appenders[name];
  const appenderModule = loadAppenderModule(appenderConfig.type, config);
  configuration.throwExceptionIf(
    config,
    configuration.not(appenderModule),
    `appender "${name}" is not valid (type "${appenderConfig.type}" could not be found)`
  );
  if (appenderModule.appender) {
    debug(`DEPRECATION: Appender ${appenderConfig.type} exports an appender function.`);
  }
  if (appenderModule.shutdown) {
    debug(`DEPRECATION: Appender ${appenderConfig.type} exports a shutdown function.`);
  }

  debug(`${name}: clustering.isMaster ? ${clustering.isMaster()}`);
  debug(`${name}: appenderModule is ${require('util').inspect(appenderModule)}`); // eslint-disable-line
  return clustering.onlyOnMaster(() => {
    debug(`calling appenderModule.configure for ${name} / ${appenderConfig.type}`);
    return appenderModule.configure(
      adapters.modifyConfig(appenderConfig),
      layouts,
      appender => appenders.get(appender),
      levels
    );
  }, () => {});
};

const setup = (config) => {
  appenders.clear();

  Object.keys(config.appenders).forEach((name) => {
    debug(`Creating appender ${name}`);
    appenders.set(name, createAppender(name, config));
  });
};

setup({ appenders: { out: { type: 'stdout' } } });

configuration.addListener((config) => {
  configuration.throwExceptionIf(
    config,
    configuration.not(configuration.anObject(config.appenders)),
    'must have a property "appenders" of type object.'
  );
  const appenderNames = Object.keys(config.appenders);
  configuration.throwExceptionIf(
    config,
    configuration.not(appenderNames.length),
    'must define at least one appender.'
  );

  appenderNames.forEach((name) => {
    configuration.throwExceptionIf(
      config,
      configuration.not(config.appenders[name].type),
      `appender "${name}" is not valid (must be an object with property "type")`
    );
  });
});

configuration.addListener(setup);

module.exports = appenders;
