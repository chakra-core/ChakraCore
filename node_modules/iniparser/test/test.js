var assert = require('assert'),
	iniparser = require('../lib/node-iniparser');

module.exports = {
	'async read file': function(){
		iniparser.parse('./files/test.ini', function(err, config){
			assert.equal(err, null);
		});
	},
	'async read non-existing file': function(){
		iniparser.parse('./files/doesnotexists.ini', function(err, config){
			assert.equal(err.code, 'ENOENT');
			assert.equal(config, null);
		});
	},
	'sync read file': function(){
		var config = iniparser.parseSync('./files/test.ini');
		assert.notEqual(config, null);
	},
	'sync read non-existing file': function(){
		assert.throws(function(){
			var config = iniparser.parseSync('./files/doesnotexists.ini');
		});
	},
	'async read file and look for variable': function(){
		iniparser.parse('./files/test.ini', function(err, config){
			assert.equal(err, null);
			assert.equal(config.foo, 'bar');
		});
	},
	'async read file and look for section': function(){
		iniparser.parse('./files/test.ini', function(err, config){
			assert.equal(err, null);
			assert.notEqual(config.worlds, null);
			assert.equal(config.worlds.earth, 'awesome');
			assert.notEqual(config.section2, null);
			assert.equal(config.section2.bar, 'foo');
		});
	},
	'sync read file and look for variable': function(){
		var config = iniparser.parseSync('./files/test.ini');
		assert.equal(config.foo, 'bar');
	},
	'sync read file and look for section': function(){
		var config = iniparser.parseSync('./files/test.ini');
		assert.notEqual(config.worlds, null);
		assert.equal(config.worlds.earth, 'awesome');
		assert.notEqual(config.section2, null);
		assert.equal(config.section2.bar, 'foo');
	},
	'variable with space at the end': function () {
		var config = iniparser.parseSync('./files/test.ini');
		assert.notEqual('bar ', config.var_with_space_at_end);
		assert.equal('bar', config.var_with_space_at_end);
	},
	'look for a commented out variable': function(){
		var config = iniparser.parseSync('./files/test.ini');
		assert.equal(config.section2.test, null);
	},
	'variable with space in value': function(){
		var config = iniparser.parseSync('./files/test.ini');
		assert.equal(config.section2.there_is, "a space in here with = and trailing tab");
	}
};
