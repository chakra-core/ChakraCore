//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// Import the utility functionality.
import jobs.generation.Utilities;

// Grab the github project name passed in
def project = GithubProject

def msbuildTypeMap = [
    'debug':'chk',
    'test':'test',
    'release':'fre'
]

def dailyRegex = 'dailies'

// ---------------
// HELPER CLOSURES
// ---------------

def CreateBuildTasks = { machine, configTag, buildExtra, testExtra, excludeConfigIf, nonDefaultTaskSetup ->
    [true, false].each { isPR ->
        ['x86', 'x64', 'arm'].each { buildArch ->
            ['debug', 'test', 'release'].each { buildType ->
                if (excludeConfigIf && excludeConfigIf(isPR, buildArch, buildType)) {
                    return // early exit: we don't want to create a job for this configuration
                }

                def config = "${buildArch}_${buildType}"
                config = (configTag == null) ? config : "${configTag}_${config}"

                // params: Project, BaseTaskName, IsPullRequest (appends '_prtest')
                def jobName = Utilities.getFullJobName(project, config, isPR)

                def testableConfig = buildType in ['debug', 'test'] && buildArch != 'arm'
                def analysisConfig = buildType in ['release']

                def buildScript = "call jenkins.buildone.cmd ${buildArch} ${buildType}"
                buildScript += buildExtra ?: ''
                buildScript += analysisConfig ? ' "/p:runcodeanalysis=true"' : ''
                def testScript = "call jenkins.testone.cmd ${buildArch} ${buildType}"
                testScript += testExtra ?: ''
                def analysisScript = ".\\Build\\scripts\\check_prefast_error.ps1 . CodeAnalysis.err"

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

                Utilities.setMachineAffinity(newJob, machine)

                def msbuildType = msbuildTypeMap.get(buildType)
                def msbuildFlavor = "build_${buildArch}${msbuildType}"

                def archivalString = "test/${msbuildFlavor}.*,test/logs/**"
                archivalString += analysisConfig ? ',CodeAnalysis.err' : ''

                Utilities.addArchival(newJob, archivalString,
                    '', // no exclusions from archival
                    false, // doNotFailIfNothingArchived=false ~= failIfNothingArchived
                    false) // archiveOnlyIfSuccessful=false ~= archiveAlways

                if (nonDefaultTaskSetup == null)
                {
                    // This call performs remaining common job setup on the newly created job.
                    // This is used most commonly for simple inner loop testing.
                    // It does the following:
                    //   1. Sets up source control for the project.
                    //   2. Adds a push trigger if the job is a PR job
                    //   3. Adds a github PR trigger if the job is a PR job.
                    //      The optional context (label that you see on github in the PR checks) is added.
                    //      If not provided the context defaults to the job name.
                    //   4. Adds standard options for build retention and timeouts
                    //   5. Adds standard parameters for PR and push jobs.
                    //      These allow PR jobs to be used for simple private testing, for instance.
                    // See the documentation for this function to see additional optional parameters.
                    Utilities.simpleInnerLoopJobSetup(newJob, project, isPR, "Windows ${config}")
                }
                else
                {
                    Utilities.standardJobSetup(newJob, project, isPR)
                    nonDefaultTaskSetup(newJob, isPR, config)
                }
            }
        }
    }
}

def DailyBuildTaskSetup = { newJob, isPR, triggerName, groupRegex ->
    // The addition of triggers makes the job non-default in GitHub.
    if (isPR) {
        def triggerRegex = "(${dailyRegex}|${groupRegex}|${triggerName})"
        Utilities.addGithubPRTrigger(newJob,
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

        Utilities.simpleInnerLoopJobSetup(newJob, project, isPR, checkName)
        Utilities.setMachineAffinity(newJob, 'Ubuntu14.04', 'latest-or-auto')
    }
}

// ----------------
// INNER LOOP TASKS
// ----------------

CreateBuildTasks('Windows_NT', null, null, null, null, null)

// -----------------
// DAILY BUILD TASKS
// -----------------

// build and test on Windows 7 with VS 2013 (Dev12/MsBuild12)
CreateBuildTasks('Windows 7', 'daily_dev12', ' msbuild12', ' -win7',
    /* excludeConfigIf */ { isPR, buildArch, buildType -> (buildArch == 'arm') },
    /* nonDefaultTaskSetup */ { newJob, isPR, config ->
        DailyBuildTaskSetup(newJob, isPR,
            "Windows 7 ${config}",
            'legacy\\s+tests')})

// build and test on the usual configuration (VS 2015) with -includeSlow
CreateBuildTasks('Windows_NT', 'daily_slow', null, ' -includeSlow', null,
    /* nonDefaultTaskSetup */ { newJob, isPR, config ->
        DailyBuildTaskSetup(newJob, isPR,
            "Windows ${config}",
            'slow\\s+tests')})

// build and test on the usual configuration (VS 2015) with JIT disabled
CreateBuildTasks('Windows_NT', 'daily_disablejit', ' "/p:BuildJIT=false"', ' -disablejit', null,
    /* nonDefaultTaskSetup */ { newJob, isPR, config ->
        DailyBuildTaskSetup(newJob, isPR,
            "Windows ${config}",
            '(disablejit|nojit)\\s+tests')})

// ----------------
// CODE STYLE TASKS
// ----------------

CreateStyleCheckTasks('./jenkins.check_eol.sh', 'ubuntu_check_eol', 'EOL Check')
CreateStyleCheckTasks('./jenkins.check_copyright.sh', 'ubuntu_check_copyright', 'Copyright Check')
