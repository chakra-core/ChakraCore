"use strict";

var assert = require("assert");
var random = require("..").generate;
var testLength = 24700;

describe("randomstring.generate(options)", function() {

  it("returns a string", function() {
    var rds = random();
    assert.equal(typeof(rds), "string");
  });

  it("defaults to 32 characters in length", function() {
    assert.equal(random().length, 32);
  });

  it("accepts length as an optional first argument", function() {
    assert.equal(random(10).length, 10);
  });
  
  it("accepts length as an option param", function() {
    assert.equal(random({ length: 7 }).length, 7);
  });
  
  it("accepts 'numeric' as charset option", function() {
    var testData = random({ length: testLength, charset: 'numeric' });
    var search = testData.search(/\D/ig);
    assert.equal(search, -1);
  });
  
  it("accepts 'alphabetic' as charset option", function() {
    var testData = random({ length: testLength, charset: 'alphabetic' });
    var search = testData.search(/\d/ig);
    assert.equal(search, -1);
  });
  
  it("accepts 'hex' as charset option", function() {
    var testData = random({ length: testLength, charset: 'hex' });
    var search = testData.search(/[^0-9a-f]/ig);
    assert.equal(search, -1);
  });
  
  it("accepts custom charset", function() {
    var charset = "abc";
    var testData = random({ length: testLength, charset: charset });
    var search = testData.search(/[^abc]/ig);
    assert.equal(search, -1);
  });
  
  it("accepts readable option", function() {
    var testData = random({ length: testLength, readable: true });
    var search = testData.search(/[0OIl]/g);
    assert.equal(search, -1);
  });
  
  it("accepts 'uppercase' as capitalization option", function() {
    var testData = random({ length: testLength, capitalization: 'uppercase'});
    var search = testData.search(/[a-z]/g);
    assert.equal(search, -1);
  });
  
  it("accepts 'lowercase' as capitalization option", function() {
    var testData = random({ length: testLength, capitalization: 'lowercase'});
    var search = testData.search(/[A-Z]/g);
    assert.equal(search, -1);
  });

  it("returns unique strings", function() {
    var results = {};
    for (var i = 0; i < 1000; i++) {
      var s = random();
      assert.notEqual(results[s], true);
      results[s] = true;
    }
    return true;
  });

  it("returns unbiased strings", function() {
    var charset = 'abcdefghijklmnopqrstuvwxyz';
    var slen = 100000;
    var s = random({ charset: charset, length: slen });
    var counts = {};
    for (var i = 0; i < s.length; i++) {
      var c = s.charAt(i);
      if (typeof counts[c] === "undefined") {
        counts[c] = 0;
      } else {
        counts[c]++;
      }
    }
    var avg = slen / charset.length;
    Object.keys(counts).sort().forEach(function(k) {
      var diff = counts[k] / avg;
      assert(diff > 0.95 && diff < 1.05,
             "bias on `" + k + "': expected average is " + avg + ", got " + counts[k]);
    });
  });

});
