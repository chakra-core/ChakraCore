//-------------------------------------------------------------------------------------------------------
// ChakraCore/Pal
// Contains portions (c) copyright Microsoft, portions copyright (c) the .NET Foundation and Contributors
// and edits (c) copyright the ChakraCore Contributors.
// See THIRD-PARTY-NOTICES.txt in the project root for .NET Foundation license
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

/*++

Module Name:

    process.cpp

Abstract:

    Implementation of process object and functions related to processes.

--*/

#include "pal/dbgmsg.h"
SET_DEFAULT_DEBUG_CHANNEL(PROCESS); // some headers have code with asserts, so do this first

#include "pal/procobj.hpp"
#include "pal/thread.hpp"
#include "pal/file.hpp"
#include "pal/handlemgr.hpp"
#include "pal/module.h"
#include "procprivate.hpp"
#include "pal/palinternal.h"
#include "pal/process.h"
#include "pal/init.h"
#include "pal/critsect.h"
#include "pal/debug.h"
#include "pal/utils.h"
#include "pal/misc.h"
#include "pal/virtual.h"
#include "pal/stackstring.hpp"

#include <errno.h>
#if HAVE_POLL
#include <poll.h>
#else
#include "pal/fakepoll.h"
#endif  // HAVE_POLL

#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#if HAVE_PRCTL_H
#include <sys/prctl.h>
#include <sys/syscall.h>
#endif
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <debugmacrosext.h>
#include <semaphore.h>
#include <stdint.h>
#include <dlfcn.h>
#include <limits.h>

#ifdef __linux__
#include <sys/syscall.h> // __NR_membarrier
// Ensure __NR_membarrier is defined for portable builds.
# if !defined(__NR_membarrier)
#  if defined(__amd64__)
#   define __NR_membarrier  324
#  elif defined(__i386__)
#   define __NR_membarrier  375
#  elif defined(__arm__)
#   define __NR_membarrier  389
#  elif defined(__aarch64__)
#   define __NR_membarrier  283
#  elif defined(__loongarch64)
#   define __NR_membarrier  283
#  else
#   error Unknown architecture
#  endif
# endif
#endif

#ifdef __APPLE__
#include <libproc.h>
#include <sys/sysctl.h>
#include <sys/posix_sem.h>
#include <mach/task.h>
#include <mach/vm_map.h>
extern "C"
{
#  include <mach/thread_state.h>
}

#define CHECK_MACH(_msg, machret) do {                                      \
        if (machret != KERN_SUCCESS)                                        \
        {                                                                   \
            char _szError[1024];                                            \
            mach_error(_szError, machret);                                  \
            abort();                                                        \
        }                                                                   \
    } while (false)

#endif // __APPLE__

#ifdef __NetBSD__
#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/sysctl.h>
#include <kvm.h>
#endif

#ifdef __FreeBSD__
#include <sys/sysctl.h>
#include <sys/user.h>
#endif

extern char *g_szCoreCLRPath;
extern bool g_running_in_exe;

using namespace CorUnix;

CObjectType CorUnix::otProcess PAL_GLOBAL (
                otiProcess,
                NULL,   // No cleanup routine
                NULL,   // No initialization routine
                0,      // No immutable data
                sizeof(CProcProcessLocalData),
                0,
                PROCESS_ALL_ACCESS,
                CObjectType::SecuritySupported,
                CObjectType::SecurityInfoNotPersisted,
                CObjectType::UnnamedObject,
                CObjectType::CrossProcessDuplicationAllowed,
                CObjectType::WaitableObject,
                CObjectType::SingleTransitionObject,
                CObjectType::ThreadReleaseHasNoSideEffects,
                CObjectType::NoOwner
                );

//
// Helper membarrier function
//
#ifdef __NR_membarrier
# define membarrier(...)  syscall(__NR_membarrier, __VA_ARGS__)
#else
# define membarrier(...)  -ENOSYS
#endif

enum membarrier_cmd
{
    MEMBARRIER_CMD_QUERY                                 = 0,
    MEMBARRIER_CMD_GLOBAL                                = (1 << 0),
    MEMBARRIER_CMD_GLOBAL_EXPEDITED                      = (1 << 1),
    MEMBARRIER_CMD_REGISTER_GLOBAL_EXPEDITED             = (1 << 2),
    MEMBARRIER_CMD_PRIVATE_EXPEDITED                     = (1 << 3),
    MEMBARRIER_CMD_REGISTER_PRIVATE_EXPEDITED            = (1 << 4),
    MEMBARRIER_CMD_PRIVATE_EXPEDITED_SYNC_CORE           = (1 << 5),
    MEMBARRIER_CMD_REGISTER_PRIVATE_EXPEDITED_SYNC_CORE  = (1 << 6)
};

CAllowedObjectTypes aotProcess PAL_GLOBAL (otiProcess);

//
// The representative IPalObject for this process
//
IPalObject* CorUnix::g_pobjProcess;

//
// Critical section that protects process data (e.g., the
// list of active threads)/
//
CRITICAL_SECTION g_csProcess PAL_GLOBAL;

//
// List and count of active threads
//
CPalThread* CorUnix::pGThreadList;
DWORD g_dwThreadCount;

//
// The command line and app name for the process
//
LPWSTR g_lpwstrCmdLine = NULL;
LPWSTR g_lpwstrAppDir = NULL;

// Thread ID of thread that has started the ExitProcess process
Volatile<LONG> terminator PAL_GLOBAL = 0;

// Process and session ID of this process.
DWORD gPID = (DWORD) -1;
DWORD gSID = (DWORD) -1;

// Application group ID for this process
#ifdef __APPLE__
LPCSTR gApplicationGroupId = nullptr;
int gApplicationGroupIdLength = 0;
#endif // __APPLE__
PathCharString* gSharedFilesPath = nullptr;

// The lowest common supported semaphore length, including null character
// NetBSD-7.99.25: 15 characters
// MacOSX 10.11: 31 -- Core 1.0 RC2 compatibility
#if defined(__NetBSD__)
#define CLR_SEM_MAX_NAMELEN 15
#elif defined(__APPLE__)
#define CLR_SEM_MAX_NAMELEN PSEMNAMLEN
#elif defined(NAME_MAX)
#define CLR_SEM_MAX_NAMELEN (NAME_MAX - 4)
#else
// On Solaris, MAXNAMLEN is 512, which is higher than MAX_PATH defined by pal.h
#define CLR_SEM_MAX_NAMELEN MAX_PATH
#endif

static_assert(CLR_SEM_MAX_NAMELEN <= MAX_PATH, "CLR_SEM_MAX_NAMELEN > MAX_PATH");

//
// Key used for associating CPalThread's with the underlying pthread
// (through pthread_setspecific)
//
pthread_key_t CorUnix::thObjKey;

static WCHAR W16_WHITESPACE[]= {0x0020, 0x0009, 0x000D, 0};
static WCHAR W16_WHITESPACE_DQUOTE[]= {0x0020, 0x0009, 0x000D, '"', 0};

enum FILETYPE
{
    FILE_ERROR,/*ERROR*/
    FILE_UNIX, /*Unix Executable*/
    FILE_DIR   /*Directory*/
};

#pragma pack(push,1)
// When creating the semaphore name on Mac running in a sandbox, We reference this structure as a byte array
// in order to encode its data into a string. Its important to make sure there is no padding between the fields
// and also at the end of the buffer. Hence, this structure is defined inside a pack(1)
struct UnambiguousProcessDescriptor
{
    UnambiguousProcessDescriptor()
    {
    }

    UnambiguousProcessDescriptor(DWORD processId, UINT64 disambiguationKey)
    {
        Init(processId, disambiguationKey);
    }

    void Init(DWORD processId, UINT64 disambiguationKey)
    {
        m_processId = processId;
        m_disambiguationKey = disambiguationKey;
    }
    UINT64 m_disambiguationKey;
    DWORD m_processId;
};
#pragma pack(pop)

static
DWORD
StartupHelperThread(
    LPVOID p);

PAL_ERROR
PROCGetProcessStatus(
    CPalThread *pThread,
    HANDLE hProcess,
    PROCESS_STATE *pps,
    DWORD *pdwExitCode);

static BOOL getFileName(LPCWSTR lpApplicationName, LPWSTR lpCommandLine, char *lpPathFileName);
static char ** buildArgv(LPCWSTR lpCommandLine, LPSTR lpAppPath, UINT *pnArg);
static BOOL getPath(LPCSTR lpFileName, UINT iLen, LPSTR  lpPathFileName);
static int checkFileType(LPCSTR lpFileName);
static BOOL PROCEndProcess(HANDLE hProcess, UINT uExitCode, BOOL bTerminateUnconditionally);

/*++
Function:
  GetCurrentProcessId

See MSDN doc.
--*/
DWORD
PALAPI
GetCurrentProcessId(
            VOID)
{
    PERF_ENTRY(GetCurrentProcessId);
    ENTRY("GetCurrentProcessId()\n" );

    LOGEXIT("GetCurrentProcessId returns DWORD %#x\n", gPID);
    PERF_EXIT(GetCurrentProcessId);
    return gPID;
}


/*++
Function:
  GetCurrentSessionId

See MSDN doc.
--*/
DWORD
PALAPI
GetCurrentSessionId(
            VOID)
{
    PERF_ENTRY(GetCurrentSessionId);
    ENTRY("GetCurrentSessionId()\n" );

    LOGEXIT("GetCurrentSessionId returns DWORD %#x\n", gSID);
    PERF_EXIT(GetCurrentSessionId);
    return gSID;
}


/*++
Function:
  GetCurrentProcess

See MSDN doc.
--*/
HANDLE
PALAPI
GetCurrentProcess(
          VOID)
{
    PERF_ENTRY(GetCurrentProcess);
    ENTRY("GetCurrentProcess()\n" );

    LOGEXIT("GetCurrentProcess returns HANDLE %p\n", hPseudoCurrentProcess);
    PERF_EXIT(GetCurrentProcess);

    /* return a pseudo handle */
    return hPseudoCurrentProcess;
}

