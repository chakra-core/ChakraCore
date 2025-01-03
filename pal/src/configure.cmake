# xplat-todo: Cleanup this file!
# we have `feature detection` for direct compilation target and manual entries for cross-compilation below
if (NOT CC_TARGET_OS_ANDROID)

include(CheckCXXSourceCompiles)
include(CheckCXXSourceRuns)
include(CheckCXXSymbolExists)
include(CheckFunctionExists)
include(CheckIncludeFiles)
include(CheckStructHasMember)
include(CheckTypeSize)
include(CheckLibraryExists)

if(CMAKE_SYSTEM_NAME STREQUAL FreeBSD)
  set(CMAKE_REQUIRED_INCLUDES /usr/local/include)
elseif(CMAKE_SYSTEM_NAME STREQUAL SunOS)
  set(CMAKE_REQUIRED_INCLUDES /opt/local/include)
endif()
if(NOT CMAKE_SYSTEM_NAME STREQUAL Darwin AND NOT CMAKE_SYSTEM_NAME STREQUAL FreeBSD)
  set(CMAKE_REQUIRED_DEFINITIONS "-D_DEFAULT_SOURCE -D_POSIX_C_SOURCE=200809L")
endif()

list(APPEND CMAKE_REQUIRED_DEFINITIONS -D_FILE_OFFSET_BITS=64)

check_include_files(ieeefp.h HAVE_IEEEFP_H)
check_include_files(alloca.h HAVE_ALLOCA_H)
check_include_files(sys/vmparam.h HAVE_SYS_VMPARAM_H)
check_include_files(mach/vm_types.h HAVE_MACH_VM_TYPES_H)
check_include_files(mach/vm_param.h HAVE_MACH_VM_PARAM_H)
check_include_files(procfs.h HAVE_PROCFS_H)
check_include_files(crt_externs.h HAVE_CRT_EXTERNS_H)
check_include_files(sys/time.h HAVE_SYS_TIME_H)
check_include_files(pthread_np.h HAVE_PTHREAD_NP_H)
check_include_files(sys/lwp.h HAVE_SYS_LWP_H)
check_include_files(runetype.h HAVE_RUNETYPE_H)
check_include_files(unicode/uchar.h HAVE_LIBICU_UCHAR_H)

check_function_exists(kqueue HAVE_KQUEUE)
check_function_exists(getpwuid_r HAVE_GETPWUID_R)
check_library_exists(pthread pthread_suspend "" HAVE_PTHREAD_SUSPEND)
check_library_exists(pthread pthread_suspend_np "" HAVE_PTHREAD_SUSPEND_NP)
check_library_exists(pthread pthread_continue "" HAVE_PTHREAD_CONTINUE)
check_library_exists(pthread pthread_continue_np "" HAVE_PTHREAD_CONTINUE_NP)
check_library_exists(pthread pthread_resume_np "" HAVE_PTHREAD_RESUME_NP)
check_library_exists(pthread pthread_attr_get_np "" HAVE_PTHREAD_ATTR_GET_NP)
check_library_exists(pthread pthread_getattr_np "" HAVE_PTHREAD_GETATTR_NP)
check_library_exists(pthread pthread_getcpuclockid "" HAVE_PTHREAD_GETCPUCLOCKID)
check_library_exists(pthread pthread_sigqueue "" HAVE_PTHREAD_SIGQUEUE)
check_function_exists(sigreturn HAVE_SIGRETURN)
check_function_exists(_thread_sys_sigreturn HAVE__THREAD_SYS_SIGRETURN)
check_function_exists(copysign HAVE_COPYSIGN)
check_function_exists(fsync HAVE_FSYNC)
check_function_exists(futimes HAVE_FUTIMES)
check_function_exists(utimes HAVE_UTIMES)
check_function_exists(sysctl HAVE_SYSCTL)
check_function_exists(sysconf HAVE_SYSCONF)
check_function_exists(localtime_r HAVE_LOCALTIME_R)
check_function_exists(gmtime_r HAVE_GMTIME_R)
check_function_exists(timegm HAVE_TIMEGM)
check_function_exists(_snwprintf HAVE__SNWPRINTF)
check_function_exists(poll HAVE_POLL)
check_function_exists(statvfs HAVE_STATVFS)
check_function_exists(thread_self HAVE_THREAD_SELF)
check_function_exists(_lwp_self HAVE__LWP_SELF)
check_function_exists(pthread_mach_thread_np HAVE_MACH_THREADS)
check_function_exists(thread_set_exception_ports HAVE_MACH_EXCEPTIONS)
check_function_exists(vm_allocate HAVE_VM_ALLOCATE)
check_function_exists(vm_read HAVE_VM_READ)
check_function_exists(directio HAVE_DIRECTIO)
check_function_exists(semget HAS_SYSV_SEMAPHORES)
check_function_exists(pthread_mutex_init HAS_PTHREAD_MUTEXES)
check_function_exists(ttrace HAVE_TTRACE)

