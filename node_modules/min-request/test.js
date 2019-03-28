var http = require('http')
var assert = require('assert')
var fs = require('fs')
var request = require('./')
var pkg = require('./package')

var port = 10241

before(function(done) {
	http.createServer(function(req, res) {
		req.pipe(res)
	}).listen(port, function(err) {
		assert(!err)
		done()
	})
})

describe('request', function() {
	it('can request json', function(done) {
		var data = {foo: 'bar'}
		request('http://localhost:' + port, {body: data}, function(err, res, body) {
			assert.deepEqual(JSON.parse(body), data)
			done()
		})
	})

	it('do not need http:', function(done) {
		var data = {foo: 'bar'}
		request('localhost:' + port, {body: data}, function(err, res, body) {
			assert.deepEqual(JSON.parse(body), data)
			done()
		})
	})

	it('can request stream', function(done) {
		request('localhost:' + port, {
			  body: fs.createReadStream('package.json')
			, method: 'POST'
		}, function(err, res, body) {
			assert.deepEqual(pkg, JSON.parse(body))
			done()
		})
	})
})
