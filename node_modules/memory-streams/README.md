# Memory Streams JS
_Memory Streams JS_ is a light-weight implementation of the `Stream.Readable` and `Stream.Writable` abstract classes from node.js. You can use the classes provided to store the result of reading and writing streams in memory. This can be useful when you need pipe your test output for later inspection or to stream files from the web into memory without have to use temporary files on disk.

Forked from https://github.com/paulja/memory-streams-js to be modified
so that `.end()` calls emit a `finish` event.

## Installation
Install with:

```bash
npm install memory-streams --save
```

## Usage
Sample usage, using the `ReadableStream` class and piping:

```js
var streams = require('memory-streams');

// Initialize with the string
var reader = new streams.ReadableStream('Hello World\n');

// Send all output to stdout
reader.pipe(process.stdout); // outputs: "Hello World\n"

// Add more data to the stream
reader.append('Hello Universe\n'); // outputs "Hello Universe\n";
```

Using the `ReadableStream` class and reading manually:

```js
var streams = require('memory-streams');

// Initialize with the string
var reader = new streams.ReadableStream('Hello World\n');

// Add more data to the stream
reader.append('Hello Universe\n'); // outputs "Hello Universe\n";

// Read the data out
console.log(reader.read().toString()); // outputs: "Hello World\nHello Universe\n"
```
    
Using the `WritableStream` class and piping the contents of a file:

```js
var streams = require('memory-streams')
  , fs      = require('fs');

// Pipe 
var reader = fs.createReadStream('index.js');
var writer = new streams.WritableStream();
reader.pipe(writer);
reader.on('readable', function() {

  // Output the content as a string
  console.log(writer.toString());
  
  // Output the content as a Buffer
  console.log(writer.toBuffer());
});
```
    
You can also call the `write` method directly to store data to the stream:

```js
var streams = require('memory-streams');

// Write method
var writer = new streams.WritableStream();
writer.write('Hello World\n');

// Output the content as a string
console.log(writer.toString()); // Outputs: "Hello World\n"
```

For more examples you can look at the tests for the module.

## License
MIT

Copyright (c) 2017 Paul Jackson

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