/*++
Function:
  CreateProcessW

Note:
  Only Standard handles need to be inherited.
  Security attributes parameters are not used.

See MSDN doc.
--*/
BOOL
PALAPI
CreateProcessW(
           IN LPCWSTR lpApplicationName,
           IN LPWSTR lpCommandLine,
           IN LPSECURITY_ATTRIBUTES lpProcessAttributes,
           IN LPSECURITY_ATTRIBUTES lpThreadAttributes,
           IN BOOL bInheritHandles,
           IN DWORD dwCreationFlags,
           IN LPVOID lpEnvironment,
           IN LPCWSTR lpCurrentDirectory,
           IN LPSTARTUPINFOW lpStartupInfo,
           OUT LPPROCESS_INFORMATION lpProcessInformation)
{
    PAL_ERROR palError = NO_ERROR;
    CPalThread *pThread;

    PERF_ENTRY(CreateProcessW);
    ENTRY("CreateProcessW(lpAppName=%p (%S), lpCmdLine=%p (%S), lpProcessAttr=%p,"
           "lpThreadAttr=%p, bInherit=%d, dwFlags=%#x, lpEnv=%p,"
           "lpCurrentDir=%p (%S), lpStartupInfo=%p, lpProcessInfo=%p)\n",
           lpApplicationName?lpApplicationName:W16_NULLSTRING,
           lpApplicationName?lpApplicationName:W16_NULLSTRING,
           lpCommandLine?lpCommandLine:W16_NULLSTRING,
           lpCommandLine?lpCommandLine:W16_NULLSTRING,lpProcessAttributes,
           lpThreadAttributes, bInheritHandles, dwCreationFlags,lpEnvironment,
           lpCurrentDirectory?lpCurrentDirectory:W16_NULLSTRING,
           lpCurrentDirectory?lpCurrentDirectory:W16_NULLSTRING,
           lpStartupInfo, lpProcessInformation);

    pThread = InternalGetCurrentThread();

    palError = InternalCreateProcess(
        pThread,
        lpApplicationName,
        lpCommandLine,
        lpProcessAttributes,
        lpThreadAttributes,
        bInheritHandles,
        dwCreationFlags,
        lpEnvironment,
        lpCurrentDirectory,
        lpStartupInfo,
        lpProcessInformation
        );

    if (NO_ERROR != palError)
    {
        pThread->SetLastError(palError);
    }

    LOGEXIT("CreateProcessW returns BOOL %d\n", NO_ERROR == palError);
    PERF_EXIT(CreateProcessW);

    return NO_ERROR == palError;
}

PAL_ERROR
PrepareStandardHandle(
    CPalThread *pThread,
    HANDLE hFile,
    IPalObject **ppobjFile,
    int *piFd
    )
{
    PAL_ERROR palError = NO_ERROR;
    IPalObject *pobjFile = NULL;
    IDataLock *pDataLock = NULL;
    CFileProcessLocalData *pLocalData = NULL;
    int iError = 0;

    palError = g_pObjectManager->ReferenceObjectByHandle(
        pThread,
        hFile,
        &aotFile,
        0,
        &pobjFile
        );

    if (NO_ERROR != palError)
    {
        ERROR("Bad handle passed through CreateProcess\n");
        goto PrepareStandardHandleExit;
    }

    palError = pobjFile->GetProcessLocalData(
        pThread,
        ReadLock,
        &pDataLock,
        reinterpret_cast<void **>(&pLocalData)
        );

    if (NO_ERROR != palError)
    {
        ASSERT("Unable to access file data\n");
        goto PrepareStandardHandleExit;
    }

    //
    // The passed in file needs to be inheritable
    //

    if (!pLocalData->inheritable)
    {
        ERROR("Non-inheritable handle passed through CreateProcess\n");
        palError = ERROR_INVALID_HANDLE;
        goto PrepareStandardHandleExit;
    }

    iError = fcntl(pLocalData->unix_fd, F_SETFD, 0);
    if (-1 == iError)
    {
        ERROR("Unable to remove close-on-exec for file (errno %i)\n", errno);
        palError = ERROR_INVALID_HANDLE;
        goto PrepareStandardHandleExit;
    }

    *piFd = pLocalData->unix_fd;
    pDataLock->ReleaseLock(pThread, FALSE);
    pDataLock = NULL;

    //
    // Transfer pobjFile reference to out parameter
    //

    *ppobjFile = pobjFile;
    pobjFile = NULL;

PrepareStandardHandleExit:

    if (NULL != pDataLock)
    {
        pDataLock->ReleaseLock(pThread, FALSE);
    }

    if (NULL != pobjFile)
    {
        pobjFile->ReleaseReference(pThread);
    }

    return palError;
}

