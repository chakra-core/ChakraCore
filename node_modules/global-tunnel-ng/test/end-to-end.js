// Use this tests for troubleshooting, you'll need a proxy runnig at an endpoint
// The idea is to make sure the tests pass if the proxy is turned on, that means the requests are resolving
// And also the tests should fail if the proxy is turned off, that means the requests are actually being proxied

const globalTunnel = require('../index');
const assert = require('chai').assert;
const request = require('request');
const http = require('http');
const https = require('https');

// You need to have a proxy running at the Proxy URL.
const proxyUrl = 'http://localhost:8080';
const resourceUrl = 'www.google.com';

describe.skip('end-to-end tests', () => {
  beforeEach(() => {
    globalTunnel.initialize(proxyUrl);
  });

  const httpResourceUrl = (secure = false) => `http${secure ? 's' : ''}://${resourceUrl}`;

  const testHttp = (httpMethod = 'request') => (secure = false) => () =>
    new Promise((resolve, reject) => {
      const request = (secure ? https : http)[httpMethod](
        httpResourceUrl(secure),
        response => {
          assert.isAtLeast(response.statusCode, 200);
          assert.isBelow(response.statusCode, 300);

          let buffer = Buffer.alloc(0);

          response.on('data', chunk => {
            buffer = Buffer.concat([buffer, chunk]);
          });
          response.on('end', () => {
            assert.isNotEmpty(buffer.toString());
            resolve();
          });
        }
      );

      request.on('error', reject);

      if (httpMethod === 'request') {
        request.end();
      }
    });

  const testHttpRequest = testHttp();
  const testHttpGet = testHttp('get');

  it('proxies http.get', testHttpGet());
  it('proxies https.get', testHttpGet(true));
  it('proxies http.request', testHttpRequest());
  it('proxies https.request', testHttpRequest(true));

  it('proxies request', () =>
    new Promise((resolve, reject) => {
      request.get({ url: httpResourceUrl(true) }, err => {
        if (err) {
          reject(err);
        } else {
          resolve();
        }
      });
    }));

  afterEach(() => {
    globalTunnel.end();
  });
});
