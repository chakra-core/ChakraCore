/*

testem_client.js
================

The client-side script that reports results back to the Testem server via Socket.IO.
It also restarts the tests by refreshing the page when instructed by the server to do so.

*/
/* globals document, window */
/* globals module */
/* globals jasmineAdapter, jasmine2Adapter, mochaAdapter */
/* globals qunitAdapter, busterAdapter, decycle, TestemConfig */
/* exported Testem */
'use strict';

function getTestemIframeSrc() {
  // Compute a URL to testem/connection.html based on the URL from which this
  // script was loaded (not the document's base URL, in case the document was
  // loaded via a file: URL)
  var scripts = document.getElementsByTagName('script');
  var thisScript = scripts[scripts.length - 1];
  var a = document.createElement('a');
  a.href = thisScript.src;
  a.pathname = '/testem/connection.html';
  return a.href;
}

function appendTestemIframeOnLoad(callback) {
  var iframeAppended = false;
  // Needs to call this synchronously during script load so we know which
  // <script> tag is loading us and we can grab the right src attribute.
  var iframeHref = getTestemIframeSrc();

  var appendIframe = function() {
    if (iframeAppended) {
      return;
    }
    iframeAppended = true;
    var iframe = document.createElement('iframe');
    iframe.style.border = 'none';
    iframe.style.position = 'fixed';
    iframe.style.right = '5px';
    iframe.style.bottom = '5px';
    iframe.frameBorder = '0';
    iframe.allowTransparency = 'true';
    iframe.src = iframeHref;
    document.body.appendChild(iframe);
    callback(iframe);
  };

  var domReady = function() {
    if (!document.body) {
      return setTimeout(domReady, 1);
    }
    appendIframe();
  };

  var DOMContentLoaded = function() {
    if (document.addEventListener) {
      document.removeEventListener('DOMContentLoaded', DOMContentLoaded, false);
    } else {
      document.detachEvent('onreadystatechange', DOMContentLoaded);
    }
    domReady();
  };

  if (document.addEventListener) {
    document.addEventListener('DOMContentLoaded', DOMContentLoaded, false);
    window.addEventListener('load', DOMContentLoaded, false);
  } else if (document.attachEvent) {
    document.attachEvent('onreadystatechange', DOMContentLoaded);
    window.attachEvent('onload', DOMContentLoaded);
  }

  if (document.readyState !== 'loading') {
    domReady();
  }
}

var testFrameworkDidInit = false;
function hookIntoTestFramework(socket) {
  if (testFrameworkDidInit) {
    return;
  }

  var found = true;
  if (typeof getJasmineRequireObj === 'function') {
    jasmine2Adapter(socket);
  } else if (typeof jasmine === 'object') {
    jasmineAdapter(socket);
  } else if (typeof Mocha === 'function') {
    mochaAdapter(socket);
  } else if (typeof QUnit === 'object') {
    qunitAdapter(socket);
  } else if (typeof buster !== 'undefined') {
    busterAdapter(socket);
  } else {
    found = false;
  }

  testFrameworkDidInit = found;
  return found;
}

var addListener;
if (typeof window !== 'undefined') {
  addListener = window.addEventListener ?
    function(obj, evt, cb) { obj.addEventListener(evt, cb, false); } :
    function(obj, evt, cb) { obj.attachEvent('on' + evt, cb); };
}

// Used internally in order to remember state involving a message that needs to
// be fired after a delay. It matters which socket sends the message, because
// the socket is configurable by custom adapters.
function Message(socket, emitArgs) {
  this.socket = socket;
  this.emitArgs = emitArgs;
}

// eslint-disable-next-line no-use-before-define
if (typeof TestemConfig === 'undefined') {
  var TestemConfig = {};
}