PAL_ERROR
CorUnix::InternalCreateProcess(
    CPalThread *pThread,
    LPCWSTR lpApplicationName,
    LPWSTR lpCommandLine,
    LPSECURITY_ATTRIBUTES lpProcessAttributes,
    LPSECURITY_ATTRIBUTES lpThreadAttributes,
    BOOL bInheritHandles,
    DWORD dwCreationFlags,
    LPVOID lpEnvironment,
    LPCWSTR lpCurrentDirectory,
    LPSTARTUPINFOW lpStartupInfo,
    LPPROCESS_INFORMATION lpProcessInformation
    )
{
    PAL_ERROR palError = NO_ERROR;
    IPalObject *pobjProcess = NULL;
    IPalObject *pobjProcessRegistered = NULL;
    IDataLock *pLocalDataLock = NULL;
    CProcProcessLocalData *pLocalData;
    IDataLock *pSharedDataLock = NULL;
    CPalThread *pDummyThread = NULL;
    HANDLE hDummyThread = NULL;
    HANDLE hProcess = NULL;
    CObjectAttributes oa(NULL, lpProcessAttributes);

    IPalObject *pobjFileIn = NULL;
    int iFdIn = -1;
    IPalObject *pobjFileOut = NULL;
    int iFdOut = -1;
    IPalObject *pobjFileErr = NULL;
    int iFdErr = -1;

    pid_t processId;
    char * lpFileName;
    PathCharString lpFileNamePS;
    char **lppArgv = NULL;
    UINT nArg;
    int  iRet;
    char **EnvironmentArray=NULL;
    int child_blocking_pipe = -1;
    int parent_blocking_pipe = -1;

    /* Validate parameters */

    /* note : specs indicate lpApplicationName should always
       be NULL; however support for it is already implemented. Leaving the code
       in, specs can change; but rejecting non-NULL for now to conform to the
       spec. */
    if( NULL != lpApplicationName )
    {
        ASSERT("lpApplicationName should be NULL, but is %S instead\n",
               lpApplicationName);
        palError = ERROR_INVALID_PARAMETER;
        goto InternalCreateProcessExit;
    }

    if (0 != (dwCreationFlags & ~(CREATE_SUSPENDED|CREATE_NEW_CONSOLE)))
    {
        ASSERT("Unexpected creation flags (%#x)\n", dwCreationFlags);
        palError = ERROR_INVALID_PARAMETER;
        goto InternalCreateProcessExit;
    }

    /* Security attributes parameters are ignored */
    if (lpProcessAttributes != NULL &&
        (lpProcessAttributes->lpSecurityDescriptor != NULL ||
         lpProcessAttributes->bInheritHandle != TRUE))
    {
        ASSERT("lpProcessAttributes is invalid, parameter ignored (%p)\n",
               lpProcessAttributes);
        palError = ERROR_INVALID_PARAMETER;
        goto InternalCreateProcessExit;
    }

    if (lpThreadAttributes != NULL)
    {
        ASSERT("lpThreadAttributes parameter must be NULL (%p)\n",
               lpThreadAttributes);
        palError = ERROR_INVALID_PARAMETER;
        goto InternalCreateProcessExit;
    }

    /* note : Win32 crashes in this case */
    if(NULL == lpStartupInfo)
    {
        ERROR("lpStartupInfo is NULL\n");
        palError = ERROR_INVALID_PARAMETER;
        goto InternalCreateProcessExit;
    }

    /* Validate lpStartupInfo.cb field */
    if (lpStartupInfo->cb < sizeof(STARTUPINFOW))
    {
        ASSERT("lpStartupInfo parameter structure size is invalid (%u)\n",
              lpStartupInfo->cb);
        palError = ERROR_INVALID_PARAMETER;
        goto InternalCreateProcessExit;
    }

    /* lpStartupInfo should be either zero or STARTF_USESTDHANDLES */
    if (lpStartupInfo->dwFlags & ~STARTF_USESTDHANDLES)
    {
        ASSERT("lpStartupInfo parameter invalid flags (%#x)\n",
              lpStartupInfo->dwFlags);
        palError = ERROR_INVALID_PARAMETER;
        goto InternalCreateProcessExit;
    }

    /* validate given standard handles if we have any */
    if (lpStartupInfo->dwFlags & STARTF_USESTDHANDLES)
    {
        palError = PrepareStandardHandle(
            pThread,
            lpStartupInfo->hStdInput,
            &pobjFileIn,
            &iFdIn
            );

        if (NO_ERROR != palError)
        {
            goto InternalCreateProcessExit;
        }

        palError = PrepareStandardHandle(
            pThread,
            lpStartupInfo->hStdOutput,
            &pobjFileOut,
            &iFdOut
            );

        if (NO_ERROR != palError)
        {
            goto InternalCreateProcessExit;
        }

        palError = PrepareStandardHandle(
            pThread,
            lpStartupInfo->hStdError,
            &pobjFileErr,
            &iFdErr
            );

        if (NO_ERROR != palError)
        {
            goto InternalCreateProcessExit;
        }
    }

    lpFileName = lpFileNamePS.OpenStringBuffer(MAX_LONGPATH-1);
    if (NULL == lpFileName)
    {
        palError = ERROR_NOT_ENOUGH_MEMORY;
        goto InternalCreateProcessExit;
    }
    if (!getFileName(lpApplicationName, lpCommandLine, lpFileName))
    {
        ERROR("Can't find executable!\n");
        palError = ERROR_FILE_NOT_FOUND;
        goto InternalCreateProcessExit;
    }

    lpFileNamePS.CloseBuffer(MAX_LONGPATH-1);
    /* check type of file */
    iRet = checkFileType(lpFileName);

    switch (iRet)
    {
        case FILE_ERROR: /* file not found, or not an executable */
            WARN ("File is not valid (%s)", lpFileName);
            palError = ERROR_FILE_NOT_FOUND;
            goto InternalCreateProcessExit;

        case FILE_UNIX: /* Unix binary file */
            break;  /* nothing to do */

        case FILE_DIR:/*Directory*/
            WARN ("File is a Directory (%s)", lpFileName);
            palError = ERROR_ACCESS_DENIED;
            goto InternalCreateProcessExit;
            break;

        default: /* not supposed to get here */
            ASSERT ("Invalid return type from checkFileType");
            palError = ERROR_FILE_NOT_FOUND;
            goto InternalCreateProcessExit;
    }

    /* build Argument list, lppArgv is allocated in buildArgv function and
       requires to be freed */
    lppArgv = buildArgv(lpCommandLine, lpFileName, &nArg);

    /* set the Environment variable */
    if (lpEnvironment != NULL)
    {
        unsigned i;
        // Since CREATE_UNICODE_ENVIRONMENT isn't supported we know the string is ansi
        unsigned EnvironmentEntries = 0;
        // Convert the environment block to array of strings
        // Count the number of entries
        // Is it a string that contains null terminated string, the end is delimited
        // by two null in a row.
        for (i = 0; ((char *)lpEnvironment)[i]!='\0'; i++)
        {
            EnvironmentEntries ++;
            for (;((char *)lpEnvironment)[i]!='\0'; i++)
            {
            }
        }
        EnvironmentEntries++;
        EnvironmentArray = (char **)InternalMalloc(EnvironmentEntries * sizeof(char *));

        EnvironmentEntries = 0;
        // Convert the environment block to array of strings
        // Count the number of entries
        // Is it a string that contains null terminated string, the end is delimited
        // by two null in a row.
        for (i = 0; ((char *)lpEnvironment)[i]!='\0'; i++)
        {
            EnvironmentArray[EnvironmentEntries] = &((char *)lpEnvironment)[i];
            EnvironmentEntries ++;
            for (;((char *)lpEnvironment)[i]!='\0'; i++)
            {
            }
        }
        EnvironmentArray[EnvironmentEntries] = NULL;
    }

    //
    // Allocate and register the process object for the new process
    //

    palError = g_pObjectManager->AllocateObject(
        pThread,
        &otProcess,
        &oa,
        &pobjProcess
        );

    if (NO_ERROR != palError)
    {
        ERROR("Unable to allocate object for new process\n");
        goto InternalCreateProcessExit;
    }

    palError = g_pObjectManager->RegisterObject(
        pThread,
        pobjProcess,
        &aotProcess,
        PROCESS_ALL_ACCESS,
        &hProcess,
        &pobjProcessRegistered
        );

    //
    // pobjProcess is invalidated by the above call, so
    // NULL it out here
    //

    pobjProcess = NULL;

    if (NO_ERROR != palError)
    {
        ERROR("Unable to register new process object\n");
        goto InternalCreateProcessExit;
    }

    //
    // Create a new "dummy" thread object
    //

    palError = InternalCreateDummyThread(
        pThread,
        lpThreadAttributes,
        &pDummyThread,
        &hDummyThread
        );

    if (dwCreationFlags & CREATE_SUSPENDED)
    {
        int pipe_descs[2];

        if (-1 == pipe(pipe_descs))
        {
            ERROR("pipe() failed! error is %d (%s)\n", errno, strerror(errno));
            palError = ERROR_NOT_ENOUGH_MEMORY;
            goto InternalCreateProcessExit;
        }

        /* [0] is read end, [1] is write end */
        pDummyThread->suspensionInfo.SetBlockingPipe(pipe_descs[1]);
        parent_blocking_pipe = pipe_descs[1];
        child_blocking_pipe = pipe_descs[0];
    }

    palError = pobjProcessRegistered->GetProcessLocalData(
        pThread,
        WriteLock,
        &pLocalDataLock,
        reinterpret_cast<void **>(&pLocalData)
        );

    if (NO_ERROR != palError)
    {
        ASSERT("Unable to obtain local data for new process object\n");
        goto InternalCreateProcessExit;
    }


    /* fork the new process */
    processId = fork();

    if (processId == -1)
    {
        ASSERT("Unable to create a new process with fork()\n");
        if (-1 != child_blocking_pipe)
        {
            close(child_blocking_pipe);
            close(parent_blocking_pipe);
        }

        palError = ERROR_INTERNAL_ERROR;
        goto InternalCreateProcessExit;
    }

    /* From the time the child process begins running, to when it reaches execve,
    the child process is not a real PAL process and does not own any PAL
    resources, although it has access to the PAL resources of its parent process.
    Thus, while the child process is in this window, it is dangerous for it to affect
    its parent's PAL resources. As a consequence, no PAL code should be used
    in this window; all code should make unix calls. Note the use of _exit
    instead of exit to avoid calling PAL_Terminate and the lack of TRACE's and
    ASSERT's. */

    if (processId == 0)  /* child process */
    {
        // At this point, the PAL should be considered uninitialized for this child process.

        // Don't want to enter the init_critsec here since we're trying to avoid
        // calling PAL functions. Furthermore, nothing should be changing
        // the init_count in the child process at this point since this is the only
        // thread executing.
        init_count = 0;

        sigset_t sm;

        //
        // Clear out the signal mask for the new process.
        //

        sigemptyset(&sm);
        iRet = sigprocmask(SIG_SETMASK, &sm, NULL);
        if (iRet != 0)
        {
            _exit(EXIT_FAILURE);
        }

        if (dwCreationFlags & CREATE_SUSPENDED)
        {
            BYTE resume_code = 0;
            ssize_t read_ret;

            /* close the write end of the pipe, the child doesn't need it */
            close(parent_blocking_pipe);

            read_again:
            /* block until ResumeThread writes something to the pipe */
            read_ret = read(child_blocking_pipe, &resume_code, sizeof(resume_code));
            if (sizeof(resume_code) != read_ret)
            {
                if (read_ret == -1 && EINTR == errno)
                {
                    goto read_again;
                }
                else
                {
                    /* note : read might return 0 (and return EAGAIN) if the other
                       end of the pipe gets closed - for example because the parent
                       process dies (very) abruptly */
                    _exit(EXIT_FAILURE);
                }
            }
            if (WAKEUPCODE != resume_code)
            {
                // resume_code should always equal WAKEUPCODE.
                _exit(EXIT_FAILURE);
            }

            close(child_blocking_pipe);
        }

        /* Set the current directory */
        if (lpCurrentDirectory)
        {
            SetCurrentDirectoryW(lpCurrentDirectory);
        }

        /* Set the standard handles to the incoming values */
        if (lpStartupInfo->dwFlags & STARTF_USESTDHANDLES)
        {
            /* For each handle, we need to duplicate the incoming unix
               fd to the corresponding standard one.  The API that I use,
               dup2, will copy the source to the destination, automatically
               closing the existing destination, in an atomic way */
            if (dup2(iFdIn, STDIN_FILENO) == -1)
            {
                // Didn't duplicate standard in.
                _exit(EXIT_FAILURE);
            }

            if (dup2(iFdOut, STDOUT_FILENO) == -1)
            {
                // Didn't duplicate standard out.
                _exit(EXIT_FAILURE);
            }

            if (dup2(iFdErr, STDERR_FILENO) == -1)
            {
                // Didn't duplicate standard error.
                _exit(EXIT_FAILURE);
            }

            /* now close the original FDs, we don't need them anymore */
            close(iFdIn);
            close(iFdOut);
            close(iFdErr);
        }

        /* execute the new process */

        if (EnvironmentArray)
        {
            execve(lpFileName, lppArgv, EnvironmentArray);
        }
        else
        {
            execve(lpFileName, lppArgv, palEnvironment);
        }

        /* if we get here, it means the execve function call failed so just exit */
        _exit(EXIT_FAILURE);
    }

    /* parent process */

    /* close the read end of the pipe, the parent doesn't need it */
    close(child_blocking_pipe);

    /* Set the process ID */
    pLocalData->dwProcessId = processId;
    pLocalDataLock->ReleaseLock(pThread, TRUE);
    pLocalDataLock = NULL;

    //
    // Release file handle info; we don't need them anymore. Note that
    // this must happen after we've released the data locks, as
    // otherwise a deadlock could result.
    //

    if (lpStartupInfo->dwFlags & STARTF_USESTDHANDLES)
    {
        pobjFileIn->ReleaseReference(pThread);
        pobjFileIn = NULL;
        pobjFileOut->ReleaseReference(pThread);
        pobjFileOut = NULL;
        pobjFileErr->ReleaseReference(pThread);
        pobjFileErr = NULL;
    }

    /* fill PROCESS_INFORMATION structure */
    lpProcessInformation->hProcess = hProcess;
    lpProcessInformation->hThread = hDummyThread;
    lpProcessInformation->dwProcessId = processId;
    lpProcessInformation->dwThreadId_PAL_Undefined = 0;


    TRACE("New process created: id=%#x\n", processId);

InternalCreateProcessExit:

    if (NULL != pLocalDataLock)
    {
        pLocalDataLock->ReleaseLock(pThread, FALSE);
    }

    if (NULL != pSharedDataLock)
    {
        pSharedDataLock->ReleaseLock(pThread, FALSE);
    }

    if (NULL != pobjProcess)
    {
        pobjProcess->ReleaseReference(pThread);
    }

    if (NULL != pobjProcessRegistered)
    {
        pobjProcessRegistered->ReleaseReference(pThread);
    }

    if (NO_ERROR != palError)
    {
        if (NULL != hProcess)
        {
            g_pObjectManager->RevokeHandle(pThread, hProcess);
        }

        if (NULL != hDummyThread)
        {
            g_pObjectManager->RevokeHandle(pThread, hDummyThread);
        }
    }

    if (EnvironmentArray)
    {
        free(EnvironmentArray);
    }

    /* if we still have the file structures at this point, it means we
       encountered an error sometime between when we acquired them and when we
       fork()ed. We not only have to release them, we have to give them back
       their close-on-exec flag */
    if (NULL != pobjFileIn)
    {
        if(-1 == fcntl(iFdIn, F_SETFD, 1))
        {
            WARN("couldn't restore close-on-exec flag to stdin descriptor! "
                 "errno is %d (%s)\n", errno, strerror(errno));
        }
        pobjFileIn->ReleaseReference(pThread);
    }

    if (NULL != pobjFileOut)
    {
        if(-1 == fcntl(iFdOut, F_SETFD, 1))
        {
            WARN("couldn't restore close-on-exec flag to stdout descriptor! "
                 "errno is %d (%s)\n", errno, strerror(errno));
        }
        pobjFileOut->ReleaseReference(pThread);
    }

    if (NULL != pobjFileErr)
    {
        if(-1 == fcntl(iFdErr, F_SETFD, 1))
        {
            WARN("couldn't restore close-on-exec flag to stderr descriptor! "
                 "errno is %d (%s)\n", errno, strerror(errno));
        }
        pobjFileErr->ReleaseReference(pThread);
    }

    /* free allocated memory */
    if (lppArgv)
    {
        free(*lppArgv);
        free(lppArgv);
    }

    return palError;
}