check_struct_has_member ("struct stat" st_atimespec "sys/types.h;sys/stat.h" HAVE_STAT_TIMESPEC)
check_struct_has_member ("struct stat" st_atimensec "sys/types.h;sys/stat.h" HAVE_STAT_NSEC)
check_struct_has_member ("struct tm" tm_gmtoff time.h HAVE_TM_GMTOFF)
check_struct_has_member ("ucontext_t" uc_mcontext.gregs[0] ucontext.h HAVE_GREGSET_T)

set(CMAKE_EXTRA_INCLUDE_FILES machine/reg.h)
check_type_size("struct reg" BSD_REGS_T)
set(CMAKE_EXTRA_INCLUDE_FILES)
set(CMAKE_EXTRA_INCLUDE_FILES asm/ptrace.h)
check_type_size("struct pt_regs" PT_REGS)
set(CMAKE_EXTRA_INCLUDE_FILES)
set(CMAKE_EXTRA_INCLUDE_FILES signal.h)
check_type_size(siginfo_t SIGINFO_T)
set(CMAKE_EXTRA_INCLUDE_FILES)
set(CMAKE_EXTRA_INCLUDE_FILES ucontext.h)
check_type_size(ucontext_t UCONTEXT_T)
set(CMAKE_EXTRA_INCLUDE_FILES)
set(CMAKE_EXTRA_INCLUDE_FILES pthread.h)
check_type_size(pthread_rwlock_t PTHREAD_RWLOCK_T)
set(CMAKE_EXTRA_INCLUDE_FILES)
set(CMAKE_EXTRA_INCLUDE_FILE procfs.h)
check_type_size(prwatch_t PRWATCH_T)
set(CMAKE_EXTRA_INCLUDE_FILE)
check_type_size(off_t SIZEOF_OFF_T)

check_cxx_symbol_exists(SYS_yield sys/syscall.h HAVE_YIELD_SYSCALL)
check_cxx_symbol_exists(INFTIM poll.h HAVE_INFTIM)
check_cxx_symbol_exists(CHAR_BIT sys/limits.h HAVE_CHAR_BIT)
check_cxx_symbol_exists(_DEBUG sys/user.h USER_H_DEFINES_DEBUG)
check_cxx_symbol_exists(_SC_PHYS_PAGES unistd.h HAVE__SC_PHYS_PAGES)
check_cxx_symbol_exists(_SC_AVPHYS_PAGES unistd.h HAVE__SC_AVPHYS_PAGES)

check_cxx_source_runs("
#include <stdlib.h>
#include <stdio.h>

extern \"C++\" {
  inline long long abs(long long _X) {
    return llabs(_X);
  }
}