var Testem = {
  emitMessageQueue: [],
  afterTestsQueue: [],
  console: {},

  // The maximum depth beyond which decycle will truncate an emitted event
  // object. When undefined, decycle uses its default.
  decycleDepth: TestemConfig.decycle_depth,

  useCustomAdapter: function(adapter) {
    adapter(new TestemSocket());
  },
  getId: function() {
    // If the test page defined a custom method for discovering our id, use
    // that
    if (window.getTestemId) {
      return window.getTestemId();
    }
    var match = window.location.pathname.match(/^\/(-?[0-9]+)/);
    return match ? match[1] : null;
  },
  emitMessage: function() {
    if (this._noConnectionRequired) {
      return;
    }
    var args = new Array(arguments.length);
    for (var i = 0; i < args.length; ++i) {
      args[i] = arguments[i];
    }

    var message = new Message(this, args);

    if (this._isIframeReady) {
      this.emitMessageToIframe(message);
    } else {
      // enqueue until iframe is ready
      this.enqueueMessage(message);
    }
  },
  emit: function(evt) {
    var argsWithoutFirst = new Array(arguments.length - 1);
    for (var i = 1; i < arguments.length; ++i) {
      argsWithoutFirst[i - 1] = arguments[i];
    }

    if (this.evtHandlers && this.evtHandlers[evt]) {
      var handlers = this.evtHandlers[evt];
      for (var j = 0; j < handlers.length; j++) {
        var handler = handlers[j];
        handler.apply(this, argsWithoutFirst);
      }
    }

    this.emitMessage.apply(this, arguments);
  },
  on: function(evt, callback) {
    if (!this.evtHandlers) {
      this.evtHandlers = {};
    }
    if (!this.evtHandlers[evt]) {
      this.evtHandlers[evt] = [];
    }
    this.evtHandlers[evt].push(callback);
  },
  handleConsoleMessage: null,
  noConnectionRequired: function() {
    this._noConnectionRequired = true;
    this.emitMessageQueue = [];
  },
  emitMessageToIframe: function(message) {
    message.socket.sendMessageToIframe('emit-message', message.emitArgs);
  },
  sendMessageToIframe: function(type, data) {
    var message = { type: type };
    var decycleDepth = -1;
    if (data) {
      message.data = data;

      if (data[0] === 'browser-console') {
        // User content in data
        decycleDepth = this.decycleDepth + 1;
      } else if (data[0] === 'test-result') {
        // User content in data.test.items
        decycleDepth = this.decycleDepth + 3;
      } else if (data[0] === 'all-test-results') {
        // User content in data.tests.test.items
        decycleDepth = this.decycleDepth + 4;
      } else {
        // Events don't contain user content / cycles
        decycleDepth = -1;
      }
    }

    message = this.serializeMessage(message, decycleDepth);
    this.iframe.contentWindow.postMessage(message, '*');
  },
  enqueueMessage: function(message) {
    if (this._noConnectionRequired) {
      return;
    }
    this.emitMessageQueue.push(message);
  },
  iframeReady: function() {
    this.drainMessageQueue();
    this._isIframeReady = true;
  },
  drainMessageQueue: function() {
    while (this.emitMessageQueue.length) {
      var item = this.emitMessageQueue.shift();
      this.emitMessageToIframe(item);
    }
  },
  listenTo: function(iframe) {
    this.iframe = iframe;
    var self = this;

    addListener(window, 'message', function messageListener(event) {
      if (event.source !== self.iframe.contentWindow) {
        // ignore messages not from the iframe
        return;
      }

      var message = self.deserializeMessage(event.data);
      var type = message.type;

      switch (type) {
        case 'reload':
          self.reload();
          break;
        case 'get-id':
          self.sendId();
          break;
        case 'no-connection-required':
          self.noConnectionRequired();
          break;
        case 'iframe-ready':
          self.iframeReady();
          break;
        case 'tap-all-test-results':
          self.emit('tap-all-test-results');
          break;
        case 'stop-run':
          self.emit('after-tests-complete');
          break;
        default:
          if (type.indexOf('testem:') === 0) {
            self.emit(type, message.data);
          }
          break;
      }
    });
  },
  sendId: function() {
    this.sendMessageToIframe('get-id', this.getId());
  },
  reload: function() {
    window.location.reload();
  },
  deserializeMessage: function(message) {
    return JSON.parse(message);
  },
  serializeMessage: function(message, depth) {
    // decycle to remove possible cyclic references
    if (depth !== -1) {
      message = decycle(message, depth);
    }
    // stringify for clients that only can handle string postMessages (IE <= 10)
    return JSON.stringify(message);
  },
  removeEventCallbacks: function(evt, callback) {
    var handlers = this.evtHandlers[evt];
    var removeIdx = [];
    if (typeof handlers === "undefined") {
      return;
    }
    for (var i = 0; i < handlers.length; i++) {
      if (handlers[i] === callback) {
        removeIdx.push(i);
      }
    }
    for (var j = 0; j < removeIdx.length; j++) {
      handlers.splice(j, 1);
    }
  },
  runAfterTests: function() {
    if (Testem.afterTestsQueue.length) {
      var afterTestsCallback = Testem.afterTestsQueue.shift();

      if (typeof afterTestsCallback !== 'function') {
        throw Error('Callback not a function');
      } else {
        afterTestsCallback.call(this, null, null, Testem.runAfterTests);
      }

    } else {
      emit('after-tests-complete');
    }
  },
  afterTests: function(cb) {
    Testem.afterTestsQueue.push(cb);
  }
};

