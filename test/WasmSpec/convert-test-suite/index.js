//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Copyright (c) 2021 ChakraCore Project Contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
/* eslint-env node */
const path = require("path");
const fs = require("fs-extra");
const Bluebird = require("bluebird");
const {spawn} = require("child_process");
const slash = require("slash");
const json5 = require("json5");
const walk = require("klaw");

Bluebird.promisifyAll(fs);
const config = json5.parse(fs.readFileSync(path.join(__dirname, "config.json5")));
const rlRoot = path.resolve(__dirname, "..");
const folders = config.folders.map(folder => path.resolve(rlRoot, folder));
const baselineDir = path.join(rlRoot, "baselines");

const argv = require("yargs")
  .help()
  .alias("help", "h")
  .options({
    rebase: {
      string: true,
      description: "Path to host to run the test create/update the baselines"
    }
  })
  .argv;

// Make sure all arguments are valid
for (const folder of folders) {
  if (!fs.statSync(folder).isDirectory()) {
    throw new Error(`${folder} is not a folder`);
  }
}

function hostFlags(specFile) {
  return `-wasm -args ${slash(specFile.relative)} -endargs`;
}

function getBaselinePath(specFile) {
  return `${slash(path.relative(rlRoot, path.join(baselineDir, specFile.relative.replace(specFile.ext, ""))))}.baseline`;
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

async function generateChakraTests() {
  const chakraTestsDestination = path.join(rlRoot, "chakra_generated");

  const chakraTests = require("./generateTests");
  await removePossiblyEmptyFolder(chakraTestsDestination);
  await fs.ensureDirAsync(chakraTestsDestination);
  await fs.writeFileAsync(path.join(chakraTestsDestination, ".keep"), "Committing an empty directory on the Windows side to work around a GVFS issue\n");
  for (const test of chakraTests) {
    const content = await test.getContent(rlRoot);
    if (!content) {
      return;
    }
    const testPath = path.join(chakraTestsDestination, `${test.name}.wast`);
    const copyrightHeader =
`;;-------------------------------------------------------------------------------------------------------
;; Copyright (C) Microsoft. All rights reserved.
;; Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
;;-------------------------------------------------------------------------------------------------------
`;
    await fs.writeFileAsync(testPath, `${copyrightHeader};;AUTO-GENERATED do not modify\n${content}`);
  }
}

function main() {
  let runs;
  return generateChakraTests(
    // Walk all the folders to find test files
  ).then(() => Bluebird.reduce(folders, (specFiles, folder) => new Promise((resolve) => {
    walk(folder)
      .on("data", item => {
        const ext = path.extname(item.path);
        const relative = path.relative(rlRoot, item.path);
        if ((
            (ext === ".wast" && item.path.indexOf(".fail") === -1) ||
            (ext === ".js")
          ) &&
          !config.excludes.includes(slash(relative))
        ) {
          specFiles.push({
            path: item.path,
            ext,
            dirname: path.dirname(item.path),
            relative
          });
        }
      })
      .on("end", () => {
        resolve(specFiles);
      });
  }), [])
  ).then(specFiles => {
    const runners = {
      ".wast": "spec.js",
      ".js": "jsapi.js",
    };
    runs = specFiles.map(specFile => {
      const {
        ext,
        relative
      } = specFile;
      const dirname = path.dirname(specFile.path);

      const useFeature = (allowRequired, feature) => !(allowRequired ^ feature.required) && (
        (feature.files || []).includes(slash(relative)) ||
        (feature.folders || []).map(folder => path.join(rlRoot, folder)).includes(dirname)
      );

      const isXplatExcluded = config["xplat-excludes"].indexOf(slash(relative)) !== -1;
      const baseline = getBaselinePath(specFile);
      const flags = hostFlags(specFile);
      const runner = runners[ext];

      const requiredFeature = config.features
        // Use first required feature found
        .filter(useFeature.bind(null, true))[0] ||
        // Or use default
        {rltags: [], flags: []};

      const tests = [{
        runner,
        tags: [].concat(requiredFeature.rltags || []),
        baseline,
        flags: [flags].concat(requiredFeature.flags || [])
      }, {
        runner,
        tags: ["exclude_dynapogo"].concat(requiredFeature.rltags || []),
        baseline,
        flags: [flags, "-nonative"].concat(requiredFeature.flags || [])
      }].concat(config.features
        .filter(useFeature.bind(null, false))
        .map(feature => ({
          runner,
          baseline,
          tags: [].concat(feature.rltags || []),
          flags: [flags].concat(feature.flags || [])
        }))
      );
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
    return fs.writeFileAsync(path.join(rlRoot, "rlexe.xml"), rlexe);
  }).then(() => {
    if (!argv.rebase) {
      return;
    }
    fs.removeSync(baselineDir);
    fs.ensureDirSync(baselineDir);
    const startRuns = runs.map(run => () => new Bluebird((resolve, reject) => {
      const test = run[0];
      fs.ensureDirSync(path.dirname(test.baseline));
      const baseline = fs.createWriteStream(test.baseline);
      baseline.on("open", () => {
        const args = [path.resolve(rlRoot, test.runner)].concat(test.flags);
        console.log(argv.rebase, args.join(" "));
        const engine = spawn(
          argv.rebase,
          args,
          {
            cwd: rlRoot,
            stdio: [baseline, baseline, baseline],
            shell: true
          }
        );
        engine.on("error", reject);
        engine.on("close", resolve);
      });
      baseline.on("error", reject);
    }));

    const cucurrentRuns = 8;
    let running = startRuns.splice(0, cucurrentRuns).map(start => start());
    return new Promise((resolve, reject) => {
      const checkIfContinue = () => {
        // Keep only runs that are not done
        running = running.filter(run => !run.isFulfilled());
        // If we have nothing left to run, terminate
        if (running.length === 0 && startRuns.length === 0) {
          return resolve();
        }
        running.push(...(startRuns.splice(0, cucurrentRuns - running.length).map(start => start())));
        Bluebird.any(running).then(checkIfContinue).catch(reject);
      };
      checkIfContinue();
    });
  });
}

main().then(() => console.log("done"), err => console.error(err));
