var parse = require('../');

describe('spawn-args', function(){

  it('should be a function', function() {
    parse.should.be.a('function');
  })

  it('should parse some arguments', function() {
    var arr = parse('--host hello.com --port 80 --title "Apple Pie With Spaces" --num 20')

    arr.length.should.equal(8);
    arr[5].should.equal('"Apple Pie With Spaces"');
  })

  it('should parse multiple quotes properly', function() {
    var arr = parse('--data \'{"url": "http://example.com"}\'')

    arr.length.should.equal(2);
    arr[1].should.equal('\'{"url": "http://example.com"}\'');
  })

  it('should parse escaped quotes properly', function() {
    var arr = parse('--data "{\\"url\\": \\"http://example.com\\"}"')

    arr.length.should.equal(2);
    arr[1].should.equal('"{\\"url\\": \\"http://example.com\\"}"');
  })

  it('should parse escaped newlines properly', function() {
    var arr = parse('--host hello.com \\\n --port 80')

    arr.length.should.equal(4);
  })

  it('should remove quotes if passed a config', function() {
    var string = '--host "hello.com" --fruit "apples oranges"'
    var keepquotes = parse(string)
    var removequotes = parse(string, {
      removequotes:true
    })
    keepquotes[1].should.equal('"hello.com"')
    keepquotes[3].should.equal('"apples oranges"')
    removequotes[1].should.equal('hello.com')
    removequotes[3].should.equal('"apples oranges"')

  })

  it('should ALWAYS remove quotes if passed a config', function() {
    var string = '--host "hello.com" --fruit "apples oranges"'
    var keepquotes = parse(string)
    var removequotes = parse(string, {
      removequotes:'always'
    })
    keepquotes[1].should.equal('"hello.com"')
    keepquotes[3].should.equal('"apples oranges"')
    removequotes[1].should.equal('hello.com')
    removequotes[3].should.equal('apples oranges')

  })
})
