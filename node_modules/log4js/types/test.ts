import * as log4js from './log4js';

log4js.configure('./filename');
const logger1 = log4js.getLogger();
logger1.level = 'debug';
logger1.debug("Some debug messages");
logger1.fatal({
  whatever: 'foo'
})

const logger3 = log4js.getLogger('cheese');
logger3.trace('Entering cheese testing');
logger3.debug('Got cheese.');
logger3.info('Cheese is Gouda.');
logger3.warn('Cheese is quite smelly.');
logger3.error('Cheese is too ripe!');
logger3.fatal('Cheese was breeding ground for listeria.');

log4js.configure({
	appenders: { cheese: { type: 'console', filename: 'cheese.log' } },
	categories: { default: { appenders: ['cheese'], level: 'error' } }
});

log4js.configure({
	appenders: {
		out: { type: 'file', filename: 'pm2logs.log' }
	},
	categories: {
		default: { appenders: ['out'], level: 'info' }
	},
	pm2: true,
	pm2InstanceVar: 'INSTANCE_ID'
});

log4js.addLayout('json', config => function (logEvent) {
	return JSON.stringify(logEvent) + config.separator;
});

log4js.configure({
	appenders: {
		out: { type: 'stdout', layout: { type: 'json', separator: ',' } }
	},
	categories: {
		default: { appenders: ['out'], level: 'info' }
	}
});

log4js.configure({
	appenders: {
		file: { type: 'dateFile', filename: 'thing.log', pattern: '.mm' }
	},
	categories: {
		default: { appenders: ['file'], level: 'debug' }
	}
});

const logger4 = log4js.getLogger('thing');
logger4.log('logging a thing');

const logger5 = log4js.getLogger('json-test');
logger5.info('this is just a test');
logger5.error('of a custom appender');
logger5.warn('that outputs json');
log4js.shutdown(() => { });

log4js.configure({
	appenders: {
		cheeseLogs: { type: 'file', filename: 'cheese.log' },
		console: { type: 'console' }
	},
	categories: {
		cheese: { appenders: ['cheeseLogs'], level: 'error' },
		another: { appenders: ['console'], level: 'trace' },
		default: { appenders: ['console', 'cheeseLogs'], level: 'trace' }
	}
});

const logger6 = log4js.getLogger('cheese');
// only errors and above get logged.
const otherLogger = log4js.getLogger();

// this will get coloured output on console, and appear in cheese.log
otherLogger.error('AAArgh! Something went wrong', { some: 'otherObject', useful_for: 'debug purposes' });
otherLogger.log('This should appear as info output');

// these will not appear (logging level beneath error)
logger6.trace('Entering cheese testing');
logger6.debug('Got cheese.');
logger6.info('Cheese is Gouda.');
logger6.log('Something funny about cheese.');
logger6.warn('Cheese is quite smelly.');
// these end up only in cheese.log
logger6.error('Cheese %s is too ripe!', 'gouda');
logger6.fatal('Cheese was breeding ground for listeria.');

// these don't end up in cheese.log, but will appear on the console
const anotherLogger = log4js.getLogger('another');
anotherLogger.debug('Just checking');

// will also go to console and cheese.log, since that's configured for all categories
const pantsLog = log4js.getLogger('pants');
pantsLog.debug('Something for pants');


import { configure, getLogger } from './log4js';
configure('./filename');
const logger2 = getLogger();
logger2.level = 'debug';
logger2.debug("Some debug messages");

configure({
	appenders: { cheese: { type: 'file', filename: 'cheese.log' } },
	categories: { default: { appenders: ['cheese'], level: 'error' } }
});

log4js.configure('./filename').getLogger();
const logger7 = log4js.getLogger();
logger7.level = 'debug';
logger7.debug("Some debug messages");

const levels: log4js.Levels = log4js.levels;
const level: log4js.Level = levels.getLevel('info');

log4js.connectLogger(logger1, {
  format: ':x, :y',
  level: 'info'
});

log4js.connectLogger(logger2, {
  format: (req, _res, format) => format(`:remote-addr - ${req.id} - ":method :url HTTP/:http-version" :status :content-length ":referrer" ":user-agent"`)
});
