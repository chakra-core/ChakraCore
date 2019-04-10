/*jslint nodejs:true, indent:4 */
/**
 * Imports
 */
//node
var util = require(process.binding('natives').util ? 'util' : 'sys');
var path = require('path');
var fs = require('fs');

//internal
var parser = require('./parser');
var formatter = require('./formatter');

//Is a tty function
var isAtty;
try {
    isAtty = require('tty').isatty;
} catch (e) {
    isAtty = function () {
        return false;
    };
}


var __readDirectory = function (path, callback, filter) {
    if (filter) {
        // process filter. are we too deep yet?
        if (!filter.depthAt) {
            filter.depthAt = 1;// initialize what depth we are at
        }
        if (filter.depth && filter.depth < filter.depthAt) {
            callback(undefined, []);// we are too deep. return "nothing found"
            return;
        }
    }
    // queue up a "readdir" file system call (and return)
    fs.readdir(path, function (err, files) {
        var doHidden, count, countFolders, data;

        if (err) {
            callback(err);
            return;
        }
        doHidden = false;               // true means: process hidden files and folders
        if (filter && filter.hidden) {
            doHidden = true;                // filter requests to process hidden files and folders
        }
        count = 0;                      // count the number of "stat" calls queued up
        countFolders = 0;               // count the number of "folders" calls queued up
        data = [];                      // the data to return

        // iterate over each file in the dir
        files.forEach(function (name) {
            var obj, processFile;

            // ignore files that start with a "." UNLESS requested to process hidden files and folders
            if (doHidden || name.indexOf(".") !== 0) {
                // queue up a "stat" file system call for every file (and return)
                count += 1;
                fs.stat(path + "/" + name, function (err, stat) {
                    if (err) {
                        callback(err);
                        return;
                    }
                    processFile = true;
                    if (filter && filter.callback) {
                        processFile = filter.callback(name, stat, filter);
                    }
                    if (processFile) {
                        obj = {};
                        obj.name = name;
                        obj.stat = stat;
                        data.push(obj);
                        if (stat.isDirectory()) {

                            countFolders += 1;
                            // perform "readDirectory" on each child folder (which queues up a readdir and returns)
                            (function (obj2) {
                                // obj2 = the "obj" object
                                __readDirectory(path + "/" + name, function (err, data2) {
                                    if (err) {
                                        callback(err);
                                        return;
                                    }
                                    // entire child folder info is in "data2" (1 fewer child folders to wait to be processed)
                                    countFolders -= 1;
                                    obj2.children = data2;
                                    if (countFolders <= 0) {
                                        // sub-folders found. This was the last sub-folder to processes.
                                        callback(undefined, data);      // callback w/ data
                                    } else {
                                        // more children folders to be processed. do nothing here.
                                    }
                                });
                            }(obj));
                        }
                    }
                    // 1 more file has been processed (or skipped)
                    count -= 1;
                    if (count <= 0) {
                        // all files have been processed.
                        if (countFolders <= 0) {
                            // no sub-folders were found. DONE. no sub-folders found
                            callback(undefined, data);      // callback w/ data
                        } else {
                            // children folders were found. do nothing here (we are waiting for the children to callback)
                        }
                    }
                });
            }
        });
        if (count <= 0) {   // if no "stat" calls started, then this was an empty folder
            callback(undefined, []);        // callback w/ empty
        }
    });
};


/**
 * Launcher constructor
 *
 * @constructor
 * @param {Object} options
 */
function Launcher(options) {
    this._parser = null;
    this._formatter = null;
    this._silent = false;

    this._stdout = process.stdout;
    this._stderr = process.stderr;
    this._colors = 'auto';

    this.configure(options);
}

/**
 * Configure the launcher
 *
 * @param {Object} options
 * @return this
 */
Launcher.prototype.configure = function (options) {
    if (options) {
        if (options.silent !== undefined) {
            this._silent = options.silent;
        }

        if (options.stdout !== undefined) {
            this._stdout = options.stdout;

            //Force reconfiguration
            options.formatter = options.formatter || {};
            options.formatter.colors = this._colors;
        }

        if (options.parser !== undefined) {
            this._getParser(options.parser);
        }

        if (options.formatter !== undefined) {
            if (options.formatter.colors !== undefined) {
                this._colors = options.formatter.colors;
            }

            options.formatter.colors = (this._colors === 'auto') ? this._guessColors() : this._colors;
            this._getFormatter(options.formatter);
        }
    }

    return this;
};

