var Twig = require("../twig")
    , twig = Twig.twig
    , PATHS = require("./paths")
    , MINIMATCH = require("minimatch")
    , FS = require("fs")
    , WALK = require("walk");

exports.defaults = {
    compress: false
    , pattern: "*\\.twig"
    , recursive: false
};

exports.compile = function(options, files) {
    // Create output template directory if necessary
    if (options.output) {
        PATHS.mkdir(options.output);
    }
    
    files.forEach(function(file) {
        FS.stat(file, function(err, stats) {
            if (stats.isDirectory()) {
                parseTemplateFolder(file, options.pattern);
            } else if (stats.isFile()) {
                parseTemplateFile(file);
            } else {
                console.log("ERROR " + file + ": Unable to stat file");
            }
        });
    });
    
    function parseTemplateFolder(directory, pattern) {
        directory = PATHS.strip_slash(directory);
    
       // Get the files in the directory
       // Walker options
        var walker  = WALK.walk(directory, { followLinks: false })
            , files = [];

        walker.on('file', function(root, stat, next) {
            // normalize (remove / from end if present)
            root = PATHS.strip_slash(root);
        
            // match against file pattern
            var name = stat.name
                , file = root + '/' + name;
            if (MINIMATCH(name, pattern)) {
                parseTemplateFile(file, directory);
                files.push(file);
            }
            next();
        });

        walker.on('end', function() {
            // console.log(files);
        });
    }

    function parseTemplateFile(file, base) {
        if (base) base = PATHS.strip_slash(base);
        var split_file = file.split("/")
            , output_file_name = split_file.pop()
            , output_file_base = PATHS.findBase(file)
            , output_directory = options.output
            , output_base = PATHS.removePath(base, output_file_base)
            , output_id
            , output_file;

        if (output_directory) {
            // Create template directory
            if (output_base !== "") {
                PATHS.mkdir(output_directory + "/" + output_base);
                output_base += "/";
            }
            output_id = output_directory + "/" + output_base + output_file_name;
            output_file = output_id + ".js"
        } else {
            output_id = file;
            output_file = output_id + ".js"
        }
            
        var tpl = twig({
            id: output_id
            , path: file
            , load: function(template) {
                // compile!
                var output = template.compile(options);
            
                FS.writeFile(output_file, output, 'utf8', function(err) {
                    if (err) {
                        console.log("Unable to compile " + file + ", error " + err);
                    } else {
                        console.log("Compiled " + file + "\t-> " + output_file);
                    }
                });
            }
        });
    }
};