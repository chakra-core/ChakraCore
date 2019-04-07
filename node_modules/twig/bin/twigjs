#!/usr/bin/env node

var PATHS = require("../lib/paths")
    , COMPILE = require("../lib/compile")
    , options = COMPILE.defaults

    , args = process.argv
    , node = args.shift()
    , thisPath = args.shift().split("/")
    , thisFile = thisPath[thisPath.length-1]
    , files = []
    , arg;
    
if (args.length == 0) {
    process.stderr.write("ERR: No input files provided\n\n");
    printUsage(process.stderr);
}

while (args.length > 0) {
    arg = args.shift();
    switch (arg) {
        case "--help":
            printUsage(process.stdout);
            return;
        case "--output":
        case "-o":
            options.output = PATHS.strip_slash(args.shift());
            break;
        case "--pattern":
        case "-p":
            options.pattern = args.shift();
            break;
        case "--module":
        case "-m":
            options.module = args.shift();
            break;
        case "--twig":
        case "-t":
            options.twig = args.shift();
            break;
        default:
            files.push(arg);
    }
}

COMPILE.compile(options, files);

function printUsage(stream) {
    stream.write("Usage:\n\t");
    stream.write(thisFile + " [options] input.twig | directory ...\n");
    stream.write("\t_______________________________________________________________________________\n\n");
    stream.write("\t" + thisFile + " can take a list of files and/or a directories as input. If a file is\n");
    stream.write("\tprovided, it is compiled, if a directory is provided, all files matching *.twig\n");
    stream.write("\tin the directory are compiled. The pattern can be overridden with --pattern\n\n")
    stream.write("\t--help         Print this help message.\n\n");
    stream.write("\t--output ...   What directory should twigjs output to. By default twigjs will\n");
    stream.write("\t               write to the same directory as the input file.\n\n");
    stream.write("\t--module ...   Should the output be written in module format. Supported formats:\n");
    stream.write("\t                   node:  Node.js / CommonJS 1.1 modules\n");
    stream.write("\t                   amd:   RequireJS / Asynchronous modules (requires --twig)\n");
    stream.write("\t                   cjs2:  CommonJS 2.0 draft8 modules (requires --twig)\n\n");
    stream.write("\t--twig ...     Used with --module. The location relative to the output directory\n");
    stream.write("\t               of twig.js. (used for module dependency resolution).\n\n");
    stream.write("\t--pattern ...  If parsing a directory of files, what files should be compiled.\n");
    stream.write("\t               Defaults to *.twig.\n\n");
    stream.write("NOTE: This is currently very rough, incomplete and under development.\n\n");
}
