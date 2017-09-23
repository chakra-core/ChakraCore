//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// Import the utility functionality.
import jobs.generation.Utilities

// Grab the github project name passed in
def project = GithubProject
def branch = GithubBranchName

def msbuildTypeMap = [
    'debug':'chk',
    'test':'test',
    'release':'fre'
]

// convert `machine` parameter to OS component of PR task name
def machineTypeToOSTagMap = [
    'Windows 7': 'Windows 7',       // Windows Server 2008 R2, equivalent to Windows 7
    'Windows_NT': 'Windows',        // Windows Server 2012 R2, equivalent to Windows 8.1 (aka Blue)
    'Ubuntu16.04': 'Ubuntu',
    'OSX': 'OSX'
]

def dailyRegex = 'dailies'

// ---------------
// HELPER CLOSURES
// ---------------

def CreateBuildTask = { isPR, buildArch, buildType, machine, configTag, buildExtra, testExtra, runCodeAnalysis, excludeConfigIf, nonDefaultTaskSetup ->
    if (excludeConfigIf && excludeConfigIf(isPR, buildArch, buildType)) {
        return // early exit: we don't want to create a job for this configuration
    }

    def config = "${buildArch}_${buildType}"
    config = (configTag == null) ? config : "${configTag}_${config}"

    // params: Project, BaseTaskName, IsPullRequest (appends '_prtest')
    def jobName = Utilities.getFullJobName(project, config, isPR)

    def testableConfig = buildType in ['debug', 'test'] && buildArch != 'arm'
    def analysisConfig = buildType in ['release'] && runCodeAnalysis

    def buildScript = "call .\\jenkins\\buildone.cmd ${buildArch} ${buildType} "
    buildScript += buildExtra ?: ''
    buildScript += analysisConfig ? ' "/p:runcodeanalysis=true"' : ''
    def testScript = "call .\\jenkins\\testone.cmd ${buildArch} ${buildType} "
    testScript += testExtra ?: ''
    def analysisScript = '.\\Build\\scripts\\check_prefast_error.ps1 . CodeAnalysis.err'

    def newJob = job(jobName) {
        // This opens the set of build steps that will be run.
        // This looks strange, but it is actually a method call, with a
        // closure as a param, since Groovy allows method calls without parens.
        // (Compare with '.each' method used above.)
        steps {
            batchFile(buildScript) // run the parameter as if it were a batch file
            if (testableConfig) {
                batchFile(testScript)
            }
            if (analysisConfig) {
                powerShell(analysisScript)
            }
        }
    }

    def msbuildType = msbuildTypeMap.get(buildType)
    def msbuildFlavor = "build_${buildArch}${msbuildType}"
    def archivalString = "test/${msbuildFlavor}.*,test/logs/**"
    archivalString += analysisConfig ? ',CodeAnalysis.err' : ''
    Utilities.addArchival(newJob, archivalString,
        '', // no exclusions from archival
        false, // doNotFailIfNothingArchived=false ~= failIfNothingArchived
        false) // archiveOnlyIfSuccessful=false ~= archiveAlways

    Utilities.setMachineAffinity(newJob, machine, 'latest-or-auto')
    Utilities.standardJobSetup(newJob, project, isPR, "*/${branch}")

    if (nonDefaultTaskSetup == null) {
        if (isPR) {
            def osTag = machineTypeToOSTagMap.get(machine)
            Utilities.addGithubPRTriggerForBranch(newJob, branch, "${osTag} ${config}")
        } else {
            Utilities.addGithubPushTrigger(newJob)
        }
    } else {
        // nonDefaultTaskSetup is e.g. DailyBuildTaskSetup (which sets up daily builds)
        // These jobs will only be configured for the branch specified below,
        // which is the name of the branch netci.groovy was processed for.
        // See list of such branches at:
        // https://github.com/dotnet/dotnet-ci/blob/master/jobs/data/repolist.txt
        nonDefaultTaskSetup(newJob, isPR, config)
    }
}

