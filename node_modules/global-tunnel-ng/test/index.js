'use strict';
var assert = require('chai').assert;
var sinon = require('sinon');
var assign = require('lodash/assign');
var pick = require('lodash/pick');

// Deliberate: node and 3rd party modules before global-tunnel
var EventEmitter = require('events').EventEmitter;
var net = require('net');
var tls = require('tls');
var http = require('http');
var globalHttpAgent = http.globalAgent;
var https = require('https');
var globalHttpsAgent = https.globalAgent;
var request = require('request');

// Deliberate: load after all 3rd party modules
var globalTunnel = require('../index');

function newFakeAgent() {
  var fakeAgent = {
    addRequest: sinon.stub()
  };
  return fakeAgent;
}

// This function replaces 'host' by 'hostname' in the options for http.request()
// background: http.request() allows to use either 'host' or 'hostname' to be used,
// both needs to be tested
function replaceHostByHostname(useHostname, options) {
  if (useHostname) {
    options.hostname = options.host;
    delete options.host;
  }
  return options;
}

var origEnv;
function saveEnv() {
  origEnv = process.env.http_proxy;
  delete process.env.http_proxy;
}
function restoreEnv() {
  if (origEnv !== undefined) {
    process.env.http_proxy = origEnv; // eslint-disable-line camelcase
  }
}

