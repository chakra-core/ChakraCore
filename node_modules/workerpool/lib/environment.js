// determines the JavaScript platform: browser or node
module.exports.platform = typeof Window !== 'undefined' || typeof WorkerGlobalScope !== 'undefined' ? 'browser' : 'node';

// determines whether the code is running in main thread or not
module.exports.isMainThread = module.exports.platform === 'browser' ? typeof Window !== 'undefined' : !process.connected;

// determines the number of cpus available
module.exports.cpus = module.exports.platform === 'browser'
    ? self.navigator.hardwareConcurrency
    : require('os').cpus().length;