/*++
Function:
  GetExitCodeProcess

See MSDN doc.
--*/
BOOL
PALAPI
GetExitCodeProcess(
    IN HANDLE hProcess,
    IN LPDWORD lpExitCode)
{
    CPalThread *pThread;
    PAL_ERROR palError = NO_ERROR;
    DWORD dwExitCode;
    PROCESS_STATE ps;

    PERF_ENTRY(GetExitCodeProcess);
    ENTRY("GetExitCodeProcess(hProcess = %p, lpExitCode = %p)\n",
          hProcess, lpExitCode);

    pThread = InternalGetCurrentThread();

    if(NULL == lpExitCode)
    {
        WARN("Got NULL lpExitCode\n");
        palError = ERROR_INVALID_PARAMETER;
        goto done;
    }

    palError = PROCGetProcessStatus(
        pThread,
        hProcess,
        &ps,
        &dwExitCode
        );

    if (NO_ERROR != palError)
    {
        ASSERT("Couldn't get process status information!\n");
        goto done;
    }

    if( PS_DONE == ps )
    {
        *lpExitCode = dwExitCode;
    }
    else
    {
        *lpExitCode = STILL_ACTIVE;
    }

done:

    if (NO_ERROR != palError)
    {
        pThread->SetLastError(palError);
    }

    LOGEXIT("GetExitCodeProcess returns BOOL %d\n", NO_ERROR == palError);
    PERF_EXIT(GetExitCodeProcess);

    return NO_ERROR == palError;
}

/*++
Function:
  ExitProcess

See MSDN doc.
--*/
PAL_NORETURN
VOID
PALAPI
ExitProcess(
    IN UINT uExitCode)
{
    DWORD old_terminator;

    PERF_ENTRY_ONLY(ExitProcess);
    ENTRY("ExitProcess(uExitCode=0x%x)\n", uExitCode );

    old_terminator = InterlockedCompareExchange(&terminator, GetCurrentThreadId(), 0);

    if (GetCurrentThreadId() == old_terminator)
    {
        // This thread has already initiated termination. This can happen
        // in two ways:
        // 1) DllMain(DLL_PROCESS_DETACH) triggers a call to ExitProcess.
        // 2) PAL_exit() is called after the last PALTerminate().
        // If the PAL is still initialized, we go straight through to
        // PROCEndProcess. If it isn't, we simply exit.
        if (!PALIsInitialized())
        {
            exit(uExitCode);
            ASSERT("exit has returned\n");
        }
        else
        {
            WARN("thread re-called ExitProcess\n");
            PROCEndProcess(GetCurrentProcess(), uExitCode, FALSE);
        }
    }
    else if (0 != old_terminator)
    {
        /* another thread has already initiated the termination process. we
           could just block on the PALInitLock critical section, but then
           PROCSuspendOtherThreads would hang... so sleep forever here, we're
           terminating anyway

           Update: [TODO] PROCSuspendOtherThreads has been removed. Can this
           code be changed? */
        WARN("termination already started from another thread; blocking.\n");
        poll(NULL, 0, INFTIM);
    }

    /* ExitProcess may be called even if PAL is not initialized.
       Verify if process structure exist
    */
    if (PALInitLock() && PALIsInitialized())
    {
        PROCEndProcess(GetCurrentProcess(), uExitCode, FALSE);

        /* Should not get here, because we terminate the current process */
        ASSERT("PROCEndProcess has returned\n");
    }
    else
    {
        exit(uExitCode);

        /* Should not get here, because we terminate the current process */
        ASSERT("exit has returned\n");
    }

    /* this should never get executed */
    ASSERT("ExitProcess should not return!\n");
    while (true);
}

/*++
Function:
  TerminateProcess

Note:
  hProcess is a handle on the current process.

See MSDN doc.
--*/
BOOL
PALAPI
TerminateProcess(
    IN HANDLE hProcess,
    IN UINT uExitCode)
{
    BOOL ret;

    PERF_ENTRY(TerminateProcess);
    ENTRY("TerminateProcess(hProcess=%p, uExitCode=%u)\n",hProcess, uExitCode );

    ret = PROCEndProcess(hProcess, uExitCode, TRUE);

    LOGEXIT("TerminateProcess returns BOOL %d\n", ret);
    PERF_EXIT(TerminateProcess);
    return ret;
}

/*++
Function:
  PROCEndProcess

  Called from TerminateProcess and ExitProcess. This does the work of
  TerminateProcess, but also takes a flag that determines whether we
  shut down unconditionally. If the flag is set, the PAL will do very
  little extra work before exiting. Most importantly, it won't shut
  down any DLLs that are loaded.

--*/
static BOOL PROCEndProcess(HANDLE hProcess, UINT uExitCode, BOOL bTerminateUnconditionally)
{
    DWORD dwProcessId;
    BOOL ret = FALSE;

    dwProcessId = PROCGetProcessIDFromHandle(hProcess);
    if (dwProcessId == 0)
    {
        SetLastError(ERROR_INVALID_HANDLE);
    }
    else if(dwProcessId != GetCurrentProcessId())
    {
        if (uExitCode != 0)
            WARN("exit code 0x%x ignored for external process.\n", uExitCode);

        if (kill(dwProcessId, SIGKILL) == 0)
        {
            ret = TRUE;
        }
        else
        {
            switch (errno) {
            case ESRCH:
                SetLastError(ERROR_INVALID_HANDLE);
                break;
            case EPERM:
                SetLastError(ERROR_ACCESS_DENIED);
                break;
            default:
                // Unexpected failure.
                ASSERT(FALSE);
                SetLastError(ERROR_INTERNAL_ERROR);
                break;
            }
        }
    }
    else
    {
        // WARN/ERROR before starting the termination process and/or leaving the PAL.
        if (bTerminateUnconditionally)
        {
            WARN("exit code 0x%x ignored for terminate.\n", uExitCode);
        }
        else if ((uExitCode & 0xff) != uExitCode)
        {
            // TODO: Convert uExitCodes into sysexits(3)?
            ERROR("exit() only supports the lower 8-bits of an exit code. "
                "status will only see error 0x%x instead of 0x%x.\n", uExitCode & 0xff, uExitCode);
        }

        TerminateCurrentProcessNoExit(bTerminateUnconditionally);

        LOGEXIT("PROCEndProcess will not return\n");

        // exit() runs atexit handlers possibly registered by foreign code.
        // The right thing to do here is to leave the PAL.  If our client
        // registered our own PAL_Terminate with atexit(), the latter will
        // explicitly re-enter us.
        PAL_Leave(PAL_BoundaryBottom);

        if (bTerminateUnconditionally)
        {
            // abort() has the semantics that
            // (1) it doesn't run atexit handlers
            // (2) can invoke CrashReporter or produce a coredump,
            // which is appropriate for TerminateProcess calls

            // If this turns out to be inappropriate for some case, where we
            // call TerminateProcess vs. ExitProcess, then there needs to be
            // a CLR wrapper for TerminateProcess and some exposure for PAL_abort()
            // to selectively use that in all but those cases.

            abort();
        }
        else
        {
            exit(uExitCode);
        }

        ASSERT(FALSE); // we shouldn't get here
    }

    return ret;
}

/*++
Function:
  PROCCleanupProcess

  Do all cleanup work for TerminateProcess, but don't terminate the process.
  If bTerminateUnconditionally is TRUE, we exit as quickly as possible.

(no return value)
--*/
void PROCCleanupProcess(BOOL bTerminateUnconditionally)
{
    /* Declare the beginning of shutdown */
    PALSetShutdownIntent();

    PALCommonCleanup();

    /* This must be called after PALCommonCleanup */
    PALShutdown();
}

/*++
Function:
  GetProcessTimes

See MSDN doc.
--*/
BOOL
PALAPI
GetProcessTimes(
        IN HANDLE hProcess,
        OUT LPFILETIME lpCreationTime,
        OUT LPFILETIME lpExitTime,
        OUT LPFILETIME lpKernelTime,
        OUT LPFILETIME lpUserTime)
{
    BOOL retval = FALSE;
    struct rusage resUsage;
    __int64 calcTime;
    const __int64 SECS_TO_NS = 1000000000; /* 10^9 */
    const __int64 USECS_TO_NS = 1000;      /* 10^3 */


    PERF_ENTRY(GetProcessTimes);
    ENTRY("GetProcessTimes(hProcess=%p, lpExitTime=%p, lpKernelTime=%p,"
          "lpUserTime=%p)\n",
          hProcess, lpCreationTime, lpExitTime, lpKernelTime, lpUserTime );

    /* Make sure hProcess is the current process, this is the only supported
       case */
    if(PROCGetProcessIDFromHandle(hProcess)!=GetCurrentProcessId())
    {
        ASSERT("GetProcessTimes() does not work on a process other than the "
              "current process.\n");
        SetLastError(ERROR_INVALID_HANDLE);
        goto GetProcessTimesExit;
    }

    /* First, we need to actually retrieve the relevant statistics from the
       OS */
    if (getrusage (RUSAGE_SELF, &resUsage) == -1)
    {
        ASSERT("Unable to get resource usage information for the current "
              "process\n");
        SetLastError(ERROR_INTERNAL_ERROR);
        goto GetProcessTimesExit;
    }

    TRACE ("getrusage User: %ld sec,%ld microsec. Kernel: %ld sec,%ld"
           " microsec\n",
           resUsage.ru_utime.tv_sec, resUsage.ru_utime.tv_usec,
           resUsage.ru_stime.tv_sec, resUsage.ru_stime.tv_usec);

    if (lpUserTime)
    {
        /* Get the time of user mode execution, in 100s of nanoseconds */
        calcTime = (__int64)resUsage.ru_utime.tv_sec * SECS_TO_NS;
        calcTime += (__int64)resUsage.ru_utime.tv_usec * USECS_TO_NS;
        calcTime /= 100; /* Produce the time in 100s of ns */
        /* Assign the time into lpUserTime */
        lpUserTime->dwLowDateTime = (DWORD)calcTime;
        lpUserTime->dwHighDateTime = (DWORD)(calcTime >> 32);
    }

    if (lpKernelTime)
    {
        /* Get the time of kernel mode execution, in 100s of nanoseconds */
        calcTime = (__int64)resUsage.ru_stime.tv_sec * SECS_TO_NS;
        calcTime += (__int64)resUsage.ru_stime.tv_usec * USECS_TO_NS;
        calcTime /= 100; /* Produce the time in 100s of ns */
        /* Assign the time into lpUserTime */
        lpKernelTime->dwLowDateTime = (DWORD)calcTime;
        lpKernelTime->dwHighDateTime = (DWORD)(calcTime >> 32);
    }

    retval = TRUE;


GetProcessTimesExit:
    LOGEXIT("GetProcessTimes returns BOOL %d\n", retval);
    PERF_EXIT(GetProcessTimes);
    return (retval);
}