def CreateBuildTasks = { machine, configTag, buildExtra, testExtra, runCodeAnalysis, excludeConfigIf, nonDefaultTaskSetup ->
    [true, false].each { isPR ->
        ['x86', 'x64', 'arm'].each { buildArch ->
            ['debug', 'test', 'release'].each { buildType ->
                CreateBuildTask(isPR, buildArch, buildType, machine, configTag, buildExtra, testExtra, runCodeAnalysis, excludeConfigIf, nonDefaultTaskSetup)
            }
        }
    }
}

def CreateXPlatBuildTask = { isPR, buildType, staticBuild, machine, platform, configTag,
    xplatBranch, nonDefaultTaskSetup, customOption, testVariant, extraBuildParams ->

    def config = (platform == "osx" ? "osx_${buildType}" : "linux_${buildType}")
    def numConcurrentCommand = (platform == "osx" ? "sysctl -n hw.logicalcpu" : "nproc")

    config = (configTag == null) ? config : "${configTag}_${config}"
    config = staticBuild ? "static_${config}" : "shared_${config}"
    config = customOption ? customOption.replaceAll(/[-]+/, "_") + "_" + config : config

    // params: Project, BaseTaskName, IsPullRequest (appends '_prtest')
    def jobName = Utilities.getFullJobName(project, config, isPR)

    def infoScript = "bash jenkins/get_system_info.sh --${platform}"
    def buildFlag = buildType == "release" ? "" : (buildType == "debug" ? "--debug" : "--test-build")
    def staticFlag = staticBuild ? "--static" : ""
    def swbCheckFlag = (platform == "linux" && buildType == "debug" && !staticBuild) ? "--wb-check" : "";
    def icuFlag = (platform == "osx" ? "--icu=/usr/local/opt/icu4c/include" : "")
    def compilerPaths = (platform == "osx") ? "" : "--cxx=/usr/bin/clang++-3.8 --cc=/usr/bin/clang-3.8"
    def buildScript = "bash ./build.sh ${staticFlag} -j=`${numConcurrentCommand}` ${buildFlag} " +
                      "${swbCheckFlag} ${compilerPaths} ${icuFlag} ${customOption} ${extraBuildParams}"
    def testScript = "bash test/runtests.sh \"${testVariant}\""

    def newJob = job(jobName) {
        steps {
            shell(infoScript)
            shell(buildScript)
            shell(testScript)
        }
    }

    def archivalString = "out/build.log"
    Utilities.addArchival(newJob, archivalString,
        '', // no exclusions from archival
        true, // doNotFailIfNothingArchived=false ~= failIfNothingArchived (true ~= doNotFail)
        false) // archiveOnlyIfSuccessful=false ~= archiveAlways

    Utilities.setMachineAffinity(newJob, machine, 'latest-or-auto')
    Utilities.standardJobSetup(newJob, project, isPR, "*/${branch}")

    if (nonDefaultTaskSetup == null) {
        if (isPR) {
            def osTag = machineTypeToOSTagMap.get(machine)
            Utilities.addGithubPRTriggerForBranch(newJob, xplatBranch, "${osTag} ${config}")
        } else {
            Utilities.addGithubPushTrigger(newJob)
        }
    } else {
        // nonDefaultTaskSetup is e.g. DailyBuildTaskSetup (which sets up daily builds)
        // These jobs will only be configured for the branch specified below,
        // which is the name of the branch netci.groovy was processed for.
        // See list of such branches at:
        // https://github.com/dotnet/dotnet-ci/blob/master/jobs/data/repolist.txt
        nonDefaultTaskSetup(newJob, isPR, config)
    }
}

