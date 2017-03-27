//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

const path = require("path");
const fs = require("fs-extra");
const stringArgv = require("string-argv");
const {spawn} = require("child_process");
const slash = require("slash");

const rlRoot = path.join(__dirname, "..");
const baselineDir = path.join(rlRoot, "baselines");

const argv = require("yargs")
  .help()
  .alias("help", "h")
  .options({
    suite: {
      string: true,
      alias: "s",
      description: "Path to the test suite",
      default: path.join(rlRoot, "testsuite"),
      demand: true,
    },
    excludes: {
      array: true,
      alias: "e",
      description: "Spec tests to exclude from the conversion (use for known failures)",
      default: []
    },
    "xplat-excludes": {
      array: true,
      description: "Spec tests to exclude when running on xplat (use for known failures)",
      default: [
        "address",
        "binary",
        "call",
        "call_indirect",
        "chakra_i64",
        "conversions",
        "fac",
        "float_exprs",
        "func",
        "func_ptrs",
        "i32",
        "i64",
        "imports",
        "int_exprs",
        "left-to-right",
        "linking",
        "memory_trap",
        "page",
        "resizing",
        "select",
        "skip-stack-guard-page",
        "start",
        "traps",
        "unreachable",
        "unwind"
      ]
    },
    rebase: {
      string: true,
      description: "Path to host to run the test create/update the baselines"
    }
  })
  .argv;

// Make sure all arguments are valid
fs.statSync(argv.suite).isDirectory();

function hostFlags(specFile) {
  return `-wasm -args ${slash(path.relative(rlRoot, specFile))} -endargs`;
}

function getBaselinePath(specFile) {
  return `${slash(path.relative(rlRoot, path.join(baselineDir, path.basename(specFile, path.extname(specFile)))))}.baseline`;
}

function removePossiblyEmptyFolder(folder) {
  return new Promise((resolve, reject) => {
    fs.remove(folder, err => {
      // ENOENT is the error code if the folder is missing
      if (err && err.code !== "ENOENT") {
        return reject(err);
      }
      resolve();
    });
  });
}

function main() {
  const chakraTestsDestination = path.join(argv.suite, "chakra");
  const chakraTests = require("./generateTests");

  return Promise.all([
    removePossiblyEmptyFolder(chakraTestsDestination),
  ]).then(() => {
    fs.ensureDirSync(chakraTestsDestination);
    return Promise.all(chakraTests.map(test => test.getContent(argv.suite)
      .then(content => new Promise((resolve, reject) => {
        if (!content) {
          return resolve();
        }
        const testPath = path.join(chakraTestsDestination, `${test.name}.wast`);
        const copyrightHeader =
`;;-------------------------------------------------------------------------------------------------------
;; Copyright (C) Microsoft. All rights reserved.
;; Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
;;-------------------------------------------------------------------------------------------------------
`;
        fs.writeFile(testPath, `${copyrightHeader};;AUTO-GENERATED do not modify\n${content}`, err => {
          if (err) {
            return reject(err);
          }
          return resolve(testPath);
        });
      }))));
  }).then(chakraTests => new Promise((resolve, reject) => {
    const specFiles = [...chakraTests];
    fs.walk(path.join(argv.suite, "core"))
      .on("data", item => {
        if (
          path.extname(item.path) === ".wast" &&
          item.path.indexOf(".fail") === -1 &&
          !argv.excludes.includes(path.basename(item.path, ".wast"))
        ) {
          specFiles.push(item.path);
        }
      })
      .on("end", () => {
        resolve(specFiles);
      });
  })).then(specFiles => new Promise((resolve, reject) => {
    fs.walk(path.join(argv.suite, "js-api"))
      .on("data", item => {
        if (path.extname(item.path) === ".js") {
          specFiles.push(item.path);
        }
      })
      .on("end", () => {
        resolve(specFiles);
      });
  })).then(specFiles => {
    const runners = {
      ".wast": "spec.js",
      ".js": "jsapi.js",
    };
    const runs = specFiles.map(specFile => {
      const ext = path.extname(specFile);
      const isXplatExcluded = argv.xplatExcludes.indexOf(path.basename(specFile, ext)) !== -1;
      const baseline = getBaselinePath(specFile);
      const flags = hostFlags(specFile);
      const tests = [{
        runner: runners[ext],
        tags: [],
        baseline: baseline,
        flags: [flags]
      }, {
        runner: runners[ext],
        tags: ["exclude_dynapogo"],
        baseline: baseline,
        flags: [flags, "-nonative"]
      }];
      if (isXplatExcluded) {
        for (const test of tests) test.tags.push("exclude_xplat");
      }
      return tests;
    });
    const rlexe = (
`<?xml version="1.0" encoding="utf-8"?>
<!-- Auto Generated by convert-test-suite -->
<regress-exe>${
  runs.map(run => run.map(test => `
  <test>
    <default>
      <files>${test.runner}</files>
      <baseline>${test.baseline}</baseline>
      <compile-flags>${test.flags.join(" ")}</compile-flags>${test.tags.length > 0 ? `
      <tags>${test.tags.join(",")}</tags>` : ""}
    </default>
  </test>`).join("")).join("")
}
</regress-exe>
`);
    return new Promise((resolve, reject) => {
      fs.writeFile(path.join(__dirname, "..", "rlexe.xml"), rlexe, err => err ? reject(err) : resolve(specFiles));
    });
  }).then(specFiles => {
    if (!argv.rebase) {
      return;
    }
    fs.removeSync(baselineDir);
    fs.ensureDirSync(baselineDir);
    return Promise.all(specFiles.map(specFile => new Promise((resolve, reject) => {
      const baseline = fs.createWriteStream(getBaselinePath(specFile));
      const args = [path.resolve(rlRoot, "spec.js"), "-nonative"].concat(stringArgv(hostFlags(specFile)));
      console.log(argv.rebase, args.join(" "));
      const engine = spawn(
        argv.rebase,
        args,
        {cwd: rlRoot}
      );
      engine.stdout.pipe(baseline);
      engine.stderr.pipe(baseline);
      engine.on("error", reject);
      engine.on("close", resolve);
    })));
  });
}

main().then(() => console.log("done"), err => console.error(err));