/*++
Function:
  GetCommandLineW

See MSDN doc.
--*/
LPWSTR
PALAPI
GetCommandLineW(
    VOID)
{
    PERF_ENTRY(GetCommandLineW);
    ENTRY("GetCommandLineW()\n");

    LPWSTR lpwstr = g_lpwstrCmdLine ? g_lpwstrCmdLine : (LPWSTR)W("");

    LOGEXIT("GetCommandLineW returns LPWSTR %p (%S)\n",
          g_lpwstrCmdLine,
          lpwstr);
    PERF_EXIT(GetCommandLineW);

    return lpwstr;
}

/*++
Function:
  OpenProcess

See MSDN doc.

Notes :
dwDesiredAccess is ignored (all supported operations will be allowed)
bInheritHandle is ignored (no inheritance)
--*/
HANDLE
PALAPI
OpenProcess(
        DWORD dwDesiredAccess,
        BOOL bInheritHandle,
        DWORD dwProcessId)
{
    PAL_ERROR palError;
    CPalThread *pThread;
    IPalObject *pobjProcess = NULL;
    IPalObject *pobjProcessRegistered = NULL;
    IDataLock *pDataLock;
    CProcProcessLocalData *pLocalData;
    CObjectAttributes oa;
    HANDLE hProcess = NULL;

    PERF_ENTRY(OpenProcess);
    ENTRY("OpenProcess(dwDesiredAccess=0x%08x, bInheritHandle=%d, "
          "dwProcessId = 0x%08x)\n",
          dwDesiredAccess, bInheritHandle, dwProcessId );

    pThread = InternalGetCurrentThread();

    if (0 == dwProcessId)
    {
        palError = ERROR_INVALID_PARAMETER;
        goto OpenProcessExit;
    }

    palError = g_pObjectManager->AllocateObject(
        pThread,
        &otProcess,
        &oa,
        &pobjProcess
        );

    if (NO_ERROR != palError)
    {
        goto OpenProcessExit;
    }

    palError = pobjProcess->GetProcessLocalData(
        pThread,
        WriteLock,
        &pDataLock,
        reinterpret_cast<void **>(&pLocalData)
        );

    if (NO_ERROR != palError)
    {
        goto OpenProcessExit;
    }

    pLocalData->dwProcessId = dwProcessId;
    pDataLock->ReleaseLock(pThread, TRUE);

    palError = g_pObjectManager->RegisterObject(
        pThread,
        pobjProcess,
        &aotProcess,
        dwDesiredAccess,
        &hProcess,
        &pobjProcessRegistered
        );

    //
    // pobjProcess was invalidated by the above call, so NULL
    // it out here
    //

    pobjProcess = NULL;

    //
    // TODO: check to see if the process actually exists?
    //

OpenProcessExit:

    if (NULL != pobjProcess)
    {
        pobjProcess->ReleaseReference(pThread);
    }

    if (NULL != pobjProcessRegistered)
    {
        pobjProcessRegistered->ReleaseReference(pThread);
    }

    if (NO_ERROR != palError)
    {
        pThread->SetLastError(palError);
    }

    LOGEXIT("OpenProcess returns HANDLE %p\n", hProcess);
    PERF_EXIT(OpenProcess);
    return hProcess;
}

#define FATAL_ASSERT(e, msg) \
    do \
    { \
        if (!(e)) \
        { \
            fprintf(stderr, "FATAL ERROR: " msg); \
            abort(); \
        } \
    } \
    while(0)

/*++
Function:
  PROCGetProcessIDFromHandle

Abstract
  Return the process ID from a process handle

Parameter
  hProcess:  process handle

Return
  Return the process ID, or 0 if it's not a valid handle
--*/
DWORD
PROCGetProcessIDFromHandle(
        HANDLE hProcess)
{
    PAL_ERROR palError;
    IPalObject *pobjProcess = NULL;
    CPalThread *pThread = InternalGetCurrentThread();

    DWORD dwProcessId = 0;

    if (hPseudoCurrentProcess == hProcess)
    {
        dwProcessId = gPID;
        goto PROCGetProcessIDFromHandleExit;
    }


    palError = g_pObjectManager->ReferenceObjectByHandle(
        pThread,
        hProcess,
        &aotProcess,
        0,
        &pobjProcess
        );

    if (NO_ERROR == palError)
    {
        IDataLock *pDataLock;
        CProcProcessLocalData *pLocalData;

        palError = pobjProcess->GetProcessLocalData(
            pThread,
            ReadLock,
            &pDataLock,
            reinterpret_cast<void **>(&pLocalData)
            );

        if (NO_ERROR == palError)
        {
            dwProcessId = pLocalData->dwProcessId;
            pDataLock->ReleaseLock(pThread, FALSE);
        }

        pobjProcess->ReleaseReference(pThread);
    }

PROCGetProcessIDFromHandleExit:

    return dwProcessId;
}

PAL_ERROR
CorUnix::InitializeProcessData(
    void
    )
{
    PAL_ERROR palError = NO_ERROR;
    bool fLockInitialized = FALSE;

    pGThreadList = NULL;
    g_dwThreadCount = 0;

    InternalInitializeCriticalSection(&g_csProcess);
    fLockInitialized = TRUE;

    if (NO_ERROR != palError)
    {
        if (fLockInitialized)
        {
            InternalDeleteCriticalSection(&g_csProcess);
        }
    }

    return palError;
}

/*++
Function
    InitializeProcessCommandLine

Abstract
    Initializes (or re-initializes) the saved command line and exe path.

Parameter
    lpwstrCmdLine
    lpwstrFullPath

Return
    PAL_ERROR

Notes
    This function takes ownership of lpwstrCmdLine, but not of lpwstrFullPath
--*/

PAL_ERROR
CorUnix::InitializeProcessCommandLine(
    LPWSTR lpwstrCmdLine,
    LPWSTR lpwstrFullPath
)
{
    PAL_ERROR palError = NO_ERROR;
    LPWSTR initial_dir = NULL;

    //
    // Save the command line and initial directory
    //

    if (lpwstrFullPath)
    {
        LPWSTR lpwstr = PAL_wcsrchr(lpwstrFullPath, '/');
        lpwstr[0] = '\0';
        size_t n = PAL_wcslen(lpwstrFullPath) + 1;

        size_t iLen = n;
        initial_dir = reinterpret_cast<LPWSTR>(InternalMalloc(iLen*sizeof(WCHAR)));
        if (NULL == initial_dir)
        {
            ERROR("malloc() failed! (initial_dir) \n");
            palError = ERROR_NOT_ENOUGH_MEMORY;
            goto exit;
        }

        if (wcscpy_s(initial_dir, iLen, lpwstrFullPath) != SAFECRT_SUCCESS)
        {
            ERROR("wcscpy_s failed!\n");
            free(initial_dir);
            palError = ERROR_INTERNAL_ERROR;
            goto exit;
        }

        lpwstr[0] = '/';

        free(g_lpwstrAppDir);
        g_lpwstrAppDir = initial_dir;
    }

    free(g_lpwstrCmdLine);
    g_lpwstrCmdLine = lpwstrCmdLine;

exit:
    return palError;
}


/*++
Function:
  CreateInitialProcessAndThreadObjects

Abstract
  Creates the IPalObjects that represent the current process
  and the initial thread

Parameter
  pThread - the initial thread

Return
  PAL_ERROR
--*/

PAL_ERROR
CorUnix::CreateInitialProcessAndThreadObjects(
    CPalThread *pThread
    )
{
    PAL_ERROR palError = NO_ERROR;
    HANDLE hThread;
    IPalObject *pobjProcess = NULL;
    IDataLock *pDataLock;
    CProcProcessLocalData *pLocalData;
    CObjectAttributes oa;
    HANDLE hProcess;

    //
    // Create initial thread object
    //

    palError = CreateThreadObject(pThread, pThread, &hThread);
    if (NO_ERROR != palError)
    {
        goto CreateInitialProcessAndThreadObjectsExit;
    }

    //
    // This handle isn't needed
    //

    (void) g_pObjectManager->RevokeHandle(pThread, hThread);

    //
    // Create and initialize process object
    //

    palError = g_pObjectManager->AllocateObject(
        pThread,
        &otProcess,
        &oa,
        &pobjProcess
        );

    if (NO_ERROR != palError)
    {
        ERROR("Unable to allocate process object");
        goto CreateInitialProcessAndThreadObjectsExit;
    }

    palError = pobjProcess->GetProcessLocalData(
        pThread,
        WriteLock,
        &pDataLock,
        reinterpret_cast<void **>(&pLocalData)
        );

    if (NO_ERROR != palError)
    {
        ASSERT("Unable to access local data");
        goto CreateInitialProcessAndThreadObjectsExit;
    }

    pLocalData->dwProcessId = gPID;
    pLocalData->ps = PS_RUNNING;
    pDataLock->ReleaseLock(pThread, TRUE);

    palError = g_pObjectManager->RegisterObject(
        pThread,
        pobjProcess,
        &aotProcess,
        PROCESS_ALL_ACCESS,
        &hProcess,
        &g_pobjProcess
        );

    //
    // pobjProcess is invalidated by the call to RegisterObject, so
    // NULL it out here to prevent it from being released later
    //

    pobjProcess = NULL;

    if (NO_ERROR != palError)
    {
        ASSERT("Failure registering process object");
        goto CreateInitialProcessAndThreadObjectsExit;
    }

    //
    // There's no need to keep this handle around, so revoke
    // it now
    //

    g_pObjectManager->RevokeHandle(pThread, hProcess);

CreateInitialProcessAndThreadObjectsExit:

    if (NULL != pobjProcess)
    {
        pobjProcess->ReleaseReference(pThread);
    }

    return palError;
}


