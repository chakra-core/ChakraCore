[![dependencies Status](https://david-dm.org/np-maintain/global-tunnel/status.svg)](https://david-dm.org/np-maintain/global-tunnel)
[![devDependencies Status](https://david-dm.org/np-maintain/global-tunnel/dev-status.svg)](https://david-dm.org/np-maintain/global-tunnel?type=dev)
[![Build Status](https://travis-ci.org/np-maintain/global-tunnel.png)](https://travis-ci.org/np-maintain/global-tunnel) [![Greenkeeper badge](https://badges.greenkeeper.io/np-maintain/global-tunnel.svg)](https://greenkeeper.io/)

# global-tunnel

Configures the [global
`http`](http://nodejs.org/docs/v0.10.24/api/all.html#all_http_globalagent) and
[`https`](http://nodejs.org/docs/v0.10.24/api/all.html#all_https_globalagent)
agents to use an upstream HTTP proxy.

Works transparently to tunnel modules that use node's default [`http.request()`
method](http://nodejs.org/docs/v0.10.24/api/all.html#all_http_request_options_callback)
as well as the popular [`request` module](https://npmjs.org/package/request).

# Installation

You can install this package by just executing the following:

    npm install global-tunnel-ng

# Usage

To make all HTTP and HTTPS connections go through an outbound HTTP proxy:

```js
var globalTunnel = require('global-tunnel-ng');

globalTunnel.initialize({
  host: '10.0.0.10',
  port: 8080,
  proxyAuth: 'userId:password', // optional authentication
  sockets: 50 // optional pool size for each http and https
});
```

This will use the `CONNECT` method for HTTPS requests and absolute-URIs for
HTTP requests, which is how many network proxies are configured.

Optionally, to tear-down the global agent and restore node's default global
agents:

```js
globalTunnel.end();
```

Any active connections will be allowed to run to completion, but new
connections will use the default global agents.

# Advanced Usage

## Options

The complete list of options to `globalTunnel.initialize`:

- **host** - the hostname or IP of the HTTP proxy to use
- **port** - the TCP port to use on that proxy
- **connect** _(optional)_ controls what protocols use the `CONNECT` method.  It
  has three possible values (strings):
  - **neither** - don't use `CONNECT`; just use absolute URIs
  - **https** - _(the default)_ only use `CONNECT` for HTTPS requests
  - **both** - use `CONNECT` for both HTTP and HTTPS requests
- **protocol** - the protocol that the proxy speaks, either `http:` or `https:`.
- **proxyAuth** - _(optional)_ to authenticate `userId:password`
- **sockets** - _(optional)_ maximum number of TCP sockets to use in each pool.
  There are two pools: one for HTTP and one for HTTPS.  Uses node's default (5)
  if falsy.

## Variations

Here's a few interesting variations on the basic config.

### Absolute URI Proxies

Another common proxy configuration is one that expects clients to use an
[absolute URI for the
Request-URI](https://tools.ietf.org/html/rfc2616#section-5.1.2) for all HTTP
and HTTPS requests.  This is common for networks that use a proxy for security
scanning and access control.

What does this mean? It means that instead of ...

```http
GET / HTTP/1.1
Host: example.com
```

... your proxy expects ...

```http
GET https://example.com/ HTTP/1.1
```

You'll need to specify `connect: 'neither'` if this is the case.  If the proxy
speaks HTTP (i.e. the connection from node --> proxy is not encrypted):

```js
globalTunnel.initialize({
  connect: 'neither',
  host: '10.0.0.10',
  port: 3128
});
```

or, if the proxy speaks HTTPS to your app instead:

```js
globalTunnel.initialize({
  connect: 'neither',
  protocol: 'https:',
  host: '10.0.0.10',
  port: 3129
});
```

### Always-CONNECT Proxies

If the proxy expects you to use the `CONNECT` method for both HTTP and HTTPS
requests, you'll need the `connect: 'both'` option.

What does this mean?  It means that instead of ...

```http
GET https://example.com/ HTTP/1.1
```

... your proxy expects ...

```http
CONNECT example.com:443 HTTP/1.1
```

Be sure to set the `protocol:` option based on what protocol the proxy speaks.

```js
globalTunnel.initialize({
  connect: 'both',
  host: '10.0.0.10',
  port: 3130
});
```

### HTTPS configuration

_EXPERIMENTAL_

If tunnelling both protocols, you can use different HTTPS client configurations
for the two phases of the connection.

```js
globalTunnel.initialize({
  connect: 'both',
  protocol: 'https:'
  host: '10.0.0.10',
  port: 3130,
  proxyHttpsOptions: {
    // use this config for app -> proxy
  },
  originHttpsOptions: {
    // use this config for proxy -> origin
  }
});
```

## Auto-Config

If `globalTunnel.initialize` doesnt receive a configuration as its first parameter the `http_proxys` and `http_proxy` environment variables will be used.

If these are missing the npm configurations `https-proxy`, `http-proxy`, `proxy` will be used instead.

If no environment variables or npm configurations are found nothing will be done.

## Retrieving proxy URL, parsed config and proxy status

As the module does some extra job determining the proxy (including parsing the environment variables) and does some normalization (like defaulting the protocol to `http:`) it may be useful to retrieve the proxy URL used by the module.

The property `globalTunnel.proxyUrl` is the URL-formatted (including the optional basic auth if provided) proxy config currently in use. It is `null` if the proxy is not currently enabled.

Similarly, the `globalTunnel.proxyConfig` contains the entire parsed and normalized config.

The property `globalTunnel.isProxying` contains the information about whether the global proxy is on or off.

# Compatibility

Any module that doesn't specify [an explicit `agent:` option to
`http.request`](http://nodejs.org/docs/v0.10.24/api/all.html#all_http_request_options_callback)
will also work with global-tunnel.

The unit tests for this module verify that the popular [`request`
module](https://npmjs.org/package/request) works with global-tunnel active.

For untested modules, it's recommended that you load and initialize
global-tunnel first.  This way, any copies of `http.globalAgent` will point to
the right thing.

# Contributing

If you'd like to contribute to or modify global-tunnel, here's a quick guide
to get you started.

## Development Dependencies

- [node.js](http://nodejs.org) >= 0.10

## Set-Up

Download via GitHub and install npm dependencies:

```sh
git clone git@github.com:np-maintain/global-tunnel.git
cd global-tunnel
npm install
```

## Testing

Testing is with the [mocha](https://github.com/visionmedia/mocha) framework.
Tests are located in the `test/` directory.

To run the tests:

```sh
npm test
```

# Support

As this is a hard fork, you may still contact the given contacts below.

Email [GoInstant Support](mailto:support@goinstant.com) or stop by [#goinstant on freenode](irc://irc.freenode.net#goinstant).

For responsible disclosures, email [GoInstant Security](mailto:security@goinstant.com).

To [file a bug](https://github.com/np-maintain/global-tunnel/issues) or
[propose a patch](https://github.com/np-maintain/global-tunnel/pulls),
please use github directly.

# Legal

&copy; 2014 GoInstant Inc., a salesforce.com company

Licensed under the BSD 3-clause license.
