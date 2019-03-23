'use strict';

const vlq = require('./vlq');

const FIELDS = ['generatedColumn', 'source', 'originalLine', 'originalColumn', 'name'];

class Coder {
  decode(mapping) {
    let value = this.rawDecode(mapping);
    let output = {};

    for (let i=0; i<FIELDS.length;i++) {
      let field = FIELDS[i];
      let prevField = 'prev_' + field;
      if (value.hasOwnProperty(field)) {
        output[field] = value[field];
        if (typeof this[prevField] !== 'undefined') {
          output[field] += this[prevField];
        }
        this[prevField] = output[field];
      }
    }
    return output;
  }

  encode(value) {
    let output = '';
    for (let i=0; i<FIELDS.length;i++) {
      let field = FIELDS[i];
      if (value.hasOwnProperty(field)){
        let prevField = 'prev_' + field;
        let valueField = value[field];
        if (typeof this[prevField] !== 'undefined') {
          output += vlq.encode(valueField-this[prevField]);
        } else {
          output += vlq.encode(valueField);
        }
        this[prevField] = valueField;
      }
    }
    return output;
  }

  resetColumn() {
    this.prev_generatedColumn = 0;
  }

  adjustLine(n) {
    this.prev_originalLine += n;
  }

  rawDecode(mapping) {
    let buf = {rest: 0};
    let output = {};
    let fieldIndex = 0;
    while (fieldIndex < FIELDS.length && buf.rest < mapping.length) {
      vlq.decode(mapping, buf.rest, buf);
      output[FIELDS[fieldIndex]] = buf.value;
      fieldIndex++;
    }
    return output;
  }

  copy() {
    let c = new Coder();
    let key;
    for (let i = 0; i < FIELDS.length; i++) {
      key = 'prev_' + FIELDS[i];
      c[key] = this[key];
    }
    return c;
  }

  serialize() {
    let output = '';
    for (let i=0; i<FIELDS.length;i++) {
      let valueField = this['prev_' + FIELDS[i]] || 0;
      output += vlq.encode(valueField);
    }
    return output;
  }

  add(other) {
    this._combine(other, function(a,b){return a + b; });
  }

  subtract(other) {
    this._combine(other, function(a,b){return a - b; });
  }

  _combine(other, operation) {
    let key;
    for (let i = 0; i < FIELDS.length; i++) {
      key = 'prev_' + FIELDS[i];
      this[key] = operation((this[key] || 0), other[key] || 0);
    }
  }

  debug(mapping) {
    let buf = {rest: 0};
    let output = [];
    let fieldIndex = 0;
    while (fieldIndex < FIELDS.length && buf.rest < mapping.length) {
      vlq.decode(mapping, buf.rest, buf);
      output.push(buf.value);
      fieldIndex++;
    }
    return output.join('.');
  }
}

module.exports = Coder;
