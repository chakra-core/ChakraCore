// https://github.com/DTrejo/run.js
// Used to run code in a directory and rerun it if any files are changed.
// Usage: ./cli.js servercode.js
// Where servercode.js is whatever js file you want node to run

// Excludes filetypes in the ignoreExtensions array
var util = require('util')
  , fs = require('fs')
  , path = require('path')
  , spawn = require('child_process').spawn
  , minimatch = require('minimatch')
  , child // child process which runs the user's code
  , ignoreExtensions = ['.dirtydb', '.db', '.styl', '.css']
  , ignoreFiles = [ 'node_modules' ]
  // switched out for coffee depending on extension.
  , node = 'node'
  , args = process.argv
  , signal = null

//
// Tell user the correct usage if they miss-type
//
if (args.length <= 2) {
  console.log('Found ' + (args.length - 2)
    , 'argument(s). Expected one or more.')
  console.log(
    'Usage: \n  runjs [SIGNAL] somecode.js [--args]\n'
    + 'Ex:  \n  runjs somecode.js --args'
    + '     \n  runjs SIGUSR2 somecode.js --args\n'
    )
  process.exit(1)
}

//
// If we define a signal, parse it out and then remove from the array
//
if (args[2].match(/^SIG/)) {
  signal = args.splice(2, 1)[0];
}

//
// use `coffee` if the file ends with .coffee
//
if (args[2].match(/\.coffee$/)) node = 'coffee'


//
// Exclude files based on .gitignore
//
if (fs.existsSync('.gitignore')) {
  ignoreFiles = ignoreFiles.concat(fs.readFileSync('.gitignore', 'utf8')
    .split('\n')
    .filter(function(s) {
      s = s.trim()
      return s.indexOf('#') !== 0 && s.length > 0
    })
  )
}

console.log('Watching', path.resolve('./'), 'and all sub-directories not'
  , 'excluded by your .gitignore. Will not monitor dotfiles.')

watchFiles(parseFolder('.'), restart) // watch all files, restart if problem
run()


process.stdin.setEncoding('utf8')

// executes the command given by the second argument
;function run() {
  // run the server
  var childArgs = args.slice(2) || []
  child = spawn(node, childArgs)

  // let the child's output escape.
  child.stdout.on('data', function(data) {
    process.stdout.write(data)
  })
  child.stderr.on('data', function(error) {
    process.stdout.write(error)
  })

  // let the user's typing get to the child
  process.stdin.pipe(child.stdin)

  console.log('\nStarting: ' + childArgs.join(' '))
}

;function restart() {
  // kill if running
  if (child) {
    if (signal) {
      child.kill(signal)
    } else {
      child.kill()
      run()
    }
  } else {
    run()
  }
}

/**
* Parses a folder recursively and returns a list of files
*
* @param root {String}
* @return {Array}
*/
;function parseFolder(root) {
  var fileList = []
    , files = fs.readdirSync(root)

  files.forEach(function(file) {
    var pathname = path.join(root, file)
      , stat = fs.lstatSync(pathname)

    if (stat === undefined) return

    // add files to file list
    if (!stat.isDirectory()) {
      fileList.push(pathname)

    // recur if directory
    } else {
      // don't watch folders matched by .gitignore regexes
      for (var i = 0, pattern; pattern = ignoreFiles[i]; i++) {
        if (minimatch(file, pattern)) {
          console.log('Found & ignored', './' + pathname, '; is listed in .gitignore')
          return
        }
      }
      fileList = fileList.concat(parseFolder(pathname))
    }
  })

  return fileList
}


/**
* Add change listener to files, which is called whenever one changes.
* Ignores certain extensions.
*
* @param files {Array}
* @param callback {function}
*/
;function watchFiles(files, callback) {

  var config = {  persistent: true, interval: 25 }
  files.forEach(function (file) {

    // don't watch dotfiles.
    if (file.indexOf('.') === 0) return

    // don't watch things with ignored extensions
    var ext = path.extname(file)
    if (ignoreExtensions.indexOf(ext) !== -1) {
      console.log('Found & ignored', './' + file, '; has ignored extension')
      return
    }

    // don't watch files matched by .gitignore regexes
    for (var i = 0, pattern; pattern = ignoreFiles[i]; i++) {
      if (minimatch(file, pattern)) {
        console.log('Found & ignored', './' + file, '; is listed in .gitignore')
        return
      }
    }

    // if any file changes, run the callback
    fs.watchFile(file, config, function (curr, prev) {

      if ((curr.mtime + '') != (prev.mtime + '')) {
        console.log('./' + file + ' changed')
        if (callback) callback()
      }
    })
  })
}