int main(int argc, char **argv) {
  long long X = 123456789 + argc;
  printf(\"%lld\", abs(X));
  exit(0);
}" PLATFORM_ACCEPTS_ABS_OVERLOAD)
check_cxx_source_runs("
#include <sys/param.h>
#include <stdlib.h>

int main(void) {
  char *path;
#ifdef PATH_MAX
  char resolvedPath[PATH_MAX];
#elif defined(MAXPATHLEN)
  char resolvedPath[MAXPATHLEN];
#else
  char resolvedPath[1024];
#endif
  path = realpath(\"a_nonexistent_file\", resolvedPath);
  if (path == NULL) {
    exit(1);
  }
  exit(0);
}" REALPATH_SUPPORTS_NONEXISTENT_FILES)
check_cxx_source_runs("
#include <stdio.h>
#include <stdlib.h>
int main(void)
{
  long long n = 0;
  sscanf(\"5000000000\", \"%qu\", &n);
  exit (n != 5000000000);
  }" SSCANF_SUPPORT_ll)
check_cxx_source_runs("
#include <stdio.h>
#include <stdlib.h>

int main()
{
  int ret;
  float f = 0;
  char * strin = \"12.34e\";

  ret = sscanf (strin, \"%e\", &f);
  if (ret <= 0)
    exit (0);
  exit(1);
}" SSCANF_CANNOT_HANDLE_MISSING_EXPONENT)
check_cxx_source_runs("
#include <stdio.h>
#include <stdlib.h>

int main(void) {
  char buf[256] = { 0 };
  snprintf(buf, 0x7fffffff, \"%#x\", 0x12345678);
  if (buf[0] == 0x0) {
    exit(1);
  }
  exit(0);
}" HAVE_LARGE_SNPRINTF_SUPPORT)
check_cxx_source_runs("
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>

int main(void) {
    int fd, numFDs;
    fd_set readFDs, writeFDs, exceptFDs;
    struct timeval time = { 0 };
    char * filename = NULL;

    filename = (char *)malloc(L_tmpnam * sizeof(char)); /* ok to leak this at exit */
    if (NULL == filename) {
      exit(0);
    }

    /* On some platforms (e.g. HP-UX) the multithreading c-runtime does not
       support the tmpnam(NULL) semantics, and it returns NULL. Therefore
       we need to use the tmpnam(pbuffer) version.
    */
    if (NULL == tmpnam(filename)) {
      exit(0);
    }
    if (mkfifo(filename, S_IRWXU) != 0) {
      if (unlink(filename) != 0) {
        exit(0);
      }
      if (mkfifo(filename, S_IRWXU) != 0) {
        exit(0);
      }
    }
    fd = open(filename, O_RDWR | O_NONBLOCK);
    if (fd == -1) {
      exit(0);
    }

    FD_ZERO(&readFDs);
    FD_ZERO(&writeFDs);
    FD_ZERO(&exceptFDs);
    FD_SET(fd, &readFDs);
    numFDs = select(fd + 1, &readFDs, &writeFDs, &exceptFDs, &time);

    close(fd);
    unlink(filename);

    /* numFDs is zero if select() works correctly */
    exit(numFD==0);
}" HAVE_BROKEN_FIFO_SELECT)
check_cxx_source_runs("
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>
#include <sys/stat.h>

int main(void)
{
  int ikq;
  int iRet;
  int fd;
  struct kevent ke, keChangeList;
  struct timespec ts = { 0, 0 };

  char * filename = NULL;

  filename = (char *)malloc(L_tmpnam * sizeof(char)); /* ok to leak this at exit */
  if (NULL == filename)
  {
    exit(1);
  }

  /* On some platforms (e.g. HP-UX) the multithreading c-runtime does not
     support the tmpnam(NULL) semantics, and it returns NULL. Therefore
     we need to use the tmpnam(pbuffer) version.
  */
  if (NULL == tmpnam(filename)) {
    exit(0);
  }
  if (mkfifo(filename, S_IRWXU) != 0) {
    if (unlink(filename) != 0) {
      exit(0);
    }
    if (mkfifo(filename, S_IRWXU) != 0) {
      exit(0);
    }
  }
  fd = open(filename, O_RDWR | O_NONBLOCK);
  if (fd == -1) {
    exit(0);
  }

  EV_SET(&keChangeList, fd, EVFILT_READ, EV_ADD | EV_CLEAR, 0, 0, NULL);
  ikq = kqueue();
  iRet = kevent(ikq, &keChangeList, 1, &ke, 1, &ts);

  close(fd);
  unlink(filename);

  /* iRet is zero is kevent() works correctly */
  return(iRet==0);
}" HAVE_BROKEN_FIFO_KEVENT)
set(CMAKE_REQUIRED_LIBRARIES pthread)
check_cxx_source_runs("
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sched.h>

int main(void)
{
  int policy;
  struct sched_param schedParam;
  int max_priority;
  int min_priority;

  if (0 != pthread_getschedparam(pthread_self(), &policy, &schedParam))
  {
    exit(1);
  }

  max_priority = sched_get_priority_max(policy);
  min_priority = sched_get_priority_min(policy);

  exit(-1 == max_priority || -1 == min_priority);
}" HAVE_SCHED_GET_PRIORITY)
set(CMAKE_REQUIRED_LIBRARIES pthread)
check_cxx_source_runs("
#include <stdlib.h>
#include <sched.h>

int main(void)
{
  if (sched_getcpu() >= 0)
  {
    exit(0);
  }
  exit(1);
}" HAVE_SCHED_GETCPU)
set(CMAKE_REQUIRED_LIBRARIES)
check_cxx_source_runs("
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>

int main()
{
  int ret;
  struct timeval tv;
  ret = gettimeofday(&tv, NULL);

  exit(ret);
}" HAVE_WORKING_GETTIMEOFDAY)
check_cxx_source_runs("
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>

int main()
{
  int ret;
#ifndef __APPLE__
  struct timespec ts;
  ret = clock_gettime(CLOCK_REALTIME, &ts);
#else
  ret = 1; // do not use clock_gettime on osx/ios (backward compatibility)
#endif
  exit(ret);
}" HAVE_WORKING_CLOCK_GETTIME)
check_cxx_source_runs("
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>

int main()
{
  int ret;
#ifndef __APPLE__
  struct timespec ts;
  ret = clock_gettime(CLOCK_MONOTONIC, &ts);
#else
  ret = 1; // do not use clock_gettime on osx/ios (backward compatibility)
#endif
  exit(ret);
}" HAVE_CLOCK_MONOTONIC)
check_cxx_source_runs("
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>

int main()
{
  int ret;
#ifndef __APPLE__
  struct timespec ts;
  ret = clock_gettime(CLOCK_MONOTONIC_COARSE, &ts);
#else
  ret = 1; // do not use clock_gettime on osx/ios (backward compatibility)
#endif
  exit(ret);
}" HAVE_CLOCK_MONOTONIC_COARSE)
check_cxx_source_runs("
#include <stdlib.h>
#include <mach/mach_time.h>

int main()
{
  int ret;
  mach_timebase_info_data_t timebaseInfo;
  ret = mach_timebase_info(&timebaseInfo);
  mach_absolute_time();
  exit(ret);
}" HAVE_MACH_ABSOLUTE_TIME)
check_cxx_source_runs("
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>

int main()
{
  int ret;
#ifndef __APPLE__
  struct timespec ts;
  ret = clock_gettime(CLOCK_THREAD_CPUTIME_ID, &ts);
#else
  ret = 1; // do not use clock_gettime on osx/ios (backward compatibility)
#endif
  exit(ret);
}" HAVE_CLOCK_THREAD_CPUTIME)
check_cxx_source_runs("
#include <stdlib.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <fcntl.h>

int main(void) {
  int devzero;
  void *retval;

  devzero = open(\"/dev/zero\", O_RDWR);
  if (-1 == devzero) {
    exit(1);
  }
  retval = mmap(NULL, 4096, PROT_READ|PROT_WRITE, MAP_SHARED, devzero, 0);
  if (retval == (void *)-1) {
    exit(1);
  }
  exit(0);
}" HAVE_MMAP_DEV_ZERO)
check_cxx_source_runs("
#include <fcntl.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <unistd.h>

#ifndef MAP_ANON
#define MAP_ANON MAP_ANONYMOUS
#endif

int main(void) {
  void *hint, *ptr;
  int pagesize;
  int fd;

  pagesize = getpagesize();
  fd = open(\"/etc/passwd\", O_RDONLY);
  if (fd == -1) {
    exit(0);
  }
  ptr = mmap(NULL, pagesize, PROT_NONE, MAP_ANON | MAP_PRIVATE, -1, 0);
  if (ptr == MAP_FAILED) {
    exit(0);
  }
  hint = mmap(NULL, pagesize, PROT_NONE, MAP_ANON | MAP_PRIVATE, -1, 0);
  if (hint == MAP_FAILED) {
    exit(0);
  }
  if (munmap(ptr, pagesize) != 0) {
    exit(0);
  }
  if (munmap(hint, pagesize) != 0) {
    exit(0);
  }
  ptr = mmap(hint, pagesize, PROT_NONE, MAP_FIXED | MAP_PRIVATE, fd, 0);
  if (ptr == MAP_FAILED || ptr != hint) {
    exit(0);
  }
  exit(1);
}" MMAP_IGNORES_HINT)
check_cxx_source_runs("
#include <sys/types.h>
#include <sys/mman.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

#ifndef MAP_ANON
#define MAP_ANON MAP_ANONYMOUS
#endif

void *handle_signal(int signal) {
  /* If we reach this, we've crashed due to mmap honoring
  PROT_NONE. */
  _exit(1);
}

int main(void) {
  int *ptr;
  struct sigaction action;

  ptr = (int *) mmap(NULL, getpagesize(), PROT_NONE,
                     MAP_ANON | MAP_PRIVATE, -1, 0);
  if (ptr == (int *) MAP_FAILED) {
    exit(0);
  }
  action.sa_handler = &handle_signal;
  action.sa_flags = 0;
  sigemptyset(&action.sa_mask);
  if (sigaction(SIGBUS, &action, NULL) != 0) {
    exit(0);
  }
  if (sigaction(SIGSEGV, &action, NULL) != 0) {
    exit(0);
  }
  /* This will drop us into the signal handler if PROT_NONE
     is honored. */
  *ptr = 123;
  exit(0);
}" MMAP_ANON_IGNORES_PROTECTION)
check_cxx_source_runs("
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>

#ifndef MAP_ANON
#define MAP_ANON MAP_ANONYMOUS
#endif

int main()
{
  int iRet = 1;
  void * pAddr = MAP_FAILED;
  int MemSize = 1024;

  MemSize = getpagesize();
  pAddr = mmap(0x0, MemSize, PROT_NONE, MAP_PRIVATE | MAP_ANON, -1, 0);
  if (pAddr == MAP_FAILED)
    exit(0);

  pAddr = mmap(pAddr, MemSize, PROT_WRITE | PROT_READ, MAP_FIXED | MAP_PRIVATE | MAP_ANON, -1, 0);
  if (pAddr == MAP_FAILED)
    iRet = 0;

  munmap(pAddr, MemSize); // don't care of this
  exit (iRet);
}" MMAP_DOESNOT_ALLOW_REMAP)
check_cxx_source_runs("
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/mman.h>

#define MEM_SIZE 1024

int main(void)
{
  char * fname;
  int fd;
  int ret;
  void * pAddr0, * pAddr1;

  fname = (char *)malloc(MEM_SIZE);
  if (!fname)
    exit(1);
  strcpy(fname, \"/tmp/name/multiplemaptestXXXXXX\");

  fd = mkstemp(fname);
  if (fd < 0)
    exit(1);

  ret = write (fd, (void *)fname, MEM_SIZE);
  if (ret < 0)
    exit(1);

  pAddr0 = mmap(0, MEM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  pAddr1 = mmap(0, MEM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

  /* In theory we should look for (pAddr1 == MAP_FAILED) && (pAddr1 != MAP_FAILED)
   but in case the first test also failed, i.e. we failed to run the test,
   let's assume that the system might not allow multiple shared mapping of the
   same file region in the same process. The code enabled in this case is
   only a fall-back code path. In case the double mmap actually works, virtually
   nothing will change and the normal code path will be executed */
  if (pAddr1 == MAP_FAILED)
    ret = 1;
  else
    ret = 0;

  if (pAddr0)
    munmap (pAddr0, MEM_SIZE);
  if (pAddr1)
    munmap (pAddr1, MEM_SIZE);
  close(fd);
  unlink(fname);
  free(fname);

  exit(ret != 1);
}" ONE_SHARED_MAPPING_PER_FILEREGION_PER_PROCESS)
set(CMAKE_REQUIRED_LIBRARIES pthread)
check_cxx_source_runs("
#include <errno.h>
#include <pthread.h>
#include <stdlib.h>

void *start_routine(void *param) { return NULL; }

int main() {
  int result;
  pthread_t tid;

  errno = 0;
  result = pthread_create(&tid, NULL, start_routine, NULL);
  if (result != 0) {
    exit(1);
  }
  if (errno != 0) {
    exit(0);
  }
  exit(1);
}" PTHREAD_CREATE_MODIFIES_ERRNO)
set(CMAKE_REQUIRED_LIBRARIES)
set(CMAKE_REQUIRED_LIBRARIES pthread)
check_cxx_source_runs("
#include <errno.h>
#include <semaphore.h>
#include <stdlib.h>

int main() {
  int result;
  sem_t sema;

  errno = 50;
  result = sem_init(&sema, 0, 0);
  if (result != 0)
  {
    exit(1);
  }
  if (errno != 50)
  {
    exit(0);
  }
  exit(1);
}" SEM_INIT_MODIFIES_ERRNO)
set(CMAKE_REQUIRED_LIBRARIES)
check_cxx_source_runs("
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

int main(void) {
  int fd;
#ifdef PATH_MAX
  char path[PATH_MAX];
#elif defined(MAXPATHLEN)
  char path[MAXPATHLEN];
#else
  char path[1024];
#endif

  sprintf(path, \"/proc/%u/$1\", getpid());
  fd = open(path, $2);
  if (fd == -1) {
    exit(1);
  }
  exit(0);
}" HAVE_PROCFS_CTL)
set(CMAKE_REQUIRED_LIBRARIES m)
check_cxx_source_runs("
#include <math.h>
#include <stdlib.h>

int main(void) {
  if (!isnan(acos(10))) {
    exit(1);
  }
  exit(0);
}" HAVE_COMPATIBLE_ACOS)
set(CMAKE_REQUIRED_LIBRARIES)
set(CMAKE_REQUIRED_LIBRARIES m)
check_cxx_source_runs("
#include <math.h>
#include <stdlib.h>

int main(void) {
  if (!isnan(asin(10))) {
    exit(1);
  }
  exit(0);
}" HAVE_COMPATIBLE_ASIN)
set(CMAKE_REQUIRED_LIBRARIES)
set(CMAKE_REQUIRED_LIBRARIES m)
check_cxx_source_runs("
#include <math.h>
#include <stdlib.h>

int main(void) {
  double pi = 3.14159265358979323846;
  double result;

  result = atan2(0.0, -0.0);
  if (fabs(pi - result) > 0.0000001) {
    exit(1);
  }

  result = atan2(-0.0, -0.0);
  if (fabs(-pi - result) > 0.0000001) {
    exit(1);
  }

  result = atan2 (-0.0, 0.0);
  if (result != 0.0 || copysign (1.0, result) > 0) {
    exit(1);
  }

  result = atan2 (0.0, 0.0);
  if (result != 0.0 || copysign (1.0, result) < 0) {
    exit(1);
  }

  exit (0);
}" HAVE_COMPATIBLE_ATAN2)
set(CMAKE_REQUIRED_LIBRARIES)
set(CMAKE_REQUIRED_LIBRARIES m)
check_cxx_source_runs("
#include <math.h>
#include <stdlib.h>

int main(void) {
  double d = exp(1.0), e = M_E;

  /* Used memcmp rather than == to test that the doubles are equal to
   prevent gcc's optimizer from using its 80 bit internal long
   doubles. If you use ==, then on BSD you get a false negative since
   exp(1.0) == M_E to 64 bits, but not 80.
  */

  if (memcmp (&d, &e, sizeof (double)) == 0) {
    exit(0);
  }
  exit(1);
}" HAVE_COMPATIBLE_EXP)
set(CMAKE_REQUIRED_LIBRARIES)
set(CMAKE_REQUIRED_LIBRARIES m)
check_cxx_source_runs("
#include <math.h>
#include <stdlib.h>

int main(void) {
  if (!isnan(log(-10000))) {
    exit(1);
  }
  exit(0);
}" HAVE_COMPATIBLE_LOG)
set(CMAKE_REQUIRED_LIBRARIES)
set(CMAKE_REQUIRED_LIBRARIES m)
check_cxx_source_runs("
#include <math.h>
#include <stdlib.h>

int main(void) {
  if (!isnan(log10(-10000))) {
    exit(1);
  }
  exit(0);
}" HAVE_COMPATIBLE_LOG10)
set(CMAKE_REQUIRED_LIBRARIES)
check_cxx_source_runs("
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(void)
{
  char* szFileName;
  FILE* pFile = NULL;
  int ret = 1;

  szFileName = tempnam(\".\", \"tmp\");

  /* open the file write-only */
  pFile = fopen(szFileName, \"a\");
  if (pFile == NULL)
  {
    exit(0);
  }
  if (ungetc('A', pFile) != EOF)
  {
    ret = 0;
  }
  unlink(szFileName);
  exit(ret);
}" UNGETC_NOT_RETURN_EOF)
set(CMAKE_REQUIRED_LIBRARIES pthread)
check_cxx_source_runs("
#include <stdlib.h>
#include <errno.h>
#include <semaphore.h>

int main() {
  sem_t sema;
  if (sem_init(&sema, 0, 0) == -1){
    exit(1);
  }
  exit(0);
}" HAS_POSIX_SEMAPHORES)
set(CMAKE_REQUIRED_LIBRARIES)
check_cxx_source_runs("
#include <sys/types.h>
#include <pwd.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>

int main(void)
{
  struct passwd sPasswd;
  struct passwd *pPasswd;
  char buf[1];
  int bufLen = sizeof(buf)/sizeof(buf[0]);
  int euid = geteuid();
  int ret = 0;

  errno = 0; // clear errno
  ret = getpwuid_r(euid, &sPasswd, buf, bufLen, &pPasswd);
  if (0 != ret)
  {
    if (ERANGE == errno)
    {
      return 0;
    }
  }

  return 1; // assume errno is NOT set for all other cases
}" GETPWUID_R_SETS_ERRNO)
check_cxx_source_runs("
#include <stdio.h>
#include <stdlib.h>

int main()
{
  FILE *fp = NULL;
  char *fileName = \"/dev/zero\";
  char buf[10];

  /*
   * Open the file in append mode and try to read some text.
   * And, make sure ferror() is set.
   */
  fp = fopen (fileName, \"a\");
  if ( (NULL == fp) ||
       (fread (buf, sizeof(buf), 1, fp) > 0) ||
       (!ferror(fp))
     )
  {
    return 0;
  }

  /*
   * Now that ferror() is set, try to close the file.
   * If we get an error, we can conclude that this
   * fgets() depended on the previous ferror().
   */
  if ( fclose(fp) != 0 )
  {
    return 0;
  }

  return 1;
}" FILE_OPS_CHECK_FERROR_OF_PREVIOUS_CALL)
set(CMAKE_REQUIRED_DEFINITIONS)

set(SYNCHMGR_SUSPENSION_SAFE_CONDITION_SIGNALING 1)
set(ERROR_FUNC_FOR_GLOB_HAS_FIXED_PARAMS 1)

if(CMAKE_SYSTEM_NAME STREQUAL Darwin)
  set(HAVE_COREFOUNDATION 1)
  set(HAVE__NSGETENVIRON 1)
  set(DEADLOCK_WHEN_THREAD_IS_SUSPENDED_WHILE_BLOCKED_ON_MUTEX 1)
  set(PAL_PTRACE "ptrace((cmd), (pid), (caddr_t)(addr), (data))")
  set(PAL_PT_ATTACH PT_ATTACH)
  set(PAL_PT_DETACH PT_DETACH)
  set(PAL_PT_READ_D PT_READ_D)
  set(PAL_PT_WRITE_D PT_WRITE_D)
  set(JA_JP_LOCALE_NAME ja_JP.SJIS)
  set(KO_KR_LOCALE_NAME ko_KR.eucKR)
  set(ZH_TW_LOCALE_NAME zh_TG.BIG5)
  set(HAS_FTRUNCATE_LENGTH_ISSUE 1)
elseif(CMAKE_SYSTEM_NAME STREQUAL FreeBSD)
  set(DEADLOCK_WHEN_THREAD_IS_SUSPENDED_WHILE_BLOCKED_ON_MUTEX 0)
  set(PAL_PTRACE "ptrace((cmd), (pid), (caddr_t)(addr), (data))")
  set(PAL_PT_ATTACH PT_ATTACH)
  set(PAL_PT_DETACH PT_DETACH)
  set(PAL_PT_READ_D PT_READ_D)
  set(PAL_PT_WRITE_D PT_WRITE_D)
  set(JA_JP_LOCALE_NAME ja_JP_LOCALE_NOT_FOUND)
  set(KO_KR_LOCALE_NAME ko_KR_LOCALE_NOT_FOUND)
  set(ZH_TW_LOCALE_NAME zh_TW_LOCALE_NOT_FOUND)
  set(HAS_FTRUNCATE_LENGTH_ISSUE 0)
  if(CMAKE_SYSTEM_PROCESSOR STREQUAL amd64)
    set(BSD_REGS_STYLE "((reg).r_##rr)")
  elseif(CMAKE_SYSTEM_PROCESSOR STREQUAL arm64)
    set(BSD_REGS_STYLE "((reg).rr)")
  else()
    message(FATAL_ERROR "Unsupported FreeBSD architecture")
  endif()

  if(EXISTS "/lib/libc.so.7")
    set(FREEBSD_LIBC "/lib/libc.so.7")
  else()
    message(FATAL_ERROR "Cannot find libc on this system.")
  endif()
else() # Anything else is Linux
  set(DEADLOCK_WHEN_THREAD_IS_SUSPENDED_WHILE_BLOCKED_ON_MUTEX 0)
  set(PAL_PTRACE "ptrace((cmd), (pid), (void*)(addr), (data))")
  set(PAL_PT_ATTACH PTRACE_ATTACH)
  set(PAL_PT_DETACH PTRACE_DETACH)
  set(PAL_PT_READ_D PTRACE_PEEKDATA)
  set(PAL_PT_WRITE_D PTRACE_POKEDATA)
  set(JA_JP_LOCALE_NAME ja_JP_LOCALE_NOT_FOUND)
  set(KO_KR_LOCALE_NAME ko_KR_LOCALE_NOT_FOUND)
  set(ZH_TW_LOCALE_NAME zh_TW_LOCALE_NOT_FOUND)
  set(HAS_FTRUNCATE_LENGTH_ISSUE 0)
endif(CMAKE_SYSTEM_NAME STREQUAL Darwin)

if(NOT NO_ICU OR EMBED_ICU)
  if(NOT HAVE_LIBICU_UCHAR_H)
    unset(HAVE_LIBICU_UCHAR_H CACHE)
    message(FATAL_ERROR "Cannot find ICU. Try installing libicu-dev or the appropriate packages for your platform. You may also disable icu/unicode with '--no-icu' argument")
  endif()
endif()
else() # ANDROID
  set(HAVE_IEEEFP_H 0)
  set(HAVE_ALLOCA_H 1)
  set(HAVE_SYS_VMPARAM_H 0)
  set(HAVE_PROCFS_H 0)
  set(HAVE_CRT_EXTERNS_H 0)
  set(HAVE_SYS_TIME_H 1)
  set(HAVE_PTHREAD_NP_H 0)
  set(HAVE_SYS_LWP_H 0)
  set(HAVE_RUNETYPE_H 0)
  set(HAVE_KQUEUE 0)
  set(HAVE_GETPWUID_R 1)
  set(HAVE_PTHREAD_SUSPEND 1)
  set(HAVE_PTHREAD_SUSPEND_NP 1)
  set(HAVE_PTHREAD_CONTINUE 1)
  # -DHAVE_PTHREAD_RESUME_NP 1
  # -DHAVE_PTHREAD_CONTINUE_NP 1
  # -DHAVE_PTHREAD_ATTR_GET_NP 1
  set(HAVE_PTHREAD_GETATTR_NP 1)
  set(HAVE_PTHREAD_SIGQUEUE 1)
  set(HAVE_SIGRETURN 1)
  set(HAVE__THREAD_SYS_SIGRETURN 1)
  set(HAVE_COPYSIGN 1)
  set(HAVE_FSYNC 1)
  set(HAVE_FUTIMES 0)
  set(HAVE_UTIMES 1)
  set(HAVE_SYSCTL 0)
  set(HAVE_SYSCONF 1)
  set(HAVE_LOCALTIME_R 1)
  set(HAVE_GMTIME_R 1)
  set(HAVE_TIMEGM 1)
  set(HAVE_POLL 1)
  set(HAVE_STATVFS 1)
  set(HAVE_THREAD_SELF 0)
  set(HAVE__LWP_SELF 0)
  set(HAVE_VM_ALLOCATE 0)
  set(HAVE_VM_READ 0)
  set(HAS_SYSV_SEMAPHORES 0)
  set(HAS_PTHREAD_MUTEXES 1)
  # -DHAVE_TTRACE
  set(HAVE_STAT_TIMESPEC 0)
  set(HAVE_STAT_NSEC 0)
  set(HAVE_TM_GMTOFF 1)
  # -DHAVE_PT_REGS 1)
  # -DHAVE_GREGSET_T
  set(HAVE_SIGINFO_T 1)
  set(HAVE_UCONTEXT_T 1)
  set(HAVE_PTHREAD_RWLOCK_T 1)
  set(HAVE_PRWATCH_T 0)
  set(HAVE_YIELD_SYSCALL 0)
  set(HAVE_INFTIM 0)
  set(HAVE_CHAR_BIT 1)
  # -DUSER_H_DEFINES_DEBUG 1
  set(HAVE__SC_PHYS_PAGES 1)
  set(HAVE__SC_AVPHYS_PAGES 1)
  # -DREALPATH_SUPPORTS_NONEXISTENT_FILES
  # -DSSCANF_CANNOT_HANDLE_MISSING_EXPONENT
  # -DSSCANF_SUPPORT_ll
  # -DHAVE_LARGE_SNPRINTF_SUPPORT 1)
  # -DHAVE_BROKEN_FIFO_SELECT 1)
  # -DHAVE_BROKEN_FIFO_KEVENT 1)
  # -DHAS_FTRUNCATE_LENGTH_ISSUE
  # -DHAVE_SCHED_GET_PRIORITY 1)
  # -DHAVE_SCHED_GETCPU 1)
  set(HAVE_WORKING_GETTIMEOFDAY 1)
  set(HAVE_WORKING_CLOCK_GETTIME 1)
  set(HAVE_CLOCK_MONOTONIC 1)
  set(HAVE_CLOCK_MONOTONIC_COARSE 1)
  set(HAVE_MACH_ABSOLUTE_TIME 0)
  set(HAVE_CLOCK_THREAD_CPUTIME 1)
  # -DSTATVFS64_PROTOTYPE_BROKEN
  # -DHAVE_MMAP_DEV_ZERO
  # -DMMAP_IGNORES_HINT
  # -DMMAP_ANON_IGNORES_PROTECTION
  set(MMAP_DOESNOT_ALLOW_REMAP 1)
  # -DONE_SHARED_MAPPING_PER_FILEREGION_PER_PROCESS
  # -DPTHREAD_CREATE_MODIFIES_ERRNO 1)
  # -DSEM_INIT_MODIFIES_ERRNO 1)
  # -DHAVE_PROCFS_CTL
  set(HAVE_COMPATIBLE_ACOS 1)
  set(HAVE_COMPATIBLE_ASIN 1)
  set(HAVE_COMPATIBLE_ATAN2 1)
  set(HAVE_COMPATIBLE_EXP 1)
  set(HAVE_COMPATIBLE_LOG 1)
  set(HAVE_COMPATIBLE_LOG10 1)
  set(UNGETC_NOT_RETURN_EOF 1)
  set(HAS_POSIX_SEMAPHORES 1)
  # -DGETPWUID_R_SETS_ERRNO
  # -DFILE_OPS_CHECK_FERROR_OF_PREVIOUS_CALL
  # -DPAL_THREAD_PRIORITY_MIN 0
  # -DPAL_THREAD_PRIORITY_MAX 0
  # -DDEADLOCK_WHEN_THREAD_IS_SUSPENDED_WHILE_BLOCKED_ON_MUTEX
  # -DSYNCHMGR_SUSPENSION_SAFE_CONDITION_SIGNALING
  # -DERROR_FUNC_FOR_GLOB_HAS_FIXED_PARAMS
  # -DHAS_FTRUNCATE_LENGTH_ISSUE
  # -DCHECK_TRACE_SPECIFIERS 0)
  # -DPROCFS_MEM_NAME=""
  # -DHAVE_GETHRTIME 1)
  set(HAVE_LOWERCASE_ISO_NAME 1)
  set(HAVE_READ_REAL_TIME 1)
  set(HAVE_UNDERSCORE_ISO_NAME 0)
  set(MKSTEMP64_IS_USED_INSTEAD_OF_MKSTEMP 0)
  set(NEED_DLCOMPAT 0)
  # set(OPEN64_IS_USED_INSTEAD_OF_OPEN 0)
  set(SELF_SUSPEND_FAILS_WITH_NATIVE_SUSPENSION 0)
  set(SET_SCHEDPARAM_NEEDS_PRIVS 0)
  set(SIGWAIT_FAILS_WHEN_PASSED_FULL_SIGSET 0)
  set(SYNCHMGR_PIPE_BASED_THREAD_BLOCKING 0)
  set(WRITE_0_BYTES_HANGS_TTY 0)
  set(HAVE_FTRUNCATE_LARGE_LENGTH_SUPPORT 1)
  set(HAVE_MACH_EXCEPTIONS 0)
  set(DEADLOCK_WHEN_THREAD_IS_SUSPENDED_WHILE_BLOCKED_ON_MUTEX 0)
  set(PAL_PTRACE "ptrace((cmd), (pid), (void*)(addr), (data))")
  set(PAL_PT_ATTACH PTRACE_ATTACH)
  set(PAL_PT_DETACH PTRACE_DETACH)
  set(PAL_PT_READ_D PTRACE_PEEKDATA)
  set(PAL_PT_WRITE_D PTRACE_POKEDATA)
  set(JA_JP_LOCALE_NAME ja_JP_LOCALE_NOT_FOUND)
  set(KO_KR_LOCALE_NAME ko_KR_LOCALE_NOT_FOUND)
  set(ZH_TW_LOCALE_NAME zh_TW_LOCALE_NOT_FOUND)
  set(HAS_FTRUNCATE_LENGTH_ISSUE 0)
endif()
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/pal/src/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/pal/src/config.h)
