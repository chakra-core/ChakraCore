#!/usr/bin/env node

var html = require("../lib/html")
var fs = require('fs')
var concat = require('concat-stream')

var args = process.argv.slice(0)
// shift off node and script name
args.shift()
args.shift()

if (args.length > 0) processFiles(args)
else readStdin()

function readStdin() {
  var stdin = process.openStdin()
  stdin.pipe(concat(function concatted (buff) {
    process.stdout.write(html.prettyPrint(buff.toString(), {indent_size: 2}))
  }))
}

function processFiles(files) {
  if (files.length > 1) {
    files.map(function(filename) {
      prettifyFile(filename)
    })
    return
  }
  var str = fs.readFileSync(files[0]).toString()
  process.stdout.write(prettify(str))
}

function prettify(str) {
  return html.prettyPrint(str, {indent_size: 2})
}

function prettifyFile(filename) {
  fs.writeFileSync(filename, prettify(fs.readFileSync(filename).toString()))
}
