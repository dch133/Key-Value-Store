/* Written by Xiru Zhu
 * This is for testing your assignment 2. 
 */
#ifndef __A2_TEST__
#define __A2_TEST__

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <semaphore.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <time.h>
#include <sys/mman.h>
#include "a2_lib.h"
//LEAVE MAX KEYS as twice the number of pods
#define __TEST_MAX_KEY__  256
#define __TEST_MAX_KEY_SIZE__ 31
#define __TEST_MAX_DATA_LENGTH__ 256
#define __TEST_MAX_POD_ENTRY__ 256
#define __TEST_SHARED_MEM_NAME__ "KV_STORE_DCHERNIS_260707258"
#define __TEST_SHARED_SEM_NAME__ "/ONLY_TEARS DCHERNIS_260707258"
#define __TEST_FORK_NUM__ 4
#define RUN_ITERATIONS 20

sem_t *open_sem_lock;
pid_t pids[__TEST_FORK_NUM__];

void kv_delete_db();
void kill_shared_mem();
void intHandler(int dummy);
void generate_string(char buf[], int length);
void generate_key(char buf[], int length, char **keys_buf, int num_keys);
void generate_unique_data(char buf[], int length, char **keys_buf, int num_keys);

#endif