/**
 *
 * @param {Array} files
 * @param {Function} callback
 * @return
 */
Launcher.prototype.validateFiles = function (files, callback) {
    var report, onValidateFileCount, onValidateFile, thisLauncher;

    thisLauncher = this;
    report = new formatter.Report();
    parser = this._getParser();

    onValidateFileCount = files.length;
    onValidateFile = function (error, fileName, reportFile) {
        onValidateFileCount -= 1;

        if (reportFile) {
            report.addFile(fileName, reportFile);
        }

        if (onValidateFileCount <= 0) {
            if (callback) {
                callback(null, report);
            }
        }
    };

    files.forEach(function (fileName) {
        thisLauncher._validateFile(fileName, function (error, reportFile) {
            onValidateFile(error, fileName, reportFile);
        });
    });
};

/**
 * Run the executer with specified args
 *
 * @param {Object} args
 * @return {int} code
 */
Launcher.prototype.run = function (args) {
    var thisLauncher, searchFilter, searchResult, onFileFoundCount, onFileFound;

    thisLauncher = this;

    searchResult = [];
    searchFilter = function (fileName, fileStat) {
        if (fileName.substring(fileName.length - 3) !== '.js') {
            return false;
        }
        return true;
    };

    if (args.length === 0) {
        args.push(process.cwd());
    }


    onFileFoundCount = args.length;
    onFileFound = function (error, filesFound) {
        searchResult = searchResult.concat(filesFound);
        onFileFoundCount -= 1;
        if (onFileFoundCount <= 0) {
            thisLauncher.validateFiles(searchResult, function (error, report) {
                var formatted;

                try {
                    formatted = thisLauncher._formatReport(report);
                    thisLauncher.printMessage(formatted, function (error, result) {
                        process.exit(report.length > 0 ? 1 : 0);
                    });
                } catch (e) {
                    thisLauncher.printError(e, true);
                }
            });
        }
    };

    args.forEach(function (searchRoot) {
        thisLauncher._findFiles(searchRoot, searchFilter, onFileFound);
    });

    return this;
};

Launcher.prototype._guessColors = function (args) {
    return isAtty(this._stdout) && (!fs.fstatSync(process.stdout.fd).isFile());
};

Launcher.prototype._findFiles = function (searchRoot, searchFilter, callback) {
    var files, searchFind;

    searchFilter = searchFilter || function () {
        return true;
    };


    files = [];
    searchRoot = searchRoot || process.cwd();
    searchFind = function (result, rootPath, filter) {
        var files = [], i;
        rootPath = rootPath || '/';
        if (! Array.isArray(result)) {
            if (result.stat.isFile()) {
                if (! filter(rootPath, result.stat)) {
                    return files;
                }
                files.push(rootPath);

            } else {
                result.children.forEach(function (child) {
                    files = files.concat(searchFind(child, path.join(rootPath, child.name), filter));
                });
            }
        } else {
            result.forEach(function (file) {
                files = files.concat(searchFind(file, path.join(rootPath, file.name), filter));
            });
        }
        return files;
    };

    fs.stat(searchRoot, function (error, stat) {
        if (error) {
            if (callback) {
                callback(error);
            }
        } else {
            if (stat.isDirectory()) {
                __readDirectory(searchRoot, function (error, result) {
                    if (error) {
                        if (callback) {
                            callback(error);
                        }
                    } else {
                        if (result) {
                            files = searchFind(result, searchRoot, searchFilter);
                        }
                        callback(undefined, files);
                    }
                });

            } else {
                fs.realpath(searchRoot, function (error, resolvedPath) {
                    files.push(resolvedPath);
                    callback(undefined, files);
                });
            }
        }
    });

};

