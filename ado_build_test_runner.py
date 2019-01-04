import subprocess
import sys
from subprocess import check_output
 
os = "OSX"
branch = "master"
debug = False
test = False
release = False
arm = False
x64 = False
x86 = False
no_jit = False
no_icu = False
run_not_static = False
run_static = False
slow = False
lite = False
legacy = False
runInfoScript = False
createBinary = False
runTestsAgainstBinary = False

latestWindowsMachine = 'windows.10.amd64.clientrs4.devex.open' # Windows 10 RS4 with Dev 15.7
latestWindowsMachineTag = None # all information is included in the machine name above

for arg in sys.argv:
    arg = arg.replace(" ","")
    argArr = arg.split(":")

    # "OSX", "Windows", or "Linux". Default: "OSX"
    if argArr[0] == "--os":
        os = argArr[1]

    # Default: "Master"
    elif argArr[0] == "--branch":
        branch = argArr[1]

    elif argArr[0] == "--run-debug":
        debug = True
    elif argArr[0] == "--run-test":
        test = True
    elif argArr[0] == "--run-release":
        release = True
    elif argArr[0] == "--run-arm":
        arm = True
    elif argArr[0] == "--run-x64":
        x64 = True
    elif argArr[0] == "--run-x86":
        x86 = True
    elif argArr[0] == "--run-no-jit":
        no_jit = True
    elif argArr[0] == "--run-no-icu":
        no_icu = True
    elif argArr[0] == "--make-run-static":
        run_static = True
    elif argArr[0] == "--make-run-not-static":
        run_not_static = True
    elif argArr[0] == "--slow":
        slow = True
    elif argArr[0] == "--lite":
        lite = True
    elif argArr[0] == "--legacy":
        legacy = True
    elif argArr[0] == "--runInfoScript":
        runInfoScript = True
    elif argArr[0] == "--createBinary":
        createBinary = True
    elif argArr[0] == "--runTestsAgainstBinary":
        runTestsAgainstBinary = True

if not run_static and not run_not_static:
    run_not_static = True

if not runInfoScript and not createBinary and not runTestsAgainstBinary:
    runInfoScript = True
    createBinary = True
    runTestsAgainstBinary = True

def CreateBuildTasks(machine, machineTag, configTag, buildExtra, testExtra, runCodeAnalysis, excludeConfigIf, nonDefaultTaskSetup):

    #isPRArr = {True, False}
    # temp replacement. Change when isPR is implemented in CreateXPlatBuildTask
    isPRArr = {False}

    for isPR in isPRArr:
        buildTypeArr = []
        if debug:
            buildTypeArr.append("debug")
        if test:
            buildTypeArr.append("test")
        if release:
            buildTypeArr.append("release")
        buildArchArr = []
        if arm:
            buildArchArr.append("arm")
        if x86:
            buildArchArr.append("x86")
        if x64:
            buildArchArr.append("x64")

        for buildType in buildTypeArr:
            for buildArch in buildArchArr:
                CreateBuildTask(isPR, buildArch, buildType, machine, machineTag, configTag, buildExtra, testExtra, runCodeAnalysis, excludeConfigIf, nonDefaultTaskSetup)

def CreateBuildTask(isPR, buildArch, buildType, machine, machineTag, configTag, buildExtra, testExtra, runCodeAnalysis, excludeConfigIf, nonDefaultTaskSetup):

    config = buildArch + "_" + buildType
    
    config = config if configTag == None else configTag + "_" + config

    testableConfig = buildType in ['debug', 'test'] and buildArch != 'arm'
    analysisConfig = buildType in ['release'] and runCodeAnalysis

    buildScript = "call .\\jenkins\\buildone.cmd "+ buildArch + " " + buildType
    buildScript += ' \"/p:runcodeanalysis=true\"' if analysisConfig else ''
    
    testScript = "call .\\jenkins\\testone.cmd " + buildArch + " " + buildType + " "

    analysisScript = '.\\Build\\scripts\\check_prefast_error.ps1 . CodeAnalysis.err'

    exeShellStr(buildScript)
    if testableConfig:
        exeShellStr(testScript)
    # TODO:
    # if analysisConfig:
    #     powerShellInPython()

def CreateXPlatBuildTasks(machine, platform, configTag, xplatBranch, nonDefaultTaskSetup, extraBuildParams):
    
    #isPRArr = {True, False}
    # temp replacement. Change when isPR is implemented in CreateXPlatBuildTask
    isPRArr = {False}

    for isPR in isPRArr:
        if no_jit:
            CreateXPlatBuildTask(isPR, "test", "", machine, platform, configTag, xplatBranch, nonDefaultTaskSetup, "--no-jit", "--variants disable_jit", extraBuildParams)

        buildTypeArr = []
        if debug:
            buildTypeArr.append("debug")
        if test:
            buildTypeArr.append("test")
        if release:
            buildTypeArr.append("release")

        for buildType in buildTypeArr:

            staticBuildConfigs = []
            if run_static:
                staticBuildConfigs.append(True)
            if run_not_static:
                staticBuildConfigs.append(False)

            if platform == "osx":
                staticBuildConfigs = {True}

            for staticBuild in staticBuildConfigs:
                CreateXPlatBuildTask(isPR, buildType, staticBuild, machine, platform, configTag, xplatBranch, nonDefaultTaskSetup, "", "", extraBuildParams)


