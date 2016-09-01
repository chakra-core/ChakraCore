//
// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information. 
//

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>

typedef int errno_t;
#define RANDOM_CACHE_SIZE 8
constexpr int rand_byte_len = sizeof(unsigned int) * RANDOM_CACHE_SIZE;
static union {
    char rstack[rand_byte_len];
    unsigned int result[RANDOM_CACHE_SIZE];
} random_cache;
static unsigned cache_index(RANDOM_CACHE_SIZE);

class Lock{
    static pthread_mutex_t random_generator_mutex;
public:
  __attribute__((noinline))
  static bool Init()
  {
      return pthread_mutex_init(&random_generator_mutex, NULL) == 0;
  } 

  __attribute__((noinline))
  Lock() {
     pthread_mutex_lock(&random_generator_mutex);
  }

  __attribute__((noinline))
  ~Lock() {
     pthread_mutex_unlock(&random_generator_mutex);
  }
};

pthread_mutex_t Lock::random_generator_mutex;

static bool GetRandom(unsigned int *result) noexcept
{
    static bool mutex_initialized = Lock::Init();
    // Do not put the `if check` into `Init` or replace it with Assert
    if(!mutex_initialized) {
      fprintf(stderr, "pthread_mutex_init has failed");
      abort();
    }
    Lock lock;

    if (cache_index < RANDOM_CACHE_SIZE) {
        *result = random_cache.result[cache_index++];
        return true;
    }

    const int rdev = open("/dev/urandom", O_RDONLY);
    unsigned len = 0;

    do
    {
        int added = read(rdev, random_cache.rstack + len, rand_byte_len - len);
        if (added < 0) {
            close(rdev);
            return false;
        }
        len += added;
    } while (len < rand_byte_len);

    close(rdev);
    *result = random_cache.result[0];
    cache_index = 1;
    return len == rand_byte_len;
}

extern "C"
errno_t __cdecl rand_s(unsigned int* randomValue) noexcept
{
    if (randomValue == nullptr) return 1;
    
    if (GetRandom(randomValue)) 
    {
        return 0;
    }
    else
    {
        *randomValue = 0;
        return 1;
    }
}