Launcher.prototype._validateFile = function (filePath, callback) {
    fs.readFile(filePath, function (error, data) {
        var report;

        if (error) {
            callback(error);
        } else {
            data = data.toString('utf-8');

            // remove any shebangs
            data = data.replace(/^\#\!.*/, '');

            parser = this._getParser();
            parser.reset().update(data);

            report = parser.getReport();
            report.forEach(function (error) {
                error.file = filePath;
            });


            callback(undefined, report);
        }
    }.bind(this));
};

Launcher.prototype._formatReport = function (report) {
    formatter = this._getFormatter();
    return formatter.format(report);
};

Launcher.prototype._getParser = function (options) {
    if (!this._parser) {
        this._parser = new parser.Parser(options);
    } else if (options) {
        this._parser.configure(options);
    }
    return this._parser;
};

Launcher.prototype._getFormatter = function (options) {
    if (!this._formatter) {
        this._formatter = new formatter.Formatter(options);
    } else if (options) {
        this._formatter.configure(options);
    }
    return this._formatter;
};

/**
 * Print message to stdout
 *
 * @param {string} message
 * @param {Function} callback
 * @return this
 */
Launcher.prototype.printMessage = function (message, callback) {
    if (this._silent) {
        callback();
    } else {
        this._stdout.write('' + message, this._getFormatter().encoding, callback);
    }
    return this;
};

/**
 * Print error message
 *
 * @param {string} error
 * @param {int} exit
 * @return this
 */
Launcher.prototype.printError = function (error, exit) {
    util.puts(error);
    if (exit === undefined || exit) {
        process.exit(1);
    }
    return this;
};


/**
 * Exports
 */
exports.Launcher = Launcher;


/**
 * Main
 */
if (__filename === process.argv[1]) {
    var usage, args, arg, positionals, launcher, options, configFile;



    //called as main executable

    usage = "Usage: " + process.argv[0] + " file.js [dir1 file2 dir2 ...] [options]\n" +
    "Options:\n\n" +
    "  --config=FILE     the path to a JSON file with JSLINT options\n" +
    "  --formatter=FILE   optional path to a /dir/dir/file.hs file to customize the output\n" +
    "  -h, --help        display this help and exit\n" +
    "  -v, --version     output version information and exit";

    args = process.argv.splice(2);
    positionals = [];
    options = {
        formatter: {
        },
        parser: {
        }
    };
    configFile = process.env.NODELINT_CONFIG_FILE;

    launcher = new Launcher();
    while (args.length !== 0) {
        arg = args.shift();
        switch (arg) {
        case '-v':
        case '--version':
            var content, pkg;
            content = fs.readFileSync(path.join(__filename, '..', '..', '..', 'package.json'), 'utf8');
            pkg = JSON.parse(content);
            util.puts(pkg.version);
            process.exit(0);
            break;
        case '-h':
        case '--help':
            util.puts(usage);
            process.exit(0);
            break;
        case '--silent':
            options.silent = true;
            break;
        case '--pretty':
            options.formatter.pretty = true;
            break;
        case '--no-color':
        case '--no-colors':
            options.formatter.colors = false;
            break;
        default:
            if (arg.indexOf('--formatter') >= 0) {
                options.formatter.type = 'callback';
                options.formatter.callback = fs.readFileSync(arg.split('=')[1], 'utf8');
            } else if (arg.indexOf('--format') >= 0) {
                options.formatter.type = arg.split('=')[1];
            } else if (arg.indexOf('--mode') >= 0) {
                options.formatter.mode = arg.split('=')[1];
            } else if (arg.indexOf('--config') >= 0) {
                var file, source;

                configFile = arg.split('=')[1];

            } else {
                positionals.push(arg);
            }
        }
    }

    //Load default config file from environment
    if (configFile) {
        var source;

        //read config file
        try {
            source = fs.readFileSync(configFile, 'utf8');
        } catch (e) {
            launcher.printError('Read error when accessing "' + configFile + '".');
        }

        //Remove comments
        source = source.replace(/\/\*.+?\*\/|\/\/.*(?=[\n\r])/g, '');

        //Parse config
        try {
            source = JSON.parse(source);
        } catch (e) {
            console.log(e.toString());
            launcher.printError('Parse Error in "' + configFile + '"');
        }


        try {
            launcher.configure(source);
        } catch (e) {
            console.log(e.toString());
            launcher.printError('Parse Error in "' + configFile + '"');
        }
    }

    try {
        launcher.configure(options);
    } catch (e) {
        launcher.printError('Configuration : ' + e.toString());
    }

    try {
        launcher.run(positionals);
    } catch (e) {
        launcher.printError('Execution : ' + e.toString());
    }
}

