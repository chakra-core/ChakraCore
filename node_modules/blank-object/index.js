(function (global, factory) {
	typeof exports === 'object' && typeof module !== 'undefined' ? module.exports = factory() :
	typeof define === 'function' && define.amd ? define(factory) :
	global.BlankObject = factory();
}(this, function () { 'use strict';

	function BlankObject() {}
	BlankObject.prototype = Object.create(null);

	return BlankObject;

}));