/*++
Function:
  PROCCleanupInitialProcess

Abstract
  Cleanup all the structures for the initial process.

Parameter
  VOID

Return
  VOID

--*/
VOID
PROCCleanupInitialProcess(VOID)
{
    CPalThread *pThread = InternalGetCurrentThread();

    InternalEnterCriticalSection(pThread, &g_csProcess);

    /* Free the application directory */
    free(g_lpwstrAppDir);

    /* Free the stored command line */
    free(g_lpwstrCmdLine);

    InternalLeaveCriticalSection(pThread, &g_csProcess);

    //
    // Object manager shutdown will handle freeing the underlying
    // thread and process data
    //

}

/*++
Function:
  PROCAddThread

Abstract
  Add a thread to the thread list of the current process

Parameter
  pThread:   Thread object

--*/
VOID
CorUnix::PROCAddThread(
    CPalThread *pCurrentThread,
    CPalThread *pTargetThread
    )
{
    /* protect the access of the thread list with critical section for
       mutithreading access */
    InternalEnterCriticalSection(pCurrentThread, &g_csProcess);

    pTargetThread->SetNext(pGThreadList);
    pGThreadList = pTargetThread;
    g_dwThreadCount += 1;

    TRACE("Thread 0x%p (id %#x) added to the process thread list\n",
          pTargetThread, pTargetThread->GetThreadId());

    InternalLeaveCriticalSection(pCurrentThread, &g_csProcess);
}


/*++
Function:
  PROCRemoveThread

Abstract
  Remove a thread form the thread list of the current process

Parameter
  CPalThread *pThread : thread object to remove

(no return value)
--*/
VOID
CorUnix::PROCRemoveThread(
    CPalThread *pCurrentThread,
    CPalThread *pTargetThread
    )
{
    CPalThread *curThread, *prevThread;

    /* protect the access of the thread list with critical section for
       mutithreading access */
    InternalEnterCriticalSection(pCurrentThread, &g_csProcess);

    curThread = pGThreadList;

    /* if thread list is empty */
    if (curThread == NULL)
    {
        ASSERT("Thread list is empty.\n");
        goto EXIT;
    }

    /* do we remove the first thread? */
    if (curThread == pTargetThread)
    {
        pGThreadList =  curThread->GetNext();
        TRACE("Thread 0x%p (id %#x) removed from the process thread list\n",
            pTargetThread, pTargetThread->GetThreadId());
        goto EXIT;
    }

    prevThread = curThread;
    curThread = curThread->GetNext();
    /* find the thread to remove */
    while (curThread != NULL)
    {
        if (curThread == pTargetThread)
        {
            /* found, fix the chain list */
            prevThread->SetNext(curThread->GetNext());
            g_dwThreadCount -= 1;
            TRACE("Thread %p removed from the process thread list\n", pTargetThread);
            goto EXIT;
        }

        prevThread = curThread;
        curThread = curThread->GetNext();
    }

    WARN("Thread %p not removed (it wasn't found in the list)\n", pTargetThread);

EXIT:
    InternalLeaveCriticalSection(pCurrentThread, &g_csProcess);
}


/*++
Function:
  PROCGetNumberOfThreads

Abstract
  Return the number of threads in the thread list.

Parameter
  void

Return
  the number of threads.
--*/
INT
CorUnix::PROCGetNumberOfThreads(
    VOID)
{
    return g_dwThreadCount;
}


/*++
Function:
  PROCProcessLock

Abstract
  Enter the critical section associated to the current process

Parameter
  void

Return
  void
--*/
VOID
PROCProcessLock(
    VOID)
{
    CPalThread * pThread =
        (PALIsThreadDataInitialized() ? InternalGetCurrentThread() : NULL);

    InternalEnterCriticalSection(pThread, &g_csProcess);
}


/*++
Function:
  PROCProcessUnlock

Abstract
  Leave the critical section associated to the current process

Parameter
  void

Return
  void
--*/
VOID
PROCProcessUnlock(
    VOID)
{
    CPalThread * pThread =
        (PALIsThreadDataInitialized() ? InternalGetCurrentThread() : NULL);

    InternalLeaveCriticalSection(pThread, &g_csProcess);
}

#if USE_SYSV_SEMAPHORES
/*++
Function:
  PROCCleanupThreadSemIds

Abstract
  Cleanup SysV semaphore ids for all threads

(no parameters, no return value)
--*/
VOID
PROCCleanupThreadSemIds(void)
{
    //
    // When using SysV semaphores, the semaphore ids used by PAL threads must be removed
    // so they can be used again.
    //

    PROCProcessLock();

    CPalThread *pTargetThread = pGThreadList;
    while (NULL != pTargetThread)
    {
        pTargetThread->suspensionInfo.DestroySemaphoreIds();
        pTargetThread = pTargetThread->GetNext();
    }

    PROCProcessUnlock();

}
#endif // USE_SYSV_SEMAPHORES

/*++
Function:
  TerminateCurrentProcessNoExit

Abstract:
    Terminate current Process, but leave the caller alive

Parameters:
    BOOL bTerminateUnconditionally - If this is set, the PAL will exit as
    quickly as possible. In particular, it will not unload DLLs.

Return value :
    No return

Note:
  This function is used in ExitThread and TerminateProcess

--*/
VOID
CorUnix::TerminateCurrentProcessNoExit(BOOL bTerminateUnconditionally)
{
    BOOL locked;
    DWORD old_terminator;

    old_terminator = InterlockedCompareExchange(&terminator, GetCurrentThreadId(), 0);

    if (0 != old_terminator && GetCurrentThreadId() != old_terminator)
    {
        /* another thread has already initiated the termination process. we
           could just block on the PALInitLock critical section, but then
           PROCSuspendOtherThreads would hang... so sleep forever here, we're
           terminating anyway

           Update: [TODO] PROCSuspendOtherThreads has been removed. Can this
           code be changed? */

        /* note that if *this* thread has already started the termination
           process, we want to proceed. the only way this can happen is if a
           call to DllMain (from ExitProcess) brought us here (because DllMain
           called ExitProcess, or TerminateProcess, or ExitThread);
           TerminateProcess won't call DllMain, so there's no danger to get
           caught in an infinite loop */
        WARN("termination already started from another thread; blocking.\n");
        poll(NULL, 0, INFTIM);
    }

    /* Try to lock the initialization count to prevent multiple threads from
       terminating/initializing the PAL simultaneously */

    /* note : it's also important to take this lock before the process lock,
       because Init/Shutdown take the init lock, and the functions they call
       may take the process lock. We must do it in the same order to avoid
       deadlocks */
    locked = PALInitLock();
    if(locked && PALIsInitialized())
    {
         PROCCleanupProcess(bTerminateUnconditionally);
     }
}

/*++
Function:
    PROCGetProcessStatus

Abstract:
    Retrieve process state information (state & exit code).

Parameters:
    DWORD process_id : PID of process to retrieve state for
    PROCESS_STATE *state : state of process (starting, running, done)
    DWORD *exit_code : exit code of process (from ExitProcess, etc.)

Return value :
    TRUE on success
--*/
PAL_ERROR
PROCGetProcessStatus(
    CPalThread *pThread,
    HANDLE hProcess,
    PROCESS_STATE *pps,
    DWORD *pdwExitCode
    )
{
    PAL_ERROR palError = NO_ERROR;
    IPalObject *pobjProcess = NULL;
    IDataLock *pDataLock;
    CProcProcessLocalData *pLocalData;
    pid_t wait_retval;
    int status;

    //
    // First, check if we already know the status of this process. This will be
    // the case if this function has already been called for the same process.
    //

    palError = g_pObjectManager->ReferenceObjectByHandle(
        pThread,
        hProcess,
        &aotProcess,
        0,
        &pobjProcess
        );

    if (NO_ERROR != palError)
    {
        goto PROCGetProcessStatusExit;
    }

    palError = pobjProcess->GetProcessLocalData(
        pThread,
        WriteLock,
        &pDataLock,
        reinterpret_cast<void **>(&pLocalData)
        );

    if (PS_DONE == pLocalData->ps)
    {
        TRACE("We already called waitpid() on process ID %#x; process has "
              "terminated, exit code is %d\n",
              pLocalData->dwProcessId, pLocalData->dwExitCode);

        *pps = pLocalData->ps;
        *pdwExitCode = pLocalData->dwExitCode;

        pDataLock->ReleaseLock(pThread, FALSE);

        goto PROCGetProcessStatusExit;
    }

    /* By using waitpid(), we can even retrieve the exit code of a non-PAL
       process. However, note that waitpid() can only provide the low 8 bits
       of the exit code. This is all that is required for the PAL spec. */
    TRACE("Looking for status of process; trying wait()");

    while(1)
    {
        /* try to get state of process, using non-blocking call */
        wait_retval = waitpid(pLocalData->dwProcessId, &status, WNOHANG);

        if ( wait_retval == (pid_t) pLocalData->dwProcessId )
        {
            /* success; get the exit code */
            if ( WIFEXITED( status ) )
            {
                *pdwExitCode = WEXITSTATUS(status);
                TRACE("Exit code was %d\n", *pdwExitCode);
            }
            else
            {
                WARN("process terminated without exiting; can't get exit "
                     "code. faking it.\n");
                *pdwExitCode = EXIT_FAILURE;
            }
            *pps = PS_DONE;
        }
        else if (0 == wait_retval)
        {
            // The process is still running.
            TRACE("Process %#x is still active.\n", pLocalData->dwProcessId);
            *pps = PS_RUNNING;
            *pdwExitCode = 0;
        }
        else if (-1 == wait_retval)
        {
            // This might happen if waitpid() had already been called, but
            // this shouldn't happen - we call waitpid once, store the
            // result, and use that afterwards.
            // One legitimate cause of failure is EINTR; if this happens we
            // have to try again. A second legitimate cause is ECHILD, which
            // happens if we're trying to retrieve the status of a currently-
            // running process that isn't a child of this process.
            if (EINTR == errno)
            {
                TRACE("waitpid() failed with EINTR; re-waiting");
                continue;
            }
            else if (ECHILD == errno)
            {
                TRACE("waitpid() failed with ECHILD; calling kill instead");
                if (kill(pLocalData->dwProcessId, 0) != 0)
                {
                    if(ESRCH == errno)
                    {
                        WARN("kill() failed with ESRCH, i.e. target "
                             "process exited and it wasn't a child, "
                             "so can't get the exit code, assuming  "
                             "it was 0.\n");
                        *pdwExitCode = 0;
                    }
                    else
                    {
                        ERROR("kill(pid, 0) failed; errno is %d (%s)\n",
                              errno, strerror(errno));
                        *pdwExitCode = EXIT_FAILURE;
                    }
                    *pps = PS_DONE;
                }
                else
                {
                    *pps = PS_RUNNING;
                    *pdwExitCode = 0;
                }
            }
            else
            {
                // Ignoring unexpected waitpid errno and assuming that
                // the process is still running
                ERROR("waitpid(pid=%u) failed with unexpected errno=%d (%s)\n",
                      pLocalData->dwProcessId, errno, strerror(errno));
                *pps = PS_RUNNING;
                *pdwExitCode = 0;
            }
        }
        else
        {
            ASSERT("waitpid returned unexpected value %d\n",wait_retval);
            *pdwExitCode = EXIT_FAILURE;
            *pps = PS_DONE;
        }
        // Break out of the loop in all cases except EINTR.
        break;
    }

    // Save the exit code for future reference (waitpid will only work once).
    if(PS_DONE == *pps)
    {
        pLocalData->ps = PS_DONE;
        pLocalData->dwExitCode = *pdwExitCode;
    }

    TRACE( "State of process 0x%08x : %d (exit code %d)\n",
           pLocalData->dwProcessId, *pps, *pdwExitCode );

    pDataLock->ReleaseLock(pThread, TRUE);

PROCGetProcessStatusExit:

    if (NULL != pobjProcess)
    {
        pobjProcess->ReleaseReference(pThread);
    }

    return palError;
}

