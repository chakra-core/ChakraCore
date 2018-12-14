import subprocess
import sys

#os = "Linux"
os = "OSX"

if len(sys.argv) >= 2:
    os = sys.argv[1]

# setup vars
branch = "master"

def CreateXPlatBuildTasks(machine, platform, configTag, xplatBranch, nonDefaultTaskSetup, extraBuildParams):
    isPRArr = {True, False}
    for isPR in isPRArr:
        CreateXPlatBuildTask(isPR, "test", "", machine, platform, configTag, xplatBranch, nonDefaultTaskSetup, "--no-jit", "--variants disable_jit", extraBuildParams)

        buildTypeArr = {"debug", "test", "release"}
        for buildType in buildTypeArr:
            staticBuildConfigs = {True, False}
            if platform is "osx":
                staticBuildConfigs = {True}

            for staticBuild in staticBuildConfigs:
                CreateXPlatBuildTask(isPR, buildType, staticBuild, machine, platform, configTag, xplatBranch, nonDefaultTaskSetup, "", "", extraBuildParams)


def CreateXPlatBuildTask(isPR, buildType, staticBuild, machine, platform, configTag, xplatBranch, nonDefaultTaskSetup, customOption, testVariant, extraBuildParams):
    config = "osx_"+buildType if platform is "osx" else "linux_"+buildType
    numConcurrentCommand = "sysctl -n hw.logicalcpu" if platform is "osx" else "nproc"
    
    config = config if configTag is None else configTag + "_" + config
    config = "static_"+config if staticBuild else "shared"+config
    # todo: config = customOption.replaceAll(/[-]+/, "_") + "_" + config if customOption else config

    '''todo: jobName = Utilities.getFullJobName(project, config, isPR)'''

    infoScript = "bash jenkins/get_system_info.sh --"+platform

    buildFlag = "" if buildType is "release" else ("--debug" if buildType is "debug" else "--test-build")

    staticFlag = "--static" if staticBuild else ""

    swbCheckFlag = "--wb-check" if platform is "linux" and buildType is "debug" and not staticBuild else ""

    # todo: icuFlag = "--icu=/Users/DDITLABS/homebrew/opt/icu4c/include" if platform == "osx" else ""
    # temp replacement:
    icuFlag = "--system-icu" if platform == "osx" else ""

    compilerPaths = "" if platform is "osx" else "--cxx=/usr/bin/clang++-3.9 --cc=/usr/bin/clang-3.9"

    buildScript = "bash ./build.sh "+staticFlag+" "+buildFlag+" "+swbCheckFlag+" "+compilerPaths+" "+icuFlag+" "+customOption+" "+extraBuildParams

    # todo: icuLibFlag = "--iculib=/Users/DDITLABS/homebrew/opt/icu4c" if platform == "osx" else ""
    # temp replacement:
    icuLibFlag = ""

    testScript = "bash test/runtests.sh "+icuLibFlag+" \""+testVariant+"\""

    exeBashStr(infoScript)
    exeBashStr(buildScript, "-j=`"+numConcurrentCommand+"`")
    exeBashStr(testScript)

def exeBashStr(bashStr, j=None):
    bashStrList = bashStr.split(" ")
    bashStrList = filter(lambda a: a != "", bashStrList)
    if j:
        bashStrList.append(j)
    print(bashStrList)
    subprocess.call(bashStrList)


# Linux build tasks:
if os == "Linux":
    osString = 'Ubuntu16.04'
    CreateXPlatBuildTasks(osString, "linux", "ubuntu", branch, None, "")

    isPRArr = {True, False}
    for isPR in isPRArr:
        CreateXPlatBuildTask(isPR, "debug", True, osString, "linux", "ubuntu", branch, None, "--no-icu", "--not-tag exclude_noicu", "")

# OSX build tasks:
elif os == "OSX":
    osString = 'OSX.1011.Amd64.Chakra.Open'
    CreateXPlatBuildTasks(osString, "osx", "osx", branch, None, "")

else:
    print("incorrect OS string value: " + os)

'''
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
    def icuFlag = (platform == "osx" ? "--icu=/Users/DDITLABS/homebrew/opt/icu4c/include" : "")
    def compilerPaths = (platform == "osx") ? "" : "--cxx=/usr/bin/clang++-3.9 --cc=/usr/bin/clang-3.9"
    def buildScript = "bash ./build.sh ${staticFlag} -j=`${numConcurrentCommand}` ${buildFlag} " +
                      "${swbCheckFlag} ${compilerPaths} ${icuFlag} ${customOption} ${extraBuildParams}"
    def icuLibFlag = (platform == "osx" ? "--iculib=/Users/DDITLABS/homebrew/opt/icu4c" : "")
    def testScript = "bash test/runtests.sh ${icuLibFlag} \"${testVariant}\""

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

    if (platform == "osx") {
        Utilities.setMachineAffinity(newJob, machine) // OSX machine string contains all info already
    } else {
        Utilities.setMachineAffinity(newJob, machine, defaultMachineTag)
    }
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
'''
