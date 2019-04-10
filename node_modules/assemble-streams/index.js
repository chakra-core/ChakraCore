/*!
 * assemble-streams <https://github.com/assemble/assemble-streams>
 *
 * Copyright (c) 2015-2017, Jon Schlinkert.
 * Released under the MIT License.
 */

'use strict';

var utils = require('./utils');

module.exports = function(options) {
  return function plugin(app) {
    if (utils.isValid(app, 'assemble-streams')) {
      app.define('toStream', appStream(app));
      app.on('view', function(view) {
        viewPlugin.call(view, view);
      });
      return collectionPlugin;
    }

    function collectionPlugin(collection) {
      if (utils.isValid(collection, 'assemble-streams', ['collection'])) {
        collection.define('toStream', collectionStream(app, this));
      }
      return viewPlugin;
    }

    function viewPlugin(view) {
      if (utils.isValid(this, 'assemble-streams', ['item', 'file'])) {
        utils.define(this, 'toStream', viewStream(app));
      }
    }
    return plugin;
  };
};

/**
 * Push a view collection into a vinyl stream.
 *
 * ```js
 * app.toStream('posts', function(file) {
 *   return file.path !== 'index.hbs';
 * })
 * ```
 * @name app.toStream
 * @param {String} `collection` Name of the collection to push into the stream.
 * @param {Function} Optionally pass a filter function to use for filtering views.
 * @return {Stream}
 * @api public
 */

function appStream(app) {
  if (!hasHandler(app, 'onStream')) {
    app.handler('onStream');
  }

  return function(name, filterFn) {
    var stream = utils.through.obj();
    stream.setMaxListeners(0);

    if (typeof name === 'undefined') {
      process.nextTick(stream.end.bind(stream));
      return utils.src(stream);
    }

    var write = writeStream(stream);
    var collection = this[name];
    var views = collection && collection.views;

    if (!views && typeof name !== 'undefined') {
      filterFn = name;
      setImmediate(function() {
        Object.keys(this.views).forEach(function(key) {
          views = this.views[key];
          write(views, filterFn);
        }, this);
        stream.end();
      }.bind(this));

      return outStream(stream, this);
    }

    setImmediate(function() {
      write(views, filterFn);
      stream.end();
    });

    return outStream(stream, this);
  };
}

/**
 * Push a view collection into a vinyl stream.
 *
 * ```js
 * app.posts.toStream(function(file) {
 *   return file.path !== 'index.hbs';
 * })
 * ```

 * @name collection.toStream
 * @param {Function} Optionally pass a filter function to use for filtering views.
 * @return {Stream}
 * @api public
 */

function collectionStream(collection) {
  if (!hasHandler(collection, 'onStream')) {
    collection.handler('onStream');
  }

  return function(filterFn) {
    var stream = utils.through.obj();
    stream.setMaxListeners(0);

    var views = this.views;
    var write = writeStream(stream);

    setImmediate(function() {
      write(views, filterFn);
      stream.end();
    });

    return outStream(stream, collection);
  };
}

/**
 * Push the current view into a vinyl stream.
 *
 * ```js
 * app.pages.getView('a.html').toStream()
 *   .on('data', function(file) {
 *     console.log(file);
 *     //=> <Page "a.html" <Buffer 2e 2e 2e>>
 *   });
 * ```
 *
 * @name view.toStream
 * @return {Stream}
 * @api public
 */

function viewStream(view) {
  return function() {
    var stream = utils.through.obj();
    stream.setMaxListeners(0);
    setImmediate(function(item) {
      stream.write(item);
      stream.end();
    }, this);
    return outStream(stream, view);
  };
}

function writeStream(stream) {
  return function(views, filterFn) {
    for (var key in views) {
      if (filter(key, views[key], filterFn)) {
        stream.write(views[key]);
      }
    }
  };
}

function outStream(stream, instance) {
  return utils.src(stream.pipe(utils.handle.once(instance, 'onStream')));
}

function hasHandler(app, name) {
  return typeof app.handler === 'function' && typeof app[name] === 'function';
}

function filter(key, view, val) {
  switch (utils.typeOf(val)) {
    case 'array':
      var len = val.length;
      var idx = -1;
      while (++idx < len) {
        var name = val[idx];
        if (utils.match(name, view)) {
          return true;
        }
      }
      return false;
    case 'function':
      return val(key, view);
    case 'string':
      return utils.match(val, view);
    default: {
      return true;
    }
  }
}