#ifdef _DEBUG
void PROCDumpThreadList()
{
    CPalThread *pThread;

    PROCProcessLock();

    TRACE ("Threads:{\n");

    pThread = pGThreadList;
    while (NULL != pThread)
    {
        TRACE ("    {pThr=0x%p tid=%#x lwpid=%#x state=%d finsusp=%d}\n",
               pThread, (int)pThread->GetThreadId(), (int)pThread->GetLwpId(),
               (int)pThread->synchronizationInfo.GetThreadState(),
               (int)pThread->suspensionInfo.GetSuspendedForShutdown());

        pThread = pThread->GetNext();
    }
    TRACE ("Threads:}\n");

    PROCProcessUnlock();
}
#endif

/* Internal function definitions **********************************************/

/*++
Function:
  getFileName

Abstract:
    Helper function for CreateProcessW, it retrieves the executable filename
    from the application name, and the command line.

Parameters:
    IN  lpApplicationName:  first parameter from CreateProcessW (an unicode string)
    IN  lpCommandLine: second parameter from CreateProcessW (an unicode string)
    OUT lpFileName: file to be executed (the new process)

Return:
    TRUE: if the file name is retrieved
    FALSE: otherwise

--*/
static
BOOL
getFileName(
       LPCWSTR lpApplicationName,
       LPWSTR lpCommandLine,
       char *lpPathFileName)
{
    LPWSTR lpEnd;
    WCHAR wcEnd;
    char * lpFileName;
    PathCharString lpFileNamePS;
    char *lpTemp;

    if (lpApplicationName)
    {
        int path_size = MAX_LONGPATH;
        lpTemp = lpPathFileName;
        /* if only a file name is specified, prefix it with "./" */
        if ((*lpApplicationName != '.') && (*lpApplicationName != '/') &&
            (*lpApplicationName != '\\'))
        {
            if (strcpy_s(lpPathFileName, MAX_LONGPATH, "./") != SAFECRT_SUCCESS)
            {
                ERROR("strcpy_s failed!\n");
                return FALSE;
            }
            lpTemp+=2;
            path_size -= 2;
       }

        /* Convert to ASCII */
        if (!WideCharToMultiByte(CP_ACP, 0, lpApplicationName, -1,
                                 lpTemp, path_size, NULL, NULL))
        {
            ASSERT("WideCharToMultiByte failure\n");
            return FALSE;
        }

        /* Replace '\' by '/' */
        FILEDosToUnixPathA(lpPathFileName);

        return TRUE;
    }
    else
    {
        /* use the Command line */

        /* filename should be the first token of the command line */

        /* first skip all leading whitespace */
        lpCommandLine = UTIL_inverse_wcspbrk(lpCommandLine,W16_WHITESPACE);
        if(NULL == lpCommandLine)
        {
            ERROR("CommandLine contains only whitespace!\n");
            return FALSE;
        }

        /* check if it is starting with a quote (") character */
        if (*lpCommandLine == 0x0022)
        {
            lpCommandLine++; /* skip the quote */

            /* file name ends with another quote */
            lpEnd = PAL_wcschr(lpCommandLine+1, 0x0022);

            /* if no quotes found, set lpEnd to the end of the Command line */
            if (lpEnd == NULL)
                lpEnd = lpCommandLine + PAL_wcslen(lpCommandLine);
        }
        else
        {
            /* filename is end out by a whitespace */
            lpEnd = PAL_wcspbrk(lpCommandLine, W16_WHITESPACE);

            /* if no whitespace found, set lpEnd to end of the Command line */
            if (lpEnd == NULL)
            {
                lpEnd = lpCommandLine + PAL_wcslen(lpCommandLine);
            }
        }

        if (lpEnd == lpCommandLine)
        {
            ERROR("application name and command line are both empty!\n");
            return FALSE;
        }

        /* replace the last character by a null */
        wcEnd = *lpEnd;
        *lpEnd = 0x0000;

        /* Convert to ASCII */
        int size = 0;
        int length = (PAL_wcslen(lpCommandLine)+1) * sizeof(WCHAR);
        lpFileName = lpFileNamePS.OpenStringBuffer(length);
        if (NULL == lpFileName)
        {
            ERROR("Not Enough Memory!\n");
            return FALSE;
        }
        if (!(size = WideCharToMultiByte(CP_ACP, 0, lpCommandLine, -1,
                                 lpFileName, length, NULL, NULL)))
        {
            ASSERT("WideCharToMultiByte failure\n");
            return FALSE;
        }

        lpFileNamePS.CloseBuffer(size);
        /* restore last character */
        *lpEnd = wcEnd;

        /* Replace '\' by '/' */
        FILEDosToUnixPathA(lpFileName);

        if (!getPath(lpFileName, MAX_LONGPATH, lpPathFileName))
        {
            /* file is not in the path */
            return FALSE;
        }
    }
    return TRUE;
}

/*++
Function:
    checkFileType

Abstract:
    Return the type of the file.

Parameters:
    IN  lpFileName:  file name

Return:
    FILE_DIR: Directory
    FILE_UNIX: Unix executable file
    FILE_ERROR: Error
--*/
static
int
checkFileType( LPCSTR lpFileName)
{
    struct stat stat_data;

    /* check if the file exist */
    if ( access(lpFileName, F_OK) != 0 )
    {
        return FILE_ERROR;
    }

    /* if it's not a PE/COFF file, check if it is executable */
    if ( -1 != stat( lpFileName, &stat_data ) )
    {
        if((stat_data.st_mode & S_IFMT) == S_IFDIR )
        {
            /*The given file is a directory*/
            return FILE_DIR;
        }
        if ( UTIL_IsExecuteBitsSet( &stat_data ) )
        {
            return FILE_UNIX;
        }
        else
        {
            return FILE_ERROR;
        }
    }
    return FILE_ERROR;

}