def CreateXPlatBuildTask(isPR, buildType, staticBuild, machine, platform, configTag, xplatBranch, nonDefaultTaskSetup, customOption, testVariant, extraBuildParams):
    config = "osx_"+buildType if platform == "osx" else "linux_"+buildType
    
    # todo: numConcurrentCommand = "sysctl -n hw.logicalcpu" if platform is "osx" else "nproc"
    # temp replacement:
    # numConcurrentCommand = "nproc"
    numConcurrentCommand = "sysctl -n hw.logicalcpu" if platform == "osx" else "nproc"

    config = config if configTag is None else configTag + "_" + config
    config = "static_"+config if staticBuild else "shared"+config
    # todo: config = customOption.replaceAll(/[-]+/, "_") + "_" + config if customOption else config

    '''todo: jobName = Utilities.getFullJobName(project, config, isPR)'''

    infoScript = "bash jenkins/get_system_info.sh --"+platform

    buildFlag = "" if buildType == "release" else ("--debug" if buildType == "debug" else "--test-build")

    staticFlag = "--static" if staticBuild else ""

    swbCheckFlag = "--wb-check" if platform == "linux" and buildType == "debug" and not staticBuild else ""

    # todo: icuFlag = "--icu=/Users/DDITLABS/homebrew/opt/icu4c/include" if platform == "osx" else ""
    # temp replacement:
    icuFlag = "--embed-icu" if platform == "osx" else ""

    # todo: compilerPaths = "" if platform is "osx" else "--cxx=/usr/bin/clang++-3.9 --cc=/usr/bin/clang-3.9"
    # temp replacement:
    compilerPaths = ""

    buildScript = "bash ./build.sh "+staticFlag+" "+buildFlag+" "+swbCheckFlag+" "+compilerPaths+" "+icuFlag+" "+customOption+" "+extraBuildParams

    if platform == "linux":
        buildScript = "sudo " + buildScript

    # todo: icuLibFlag = "--iculib=/Users/DDITLABS/homebrew/opt/icu4c" if platform == "osx" else ""
    # temp replacement:
    icuLibFlag = ""

    testScript = "bash test/runtests.sh "+icuLibFlag

    j = getJ(numConcurrentCommand)

    if runInfoScript:
        printToADO("----- INFO SCRIPT -----")
        exeBashStr(infoScript)

    if createBinary:
        printToADO("----- BUILD SCRIPT -----")
        if j:
            exeBashStr(buildScript, j)
        else:
            exeBashStr(buildScript)
    
    if runTestsAgainstBinary:
        printToADO("----- TEST SCRIPT -----")
        exeBashStr(testScript, None, testVariant)

# Unix
def exeBashStr(bashStr, j=None, testVariant=None, windows=False):
    bashStrList = bashStr.split(" ")
    bashStrList = filter(lambda a: a != "", bashStrList)
    if j:
        bashStrList.append("-j="+j)
    if testVariant:
        bashStrList.append("\""+testVariant+"\"")
    printToADO(bashStrList, windows)

    ret = None
    try:
        if windows:
            ret = subprocess.call(bashStrList, shell=True)
        else:
            ret = subprocess.call(bashStrList)
    except:
        print("Invalid Bash String: "+bashStr)
        return None

    if ret != 0:
        print("exit code: "+str(ret))
        sys.exit(ret)

    return ret

# Windows
def exeShellStr(shellStr):
    printToADO(shellStr, True)
    exeBashStr(shellStr, None, None, True)

def printToADO(v, windows = False):
    if windows:
        subprocess.call(["echo", str(v)], shell=True)
    else:
        subprocess.call(["echo", str(v)])

def getJ(jArg):
    j = exeBashStr(jArg)
    if not j or j <= 0:
        printToADO("j not found; value of j: "+str(j))
        return None
    j = str(j)
    printToADO("j found: "+j)
    return j

# Linux build tasks:
if os == "Linux":
    osString = 'Ubuntu16.04'

    # not sure if --lto-thin is needed, using temporarily because cmake cant find ll vm
    CreateXPlatBuildTasks(osString, "linux", "ubuntu", branch, None, "--lto-thin")

    if no_icu:
        #isPRArr = {True, False}
        # temp replacement. Change when isPR is implemented in CreateXPlatBuildTask
        isPRArr = {False}
        for isPR in isPRArr:
            CreateXPlatBuildTask(isPR, "debug", True, osString, "linux", "ubuntu", branch, None, "--no-icu", "--not-tag exclude_noicu", "--lto-thin")

# OSX build tasks:
elif os == "OSX":
    osString = 'OSX.1011.Amd64.Chakra.Open'
    CreateXPlatBuildTasks(osString, "osx", "osx", branch, None, "")

#                CreateXPlatBuildTask(isPR, buildType, staticBuild, machine, platform, configTag, xplatBranch, nonDefaultTaskSetup, "", "", extraBuildParams)


elif os == "Windows":

    if slow:
        CreateBuildTask(True, 'x64', 'debug', latestWindowsMachine, latestWindowsMachineTag, 'ci_slow', None, '-win10 -includeSlow', False, None, None)
    elif no_jit:
        CreateBuildTask(True, 'x64', 'debug', latestWindowsMachine, latestWindowsMachineTag, 'ci_disablejit', '"/p:BuildJIT=false"', '-win10 -disablejit', False, None, None)
    elif lite:
        CreateBuildTask(True, 'x64', 'debug', latestWindowsMachine, latestWindowsMachineTag, 'ci_lite', '"/p:BuildLite=true"', '-win10 -lite', False, None, None)
    else:
        CreateBuildTasks(latestWindowsMachine, latestWindowsMachineTag, None, None, "-win10", True, None, None)

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
