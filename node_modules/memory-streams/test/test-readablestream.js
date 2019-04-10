var streams = require('../')
  , should  = require('should');

// Definition
streams.should.not.be.undefined;
streams.should.have.property('ReadableStream');

// Read method
var reader = new streams.ReadableStream('Hello World\n');
reader.read().toString().should.equal('Hello World\n');
should.not.exist(reader.read());

// Append method
reader.append('Hello Universe\n');
reader.read().toString().should.equal('Hello Universe\n');

// Done
console.log( '> ReadableStream tests complete.');