// Represents a configurable socket on top of window.Testem, which is provided
// to each custom adapter.
function TestemSocket() {}
TestemSocket.prototype = Testem;

// Exporting this as a module so that it can be unit tested in Node.
if (typeof module !== 'undefined') {
  module.exports = Testem;
}

function init() {
  appendTestemIframeOnLoad(function(iframe) {
    Testem.listenTo(iframe);
  });
  interceptWindowOnError();
  takeOverConsole();
  setupTestStats();
  Testem.hookIntoTestFramework = function() {
    if (!hookIntoTestFramework(Testem)) {
      throw new Error('Testem was unable to detect a test framework, please load it before invoking Testem.hookIntoTestFramework');
    }
  };
  hookIntoTestFramework(Testem);
  Testem.on('all-test-results', Testem.runAfterTests);
  Testem.on('tap-all-test-results', Testem.runAfterTests);
}

function setupTestStats() {
  var originalTitle = document.title;
  var total = 0;
  var passed = 0;
  Testem.on('test-result', function(test) {
    total++;
    if (test.failed === 0) {
      passed++;
    }
    updateTitle();
  });

  function updateTitle() {
    if (!total) {
      return;
    }
    document.title = originalTitle + ' (' + passed + '/' + total + ')';
  }
}

function takeOverConsole() {
  function intercept(method) {
    var original = console[method];
    Testem.console[method] = original;
    console[method] = function() {
      var doDefault, message;
      var args = new Array(arguments.length);
      for (var i = 0; i < args.length; ++i) {
        args[i] = arguments[i];
      }

      if (Testem.handleConsoleMessage) {
        message = decycle(args, Testem.decycleDepth).join(' ');
        doDefault = Testem.handleConsoleMessage(message);
      }

      if (doDefault !== false) {
        args.unshift(method);
        args.unshift('browser-console');
        emit.apply(Testem, args);

        if (typeof original === 'object') {
          // Do this for IE
          Function.prototype.apply.call(original, console, arguments);
        } else {
          // Do this for normal browsers
          original.apply(console, arguments);
        }
      }
    };
  }
  var methods = ['log', 'warn', 'error', 'info'];
  for (var i = 0; i < methods.length; i++) {
    if (window.console && console[methods[i]]) {
      intercept(methods[i]);
    }
  }
}

function interceptWindowOnError() {
  var orginalOnError = window.onerror;
  window.onerror = function(msg, url, line) {
    if (typeof msg === 'string' && typeof url === 'string' && typeof line === 'number') {
      emit('top-level-error', msg, url, line);
    }
    if (orginalOnError) {
      orginalOnError.apply(window, arguments);
    }
  };
}

function emit() {
  Testem.emit.apply(Testem, arguments);
}

if (typeof window !== 'undefined') {
  window.Testem = Testem;

  // Stub window.alert and window.confirm to throw error so alert and confirm does not get used in tests
  // this will prevent browser disconnect failures
  window.alert = function() {
    throw new Error('[Testem] Calling window.alert() in tests is disabled, because it causes testem to fail with browser disconnect error.');
  }

  window.confirm = function() {
    throw new Error('[Testem] Calling window.confirm() in tests is disabled, because it causes testem to fail with browser disconnect error.');
  }
  init();
}
