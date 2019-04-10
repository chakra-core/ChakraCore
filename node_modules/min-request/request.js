var http = require('http')
var url = require('url')
var util = require('util')
var debug = function() {}
if (util.debuglog) {
	debug = util.debuglog('request')
}

module.exports = exports = request

function canPipe(x) {
	return x && 'function' == typeof x.pipe
}

function request(raw, opt, cb) {
	if ('function' == opt) {
		cb = opt
		opt = {}
	}
	if (!/^http/i.test(raw)) {
		raw = 'http://' + raw
	}
	var parsed = url.parse(raw)
	var headers = opt.headers || {}
	var options = {
		  hostname: parsed.hostname
		, port: parsed.port
		, path: parsed.path
		, auth: parsed.auth || undefined
	}
	var method = 'GET'
	var body = opt.body || opt.data
	var type, len

	if (null != body) {
		// string, buffer, stream, object
		method = 'POST'
		if (Buffer.isBuffer(body)) {
			type = 'bin'
			len = body.length
		} else if (canPipe(body)) {
			type = 'bin'
		} else {
			// json
			try {
				body = JSON.stringify(body)
			} catch (e) {
				return cb(e)
			}
			type = 'application/json;charset=utf-8'
			len = Buffer.byteLength(body)
		}
	}

	if (type) headers['Content-Type'] = type
	if (len) headers['Content-Length'] = len

	options.headers = headers
	options.method = opt.method || method
	debug('options: %j', options)

	var req = http.request(options, function(res) {
		var body = ''
		res.on('data', function(buf) {
			body += buf
		}).on('end', function() {
			res.body = body
			cb(null, res, res.body)
		}).on('error', cb)
	}).on('error', cb)

	if (body) {
		if (canPipe(body)) {
			return body.pipe(req)
		} else {
			req.write(body)
		}
	}

	req.end()
}
