var streams = require('../')
  , should  = require('should')
  , fs      = require('fs');

// Definition
streams.should.not.be.undefined;
streams.should.have.property('WritableStream');

// Write method
var writer = new streams.WritableStream();
writer.toString().should.be.empty;
writer.write('Hello World\n');
writer.toString().should.equal('Hello World\n');
writer.toBuffer().should.not.be.null;
writer.toBuffer().toString().should.equal('Hello World\n');

// Write more
writer.write('Hello Universe\n');
writer.toString().should.equal('Hello World\nHello Universe\n');

// Pipe test
var content = fs.readFileSync('index.js');
var reader  = fs.createReadStream('index.js');
writer = new streams.WritableStream();
reader.pipe(writer);
reader.on('readable', function() {
  writer.toString().should.equal(content.toString());
});

// Done
console.log( '> WritableStream tests complete.');