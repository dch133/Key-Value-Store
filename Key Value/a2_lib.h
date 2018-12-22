#ifndef __A2_LIB_HEADER__
#define __A2_LIB_HEADER__

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <time.h>
#include <string.h>
#include <stdbool.h>

/* -------------------------------------
	Define your own globals here
----------------------------------------*/

#define __KV_WRITERS_SEMAPHORE__	"WRITER_DCHERNIS_260707258"
#define __KV_READERS_SEMAPHORE__	"READER_DCHERNIS_260707258" 
#define __KV_STORE_NAME__			"KV_STORE_DCHERNIS_260707258"
#define KV_EXIT_SUCCESS				0
#define KV_EXIT_FAILURE				-1
#define NUM_PODS	256
#define NUM_VALUES	256
#define NUM_KEYS	32
#define __KEY_VALUE_STORE_SIZE__	 sizeof(STORE)

/* ---------------------------------------- */


typedef struct VALUE 
{
    char value[256]; 
} VALUE;


typedef struct KEY_PAIR 
{
	int headIndex; //index for oldest value added
    int currentReadValue; //index of the value to be read
    char key[32]; 
    VALUE values[NUM_VALUES]; 
} KEY_PAIR;

typedef struct POD 
{
    int num_readers;
    int headIndex;  //index for oldest key added
    KEY_PAIR kvPairs[NUM_KEYS]; 
} POD;

typedef struct KV_STORE 
{ 
    POD pods[NUM_PODS]; 
} STORE;


unsigned long generate_hash(unsigned char *str);

int kv_store_create(char *kv_store_name);
int kv_store_write(char *key, char *value);
char *kv_store_read(char *key);
char **kv_store_read_all(char *key);

/*Helper methods*/
void initialiseStore(STORE* store);
int insertKV(STORE* store, char* key, char* value);
char* findKV(STORE* store, char* key);
char** findAllKV(STORE* store, char* key);

void signalSemaphore(sem_t *W_Sem, sem_t *R_Sem, POD* pod);
#endif
