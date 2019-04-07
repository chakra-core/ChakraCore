'use strict';

function _classCallCheck(instance, Constructor) { if (!(instance instanceof Constructor)) { throw new TypeError("Cannot call a class as a function"); } }

function _defineProperties(target, props) { for (var i = 0; i < props.length; i++) { var descriptor = props[i]; descriptor.enumerable = descriptor.enumerable || false; descriptor.configurable = true; if ("value" in descriptor) descriptor.writable = true; Object.defineProperty(target, descriptor.key, descriptor); } }

function _createClass(Constructor, protoProps, staticProps) { if (protoProps) _defineProperties(Constructor.prototype, protoProps); if (staticProps) _defineProperties(Constructor, staticProps); return Constructor; }

var _require = require('triple-beam'),
    MESSAGE = _require.MESSAGE;

var Printf =
/*#__PURE__*/
function () {
  function Printf(templateFn) {
    _classCallCheck(this, Printf);

    this.template = templateFn;
  }

  _createClass(Printf, [{
    key: "transform",
    value: function transform(info) {
      info[MESSAGE] = this.template(info);
      return info;
    }
  }]);

  return Printf;
}();
/*
 * function printf (templateFn)
 * Returns a new instance of the printf Format that creates an
 * intermediate prototype to store the template string-based formatter
 * function.
 */


module.exports = function (opts) {
  return new Printf(opts);
};

module.exports.Printf = module.exports.Format = Printf;