// Generic task to trigger clang-based cross-plat build tasks
def CreateXPlatBuildTasks = { machine, platform, configTag, xplatBranch, nonDefaultTaskSetup, extraBuildParams ->
    [true, false].each { isPR ->
        CreateXPlatBuildTask(isPR, "test", "", machine, platform,
            configTag, xplatBranch, nonDefaultTaskSetup, "--no-jit", "--variants disable_jit", extraBuildParams)

        ['debug', 'test', 'release'].each { buildType ->
            def staticBuildConfigs = [true, false]
            if (platform == "osx") {
                staticBuildConfigs = [true]
            }

            staticBuildConfigs.each { staticBuild ->
                CreateXPlatBuildTask(isPR, buildType, staticBuild, machine, platform,
                    configTag, xplatBranch, nonDefaultTaskSetup, "", "", extraBuildParams)
            }
        }
    }
}

def DailyBuildTaskSetup = { newJob, isPR, triggerName, groupRegex ->
    // The addition of triggers makes the job non-default in GitHub.
    if (isPR) {
        def triggerRegex = "(${dailyRegex}|${groupRegex}|${triggerName})"
        Utilities.addGithubPRTriggerForBranch(newJob, branch,
            triggerName, // GitHub task name
            "(?i).*test\\W+${triggerRegex}.*")
    } else {
        Utilities.addPeriodicTrigger(newJob, '@daily')
    }
}

def CreateStyleCheckTasks = { taskString, taskName, checkName ->
    [true, false].each { isPR ->
        def jobName = Utilities.getFullJobName(project, taskName, isPR)

        def newJob = job(jobName) {
            steps {
                shell(taskString)
            }
        }

        Utilities.standardJobSetup(newJob, project, isPR, "*/${branch}")
        if (isPR) {
            // Set PR trigger.
            Utilities.addGithubPRTriggerForBranch(newJob, branch, checkName)
        } else {
            // Set a push trigger
            Utilities.addGithubPushTrigger(newJob)
        }

        Utilities.setMachineAffinity(newJob, 'Ubuntu16.04', 'latest-or-auto')
    }
}

// ----------------
// INNER LOOP TASKS
// ----------------

CreateBuildTasks('Windows_NT', null, null, "-winBlue", true, null, null)

// Add some additional daily configs to trigger per-PR as a quality gate:
// x64_debug Slow Tests
CreateBuildTask(true, 'x64', 'debug',
    'Windows_NT', 'ci_slow', null, '-winBlue -includeSlow', false, null, null)
// x64_debug DisableJIT
CreateBuildTask(true, 'x64', 'debug',
    'Windows_NT', 'ci_disablejit', '"/p:BuildJIT=false"', '-winBlue -disablejit', false, null, null)
// x64_debug Lite
CreateBuildTask(true, 'x64', 'debug',
    'Windows_NT', 'ci_lite', '"/p:BuildLite=true"', '-winBlue -lite', false, null, null)
// x64_debug Legacy
CreateBuildTask(true, 'x64', 'debug',
    'Windows 7', 'ci_dev12', 'msbuild12', '-win7 -includeSlow', false, null, null)

// -----------------
// DAILY BUILD TASKS
// -----------------

if (!branch.endsWith('-ci')) {
    // build and test on Windows 7 with VS 2013 (Dev12/MsBuild12)
    CreateBuildTasks('Windows 7', 'daily_dev12', 'msbuild12', '-win7 -includeSlow', false,
        /* excludeConfigIf */ { isPR, buildArch, buildType -> (buildArch == 'arm') },
        /* nonDefaultTaskSetup */ { newJob, isPR, config ->
            DailyBuildTaskSetup(newJob, isPR,
                "Windows 7 ${config}",
                '(dev12|legacy)\\s+tests')})

    // build and test on the usual configuration (VS 2015) with -includeSlow
    CreateBuildTasks('Windows_NT', 'daily_slow', null, '-winBlue -includeSlow', false,
        /* excludeConfigIf */ null,
        /* nonDefaultTaskSetup */ { newJob, isPR, config ->
            DailyBuildTaskSetup(newJob, isPR,
                "Windows ${config}",
                'slow\\s+tests')})

    // build and test on the usual configuration (VS 2015) with JIT disabled
    CreateBuildTasks('Windows_NT', 'daily_disablejit', '"/p:BuildJIT=false"', '-winBlue -disablejit', true,
        /* excludeConfigIf */ null,
        /* nonDefaultTaskSetup */ { newJob, isPR, config ->
            DailyBuildTaskSetup(newJob, isPR,
                "Windows ${config}",
                '(disablejit|nojit)\\s+tests')})
}

