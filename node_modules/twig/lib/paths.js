var FS = require("fs")
    , sep_chr = '/';
    
exports.relativePath = function(base, file) {
    var base_path = exports.normalize(base.split(sep_chr)),
        new_path = [],
        val;

    // Remove file from url
    base_path.pop();
    base_path = base_path.concat(file.split(sep_chr));
    
    while (base_path.length > 0) {
        val = base_path.shift();
        if (val == ".") {
            // Ignore
        } else if (val == ".." && new_path.length > 0 && new_path[new_path.length-1] != "..") {
            new_path.pop();
        } else {
            new_path.push(val);
        }
    }

    return new_path.join(sep_chr);
};

exports.findBase = function(file) {
    var paths = exports.normalize(file.split(sep_chr));
    // we want everything before the file
    if (paths.length > 1) {
        // get rid of the filename
        paths.pop();
        return paths.join(sep_chr) + sep_chr;
    } else {
        // we're in the file directory
        return "";
    }
};

exports.removePath = function(path, file) {
    if (!path) return "";
    
    var base_path = exports.normalize(path.split(sep_chr))
        , file_path = exports.normalize(file.split(sep_chr))
        , val
        , file_val;
    
    // strip base path off of file path
    while(base_path.length > 0) {
        val = base_path.shift();
        if (val !== '') {
            file_val = file_path.shift();
        }
    }
    return file_path.join(sep_chr);
};

exports.normalize = function(file_arr) {
    var new_arr = []
        , val;
    while(file_arr.length > 0) {
        val = file_arr.shift();
        if (val !== '') {
            new_arr.push(val);
        }
    }
    return new_arr;
};

exports.strip_slash = function(path) {
    if (path.substr(-1) == '/') path = path.substring(0, path.length-1);
    return path;
};

exports.mkdir = function(dir) {
    try {
        FS.mkdirSync(dir);
    } catch (err) {
        if (err.code == "EEXIST")  {
            // ignore if it's a "EEXIST" exeption
        }  else {
            console.log(err);
            throw err;
        }
    }
};