/*++
Function:
  buildArgv

Abstract:
    Helper function for CreateProcessW, it builds the array of argument in
    a format than can be passed to execve function.lppArgv is allocated
    in this function and must be freed by the caller.

Parameters:
    IN  lpCommandLine: second parameter from CreateProcessW (an unicode string)
    IN  lpAppPath: canonical name of the application to launched
    OUT lppArgv: array of arguments to be passed to the new process

Return:
    the number of arguments

note: this doesn't yet match precisely the behavior of Windows, but should be
sufficient.
what's here:
1) stripping nonquoted whitespace
2) handling of quoted parameters and quoted "parts" of parameters, removal of
   doublequotes (<aaaa"b bbb b"ccc> becomes <aaaab bbb bccc>)
3) \" as an escaped doublequote, both within doublequoted sequences and out
what's known missing :
1) \\ as an escaped backslash, but only if the string of '\'
   is followed by a " (escaped or not)
2) "alternate" escape sequence : double-doublequote within a double-quoted
    argument (<"aaa a""aa aaa">) expands to a single-doublequote(<aaa a"aa aaa>)
note that there may be other special cases
--*/
static
char **
buildArgv(
      LPCWSTR lpCommandLine,
      LPSTR lpAppPath,
      UINT *pnArg)
{
    CPalThread *pThread = NULL;
    UINT iWlen;
    char *lpAsciiCmdLine;
    char *pChar;
    char **lppArgv;
    char **lppTemp;
    UINT i,j;

    *pnArg = 0;

    iWlen = WideCharToMultiByte(CP_ACP,0,lpCommandLine,-1,NULL,0,NULL,NULL);

    if(0 == iWlen)
    {
        ASSERT("Can't determine length of command line\n");
        return NULL;
    }

    pThread = InternalGetCurrentThread();
    /* make sure to allocate enough space, up for the worst case scenario */
    int iLength = (iWlen + strlen(lpAppPath) + 2);
    lpAsciiCmdLine = (char *) InternalMalloc(iLength);

    if (lpAsciiCmdLine == NULL)
    {
        ERROR("Unable to allocate memory\n");
        return NULL;
    }

    pChar = lpAsciiCmdLine;

    /* put the canonical name of the application as the first parameter */
    if ((strcpy_s(lpAsciiCmdLine, iLength, "\"") != SAFECRT_SUCCESS) ||
        (strcat_s(lpAsciiCmdLine, iLength, lpAppPath) != SAFECRT_SUCCESS) ||
        (strcat_s(lpAsciiCmdLine, iLength,  "\"") != SAFECRT_SUCCESS) ||
        (strcat_s(lpAsciiCmdLine, iLength, " ") != SAFECRT_SUCCESS))
    {
        ERROR("strcpy_s/strcat_s failed!\n");
        return NULL;
    }

    pChar = lpAsciiCmdLine + strlen (lpAsciiCmdLine);

    /* let's skip the first argument in the command line */

    /* strip leading whitespace; function returns NULL if there's only
        whitespace, so the if statement below will work correctly */
    lpCommandLine = UTIL_inverse_wcspbrk((LPWSTR)lpCommandLine, W16_WHITESPACE);

    if (lpCommandLine)
    {
        LPCWSTR stringstart = lpCommandLine;

        do
        {
            /* find first whitespace or dquote character */
            lpCommandLine = PAL_wcspbrk(lpCommandLine,W16_WHITESPACE_DQUOTE);
            if(NULL == lpCommandLine)
            {
                /* no whitespace or dquote found : first arg is only arg */
                break;
            }
            else if('"' == *lpCommandLine)
            {
                /* got a dquote; skip over it if it's escaped; make sure we
                    don't try to look before the first character in the
                    string */
                if(lpCommandLine > stringstart && '\\' == lpCommandLine[-1])
                {
                    lpCommandLine++;
                    continue;
                }

                /* found beginning of dquoted sequence, run to the end */
                /* don't stop if we hit an escaped dquote */
                lpCommandLine++;
                while( *lpCommandLine )
                {
                    lpCommandLine = PAL_wcschr(lpCommandLine, '"');
                    if(NULL == lpCommandLine)
                    {
                        /* no ending dquote, arg runs to end of string */
                        break;
                    }
                    if('\\' != lpCommandLine[-1])
                    {
                        /* dquote is not escaped, dquoted sequence is over*/
                        break;
                    }
                    lpCommandLine++;
                }
                if(NULL == lpCommandLine || '\0' == *lpCommandLine)
                {
                    /* no terminating dquote */
                    break;
                }

                /* step over dquote, keep looking for end of arg */
                lpCommandLine++;
            }
            else
            {
                /* found whitespace : end of arg. */
                lpCommandLine++;
                break;
            }
        }while(lpCommandLine);
    }

    /* Convert to ASCII */
    if (lpCommandLine)
    {
        if (!WideCharToMultiByte(CP_ACP, 0, lpCommandLine, -1,
                                 pChar, iWlen+1, NULL, NULL))
        {
            ASSERT("Unable to convert to a multibyte string\n");
            free(lpAsciiCmdLine);
            return NULL;
        }
    }

    pChar = lpAsciiCmdLine;

    /* loops through all the arguments, to find out how many arguments there
       are; while looping replace whitespace by \0 */

    /* skip leading whitespace (and replace by '\0') */
    /* note : there shouldn't be any, command starts either with PE loader name
       or computed application path, but this won't hurt */
    while (*pChar)
    {
        if (!isspace((unsigned char) *pChar))
        {
           break;
        }
        WARN("unexpected whitespace in command line!\n");
        *pChar++ = '\0';
    }

    while (*pChar)
    {
        (*pnArg)++;

        /* find end of current arg */
        while(*pChar && !isspace((unsigned char) *pChar))
        {
            if('"' == *pChar)
            {
                /* skip over dquote if it's escaped; make sure we don't try to
                   look before the start of the string for the \ */
                if(pChar > lpAsciiCmdLine && '\\' == pChar[-1])
                {
                    pChar++;
                    continue;
                }

                /* found leading dquote : look for ending dquote */
                pChar++;
                while (*pChar)
                {
                    pChar = strchr(pChar,'"');
                    if(NULL == pChar)
                    {
                        /* no ending dquote found : argument extends to the end
                           of the string*/
                        break;
                    }
                    if('\\' != pChar[-1])
                    {
                        /* found a dquote, and it's not escaped : quoted
                           sequence is over*/
                        break;
                    }
                    /* found a dquote, but it was escaped : skip over it, keep
                       looking */
                    pChar++;
                }
                if(NULL == pChar || '\0' == *pChar)
                {
                    /* reached the end of the string : we're done */
                    break;
                }
            }
            pChar++;
        }
        if(NULL == pChar)
        {
            /* reached the end of the string : we're done */
            break;
        }
        /* reached end of arg; replace trailing whitespace by '\0', to split
           arguments into separate strings */
        while (isspace((unsigned char) *pChar))
        {
            *pChar++ = '\0';
        }
    }

    /* allocate lppargv according to the number of arguments
       in the command line */
    lppArgv = (char **) InternalMalloc((((*pnArg)+1) * sizeof(char *)));

    if (lppArgv == NULL)
    {
        free(lpAsciiCmdLine);
        return NULL;
    }

    lppTemp = lppArgv;

    /* at this point all parameters are separated by NULL
       we need to fill the array of arguments; we must also remove all dquotes
       from arguments (new process shouldn't see them) */
    for (i = *pnArg, pChar = lpAsciiCmdLine; i; i--)
    {
        /* skip NULLs */
        while (!*pChar)
        {
            pChar++;
        }

        *lppTemp = pChar;

        /* go to the next parameter, removing dquotes as we go along */
        j = 0;
        while (*pChar)
        {
            /* copy character if it's not a dquote */
            if('"' != *pChar)
            {
                /* if it's the \ of an escaped dquote, skip over it, we'll
                   copy the " instead */
                if( '\\' == pChar[0] && '"' == pChar[1] )
                {
                    pChar++;
                }
                (*lppTemp)[j++] = *pChar;
            }
            pChar++;
        }
        /* re-NULL terminate the argument */
        (*lppTemp)[j] = '\0';

        lppTemp++;
    }

    *lppTemp = NULL;

    return lppArgv;
}


/*++
Function:
  getPath

Abstract:
    Helper function for CreateProcessW, it looks in the path environment
    variable to find where the process to executed is.

Parameters:
    IN  lpFileName: file name to search in the path
    OUT lpPathFileName: returned string containing the path and the filename

Return:
    TRUE if found
    FALSE otherwise
--*/
static
BOOL
getPath(
      LPCSTR lpFileName,
      UINT iLen,
      LPSTR  lpPathFileName)
{
    LPSTR lpPath;
    LPSTR lpNext;
    LPSTR lpCurrent;
    LPWSTR lpwstr;
    INT n;
    INT nextLen;
    INT slashLen;
    CPalThread *pThread = NULL;

    /* if a path is specified, only look there */
    if(strchr(lpFileName, '/'))
    {
        if (access (lpFileName, F_OK) == 0)
        {
            if (strcpy_s(lpPathFileName, iLen, lpFileName) != SAFECRT_SUCCESS)
            {
                TRACE("strcpy_s failed!\n");
                return FALSE;
            }

            TRACE("file %s exists\n", lpFileName);
            return TRUE;
        }
        else
        {
            TRACE("file %s doesn't exist.\n", lpFileName);
            return FALSE;
        }
    }

    /* first look in directory from which the application loaded */
    lpwstr = g_lpwstrAppDir;

    if (lpwstr)
    {
        /* convert path to multibyte, check buffer size */
        n = WideCharToMultiByte(CP_ACP, 0, lpwstr, -1, lpPathFileName, iLen,
            NULL, NULL);
        if (n == 0)
        {
            ASSERT("WideCharToMultiByte failure!\n");
            return FALSE;
        }

        n += strlen(lpFileName) + 2;
        if (n > (INT)iLen)
        {
            ERROR("Buffer too small for full path!\n");
            return FALSE;
        }

        if ((strcat_s(lpPathFileName, iLen, "/") != SAFECRT_SUCCESS) ||
            (strcat_s(lpPathFileName, iLen, lpFileName) != SAFECRT_SUCCESS))
        {
            ERROR("strcat_s failed!\n");
            return FALSE;
        }

        if (access(lpPathFileName, F_OK) == 0)
        {
            TRACE("found %s in application directory (%s)\n", lpFileName, lpPathFileName);
            return TRUE;
        }
    }

    /* then try the current directory */
    if ((strcpy_s(lpPathFileName, iLen, "./") != SAFECRT_SUCCESS) ||
        (strcat_s(lpPathFileName, iLen, lpFileName) != SAFECRT_SUCCESS))
    {
        ERROR("strcpy_s/strcat_s failed!\n");
        return FALSE;
    }

    if (access (lpPathFileName, R_OK) == 0)
    {
        TRACE("found %s in current directory.\n", lpFileName);
        return TRUE;
    }

    pThread = InternalGetCurrentThread();
    /* Then try to look in the path */
    int iLen2 = strlen(MiscGetenv("PATH"))+1;
    lpPath = (LPSTR) InternalMalloc(iLen2);

    if (!lpPath)
    {
        ERROR("couldn't allocate memory for $PATH\n");
        return FALSE;
    }

    if (strcpy_s(lpPath, iLen2, MiscGetenv("PATH")) != SAFECRT_SUCCESS)
    {
        ERROR("strcpy_s failed!");
        return FALSE;
    }

    lpNext = lpPath;

    /* search in every path directory */
    TRACE("looking for file %s in $PATH (%s)\n", lpFileName, lpPath);
    while (lpNext)
    {
        /* skip all leading ':' */
        while(*lpNext==':')
        {
            lpNext++;
        }

        /* search for ':' */
        lpCurrent = strchr(lpNext, ':');
        if (lpCurrent)
        {
            *lpCurrent++ = '\0';
        }

        nextLen = strlen(lpNext);
        slashLen = (lpNext[nextLen-1] == '/') ? 0:1;

        /* verify if the path fit in the OUT parameter */
        if (slashLen + nextLen + strlen (lpFileName) >= iLen)
        {
            InternalFree(lpPath);
            ERROR("buffer too small for full path\n");
            return FALSE;
        }

        strcpy_s (lpPathFileName, iLen, lpNext);

        /* append a '/' if there's no '/' at the end of the path */
        if ( slashLen == 1 )
        {
            strcat_s (lpPathFileName, iLen, "/");
        }

        strcat_s (lpPathFileName, iLen, lpFileName);

        if (access (lpPathFileName, F_OK) == 0)
        {
            TRACE("Found %s in $PATH element %s\n", lpFileName, lpNext);
            InternalFree(lpPath);
            return TRUE;
        }

        lpNext = lpCurrent;  /* search in the next directory */
    }

    InternalFree(lpPath);
    TRACE("File %s not found in $PATH\n", lpFileName);
    return FALSE;
}