describe('global-proxy', function() {
  // Save and restore http_proxy environment variable (yes, it's lower-case by
  // convention).
  before(saveEnv);
  after(restoreEnv);

  // Sinon setup & teardown
  var sandbox;
  var origHttpCreateConnection;

  before(function() {
    sandbox = sinon.createSandbox();

    sandbox.stub(globalHttpAgent, 'addRequest');
    sandbox.stub(globalHttpsAgent, 'addRequest');

    assert.equal(http.Agent.prototype.addRequest, https.Agent.prototype.addRequest);
    sandbox.spy(http.Agent.prototype, 'addRequest');

    sandbox.stub(net, 'createConnection').callsFake(function() {
      return new EventEmitter();
    });
    sandbox.stub(tls, 'connect').callsFake(function() {
      return new EventEmitter();
    });

    // This is needed as at some point Node HTTP aggent implementation started
    // plucking the createConnection method from the `net` module
    // instead of doing `net.createConnection`
    origHttpCreateConnection = http.Agent.prototype.createConnection;
    http.Agent.prototype.createConnection = net.createConnection;
  });

  afterEach(function() {
    sandbox.resetHistory();
  });

  after(function() {
    sandbox.restore();
    http.Agent.prototype.createConnection = origHttpCreateConnection;
  });

  describe('invalid configs', function() {
    it('requires a host', function() {
      var conf = { host: null, port: 1234 };
      assert.throws(function() {
        globalTunnel.initialize(conf);
      }, 'upstream proxy host is required');
      globalTunnel.end();
    });

    it('requires a port', function() {
      var conf = { host: '10.2.3.4', port: 0 };
      assert.throws(function() {
        globalTunnel.initialize(conf);
      }, 'upstream proxy port is required');
      globalTunnel.end();
    });

    it('clamps tunnel types', function() {
      var conf = { host: '10.2.3.4', port: 1234, connect: 'INVALID' };
      assert.throws(function() {
        globalTunnel.initialize(conf);
      }, 'valid connect options are "neither", "https", or "both"');
      globalTunnel.end();
    });
  });

  describe('exposed config', function() {
    afterEach(function() {
      globalTunnel.end();
    });

    it('has the same params as the passed config', function() {
      var conf = {
        host: 'proxy.com',
        port: 1234,
        proxyAuth: 'user:pwd',
        protocol: 'https:'
      };
      globalTunnel.initialize(conf);
      assert.deepEqual(
        conf,
        pick(globalTunnel.proxyConfig, ['host', 'port', 'proxyAuth', 'protocol'])
      );
    });

    it('has the expected defaults', function() {
      var conf = { host: 'proxy.com', port: 1234, proxyAuth: 'user:pwd' };
      globalTunnel.initialize(conf);
      assert.equal(globalTunnel.proxyConfig.protocol, 'http:');
    });
  });

  describe('stringified config', function() {
    afterEach(function() {
      globalTunnel.end();
    });

    it('has the same params as the passed config', function() {
      var conf = {
        host: 'proxy.com',
        port: 1234,
        proxyAuth: 'user:pwd',
        protocol: 'https'
      };
      globalTunnel.initialize(conf);
      assert.equal(globalTunnel.proxyUrl, 'https://user:pwd@proxy.com:1234');
    });

    it('encodes url', function() {
      var conf = {
        host: 'proxy.com',
        port: 1234,
        proxyAuth: 'user:4P@S$W0_r-D',
        protocol: 'https'
      };
      globalTunnel.initialize(conf);
      assert.equal(globalTunnel.proxyUrl, 'https://user:4P%40S%24W0_r-D@proxy.com:1234');
    });
  });

  function proxyEnabledTests(testParams) {
    function connected(innerProto) {
      var innerSecure = innerProto === 'https:';

      var called;
      if (testParams.isHttpsProxy) {
        called = tls.connect;
        sinon.assert.notCalled(net.createConnection);
      } else {
        called = net.createConnection;
        sinon.assert.notCalled(tls.connect);
      }

      sinon.assert.calledOnce(called);
      if (typeof called.getCall(0).args[0] === 'object') {
        sinon.assert.calledWith(called, sinon.match.has('port', testParams.port));
        sinon.assert.calledWith(called, sinon.match.has('host', '10.2.3.4'));
      } else {
        sinon.assert.calledWith(called, testParams.port, '10.2.3.4');
      }

      var isCONNECT =
        testParams.connect === 'both' || (innerSecure && testParams.connect === 'https');
      if (isCONNECT) {
        var expectConnect = 'example.dev:' + (innerSecure ? 443 : 80);
        var whichAgent = innerSecure ? https.globalAgent : http.globalAgent;

        sinon.assert.calledOnce(whichAgent.request);
        sinon.assert.calledWith(whichAgent.request, sinon.match.has('method', 'CONNECT'));
        sinon.assert.calledWith(
          whichAgent.request,
          sinon.match.has('path', expectConnect)
        );
      } else {
        sinon.assert.calledOnce(http.Agent.prototype.addRequest);
        var req = http.Agent.prototype.addRequest.getCall(0).args[0];

        var method = req.method;
        assert.equal(method, 'GET');

        var path = req.path;
        if (innerSecure) {
          assert.match(path, new RegExp('^https://example\\.dev:443/'));
        } else {
          assert.match(path, new RegExp('^http://example\\.dev:80/'));
        }
      }
    }

    var localSandbox;
    beforeEach(function() {
      localSandbox = sinon.createSandbox();
      if (testParams.connect === 'both') {
        localSandbox.spy(http.globalAgent, 'request');
      }
      if (testParams.connect !== 'neither') {
        localSandbox.spy(https.globalAgent, 'request');
      }
    });
    afterEach(function() {
      localSandbox.restore();
    });

    it('(got proxying set up)', function() {
      assert.isTrue(globalTunnel.isProxying);
    });

    describe('with the request library', function() {
      it('will proxy http requests', function(done) {
        assert.isTrue(globalTunnel.isProxying);
        var dummyCb = sinon.stub();
        request.get('http://example.dev/', dummyCb);
        setImmediate(function() {
          connected('http:');
          sinon.assert.notCalled(globalHttpAgent.addRequest);
          sinon.assert.notCalled(globalHttpsAgent.addRequest);
          done();
        });
      });

      it('will proxy https requests', function(done) {
        assert.isTrue(globalTunnel.isProxying);
        var dummyCb = sinon.stub();
        request.get('https://example.dev/', dummyCb);
        setImmediate(function() {
          connected('https:');
          sinon.assert.notCalled(globalHttpAgent.addRequest);
          sinon.assert.notCalled(globalHttpsAgent.addRequest);
          done();
        });
      });
    });

    describe('using raw request interface', function() {
      function rawRequest(useHostname) {
        var req = http.request(
          replaceHostByHostname(useHostname, {
            method: 'GET',
            path: '/raw-http',
            host: 'example.dev'
          }),
          function() {}
        );
        req.end();

        connected('http:');
        sinon.assert.notCalled(globalHttpAgent.addRequest);
        sinon.assert.notCalled(globalHttpsAgent.addRequest);
      }
      it('will proxy http requests (`host`)', function() {
        rawRequest(false);
      });
      it('will proxy http requests (`hostname`)', function() {
        rawRequest(true);
      });

      it('will proxy https requests', function() {
        var req = https.request(
          replaceHostByHostname(false, {
            method: 'GET',
            path: '/raw-https',
            host: 'example.dev'
          }),
          function() {}
        );
        req.end();

        connected('https:');
        sinon.assert.notCalled(globalHttpAgent.addRequest);
        sinon.assert.notCalled(globalHttpsAgent.addRequest);
      });

      it('request respects explicit agent param', function() {
        var agent = newFakeAgent();
        var req = http.request(
          replaceHostByHostname(false, {
            method: 'GET',
            path: '/raw-http-w-agent',
            host: 'example.dev',
            agent: agent
          }),
          function() {}
        );
        req.end();

        sinon.assert.notCalled(globalHttpAgent.addRequest);
        sinon.assert.notCalled(globalHttpsAgent.addRequest);
        sinon.assert.notCalled(net.createConnection);
        sinon.assert.notCalled(tls.connect);
        sinon.assert.calledOnce(agent.addRequest);
      });

      describe('request with `null` agent and defined `createConnection`', function() {
        before(function() {
          sinon.stub(http.ClientRequest.prototype, 'onSocket');
        });
        after(function() {
          http.ClientRequest.prototype.onSocket.restore();
        });

        function noAgent(useHostname) {
          var createConnection = sinon.stub();
          var req = http.request(
            replaceHostByHostname(useHostname, {
              method: 'GET',
              path: '/no-agent',
              host: 'example.dev',
              agent: null,
              createConnection: createConnection
            }),
            function() {} // eslint-disable-line max-nested-callbacks
          );
          req.end();

          sinon.assert.notCalled(globalHttpAgent.addRequest);
          sinon.assert.notCalled(globalHttpsAgent.addRequest);
          sinon.assert.calledOnce(createConnection);
        }
        it('uses no agent (`host`)', function() {
          noAgent(false);
        });
        it('uses no agent (`hostname`)', function() {
          noAgent(true);
        });
      });
    });
  }

  function enabledBlock(conf, testParams) {
    before(function() {
      globalTunnel.initialize(conf);
    });
    after(function() {
      globalTunnel.end();
    });

    testParams = assign(
      {
        port: conf && conf.port,
        isHttpsProxy: conf && conf.protocol === 'https:',
        connect: (conf && conf.connect) || 'https'
      },
      testParams
    );

    proxyEnabledTests(testParams);
  }

  describe('with http proxy in intercept mode', function() {
    enabledBlock({
      connect: 'neither',
      protocol: 'http:',
      host: '10.2.3.4',
      port: 3333
    });
  });

  describe('with https proxy in intercept mode', function() {
    enabledBlock({
      connect: 'neither',
      protocol: 'https:',
      host: '10.2.3.4',
      port: 3334
    });
  });

  describe('with http proxy in CONNECT mode', function() {
    enabledBlock({
      connect: 'both',
      protocol: 'http:',
      host: '10.2.3.4',
      port: 3335
    });
  });

  describe('with https proxy in CONNECT mode', function() {
    enabledBlock({
      connect: 'both',
      protocol: 'https:',
      host: '10.2.3.4',
      port: 3336
    });
  });

  describe('with http proxy in mixed mode', function() {
    enabledBlock({
      protocol: 'http:',
      host: '10.2.3.4',
      port: 3337
    });
  });

  describe('with https proxy in mixed mode', function() {
    enabledBlock({
      protocol: 'https:',
      host: '10.2.3.4',
      port: 3338
    });
  });

  describe('using env var', function() {
    after(function() {
      delete process.env.http_proxy;
      assert.isUndefined(process.env.http_proxy);
    });

    describe('for http', function() {
      before(function() {
        process.env.http_proxy = 'http://10.2.3.4:1234'; // eslint-disable-line camelcase
      });
      enabledBlock(null, { isHttpsProxy: false, connect: 'https', port: 1234 });
    });

    describe('for https', function() {
      before(function() {
        process.env.http_proxy = 'https://10.2.3.4:1235'; // eslint-disable-line camelcase
      });
      enabledBlock(null, { isHttpsProxy: true, connect: 'https', port: 1235 });
    });
  });

  describe('using npm config', function() {
    var expectedProxy = { isHttpsProxy: false, connect: 'https', port: 1234 };
    var npmConfig = { get: function() {} };
    var npmConfigStub = sinon.stub(npmConfig, 'get');

    function configNpm(key, value) {
      return function() {
        global.__GLOBAL_TUNNEL_DEPENDENCY_NPMCONF__ = function() {
          return npmConfig;
        };

        npmConfigStub.withArgs(key).returns(value || 'http://10.2.3.4:1234');
      };
    }

    after(function() {
      global.__GLOBAL_TUNNEL_DEPENDENCY_NPMCONF__ = undefined;
    });

    describe('https-proxy', function() {
      before(configNpm('https-proxy'));
      enabledBlock(null, expectedProxy);
    });

    describe('http-proxy', function() {
      before(configNpm('http-proxy'));
      enabledBlock(null, expectedProxy);
    });

    describe('proxy', function() {
      before(configNpm('proxy'));
      enabledBlock(null, expectedProxy);
    });
    describe('order', function() {
      before(function() {
        configNpm('proxy')();
        configNpm('https-proxy', 'http://10.2.3.4:12345')();
        configNpm('http-proxy')();
      });

      enabledBlock(null, { isHttpsProxy: false, connect: 'https', port: 12345 });
    });

    describe('also using env var', function() {
      before(function() {
        configNpm('proxy')();
        process.env.http_proxy = 'http://10.2.3.4:1234'; // eslint-disable-line camelcase
      });

      after(function() {
        delete process.env.http_proxy;
      });

      enabledBlock(null, expectedProxy);
    });
  });

  // Deliberately after the block above
  describe('with proxy disabled', function() {
    it('claims to be disabled', function() {
      assert.isFalse(globalTunnel.isProxying);
    });

    it('will NOT proxy http requests', function(done) {
      var dummyCb = sinon.stub();
      request.get('http://example.dev/', dummyCb);
      setImmediate(function() {
        sinon.assert.calledOnce(globalHttpAgent.addRequest);
        sinon.assert.notCalled(globalHttpsAgent.addRequest);
        done();
      });
    });

    it('will NOT proxy https requests', function(done) {
      var dummyCb = sinon.stub();
      request.get('https://example.dev/', dummyCb);
      setImmediate(function() {
        sinon.assert.notCalled(globalHttpAgent.addRequest);
        sinon.assert.calledOnce(globalHttpsAgent.addRequest);
        done();
      });
    });
  });
});