// ----------------
// CODE STYLE TASKS
// ----------------

CreateStyleCheckTasks('./jenkins/check_copyright.sh', 'ubuntu_check_copyright', 'Copyright Check')
CreateStyleCheckTasks('./jenkins/check_eol.sh', 'ubuntu_check_eol', 'EOL Check')
CreateStyleCheckTasks('./jenkins/check_tabs.sh', 'ubuntu_check_tabs', 'Tab Check')

// --------------
// XPLAT BRANCHES
// --------------

// Explicitly enumerate xplat-incompatible branches, because we don't anticipate any future incompatible branches
def isXPlatCompatibleBranch = !(branch in ['release/1.1', 'release/1.1-ci', 'release/1.2', 'release/1.2-ci'])

// Include these explicitly-named branches
def isXPlatDailyBranch = branch in ['master', 'linux', 'xplat']
// Include some release/* branches (ignore branches ending in '-ci')
if (branch.startsWith('release') && !branch.endsWith('-ci')) {
    // Allows all current and future release/* branches on which we should run daily builds of XPlat configs.
    // RegEx matches branch names we should ignore (e.g. release/1.1, release/1.2, release/1.2-pre)
    includeReleaseBranch = !(branch =~ /^release\/(1\.[12](\D.*)?)$/)
    isXPlatDailyBranch |= includeReleaseBranch
}

// -----------------
// LINUX BUILD TASKS
// -----------------

if (isXPlatCompatibleBranch) {
    def osString = 'Ubuntu16.04'

    // PR and CI checks
    CreateXPlatBuildTasks(osString, "linux", "ubuntu", branch, null, "")

    // daily builds
    if (isXPlatDailyBranch) {
        CreateXPlatBuildTasks(osString, "linux", "daily_ubuntu", branch,
            /* nonDefaultTaskSetup */ { newJob, isPR, config ->
                DailyBuildTaskSetup(newJob, isPR,
                    "Ubuntu ${config}",
                    'linux\\s+tests')},
            /* extraBuildParams */ "--extra-defines=PERFMAP_TRACE_ENABLED=1")
    }
}

// ---------------
// OSX BUILD TASKS
// ---------------

if (isXPlatCompatibleBranch) {
    def osString = 'OSX'

    // PR and CI checks
    CreateXPlatBuildTasks(osString, "osx", "osx", branch, null, "")

    // daily builds
    if (isXPlatDailyBranch) {
        CreateXPlatBuildTasks(osString, "osx", "daily_osx", branch,
            /* nonDefaultTaskSetup */ { newJob, isPR, config ->
                DailyBuildTaskSetup(newJob, isPR,
                    "OSX ${config}",
                    'linux\\s+tests')},
            /* extraBuildParams */ "")
    }
}

// ------------
// HELP MESSAGE
// ------------

Utilities.createHelperJob(this, project, branch,
    "Welcome to the ${project} Repository",  // This is prepended to the help message
    "For additional documentation on ChakraCore CI checks, please see:\n" +
    "\n" +
    "* https://github.com/Microsoft/ChakraCore/wiki/Jenkins-CI-Checks\n" +
    "* https://github.com/Microsoft/ChakraCore/wiki/Jenkins-Build-Triggers\n" +
    "* https://github.com/Microsoft/ChakraCore/wiki/Jenkins-Repro-Steps\n" +
    "\n" +
    "Have a nice day!")  // This is appended to the help message.  You might put known issues here.
