#include "comp310_a2_test.h"
#include "a2_lib.h"
#include <stdio.h>
#define MAGIC_HASH_NUMBER 5381

unsigned long generate_hash(unsigned char *str) {
	int c;
    #ifndef SDBM
	unsigned long hash = MAGIC_HASH_NUMBER;
	while (c = *str++)
            hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
	
	#else
        unsigned long hash = 0;
        while (c = *str++)
            hash = c + (hash << 6) + (hash << 16) - hash;
	#endif
	return hash;
}

void generate_string(char buf[], int length){
    int type;
    for(int i = 0; i < length; i++){
        type = rand() % 3;
        if(type == 2)
            buf[i] = rand() % 26 + 'a';
        else if(type == 1)
            buf[i] = rand() % 10 + '0';
        else
            buf[i] = rand() % 26 + 'A';
    }
    buf[length - 1] = '\0';
}

void generate_unique_data(char buf[], int length, char **keys_buf, int num_keys){
    generate_string(buf, __TEST_MAX_DATA_LENGTH__);
    int counter = 0;
    for(int i = 0; i < num_keys; i++){
        if(strcmp(keys_buf[i], buf) == 0){
            counter++;
        }
    }
    if(counter > 1){
        generate_unique_data(buf, length, keys_buf, num_keys);
    }
    return;
}

void generate_key(char buf[], int length, char **keys_buf, int num_keys){
    generate_string(buf, __TEST_MAX_KEY_SIZE__);
    int counter = 0;
    for(int i = 0; i < num_keys; i++){
        if(strcmp(keys_buf[i], buf) == 0){
            counter++;
        }
    }
    if(counter > 1){
        generate_key(buf, length, keys_buf, num_keys);
    }
    return;

}

void initialiseStore(STORE* store)
{
    for (int i = 0; i < NUM_PODS; i++)
    {
        store->pods[i].headIndex = 0;
        store->pods[i].num_readers = 0;
        for (int j = 0; j < NUM_KEYS;j++)
        {
            store->pods[i].kvPairs[j].headIndex = 0;
            store->pods[i].kvPairs[j].currentReadValue = 0;
            memset(store->pods[i].kvPairs[j].key, 0, 32);                  //wipe old key data
            for(int k = 0; k < NUM_VALUES; k++)                           
            {
                memset(store->pods[i].kvPairs[j].values[k].value, 0, 256); //wipe old values
            }
        }
    }
}

int insertKV(STORE* store, char* key, char* value)
{
    //check if key and/or value is empty
    if((key == NULL) || (key[0] == '\0') || (value == NULL) || (value[0] == '\0'))
    {
        printf("ERROR: Key and/or Value is Null\n");
        return -1;
    }

    int status = 0; //return value

    unsigned long podIndex = generate_hash(key) % NUM_PODS;
    POD *pod = &(store->pods[podIndex]);

    //Lock the pod for writing
    sem_t *W_Sem;

    if ( (W_Sem = sem_open(__KV_WRITERS_SEMAPHORE__, O_RDWR | O_CREAT, 0644, 1)) == SEM_FAILED)
    {
        perror("Failed to open semaphore for Writer");
        status = -1;
    }
    sem_wait(W_Sem);

    //Loop to find the key or hit empty slot
    int keyIndex = 0;
    while (strcmp(pod->kvPairs[keyIndex].key, key) != 0 && strcmp(pod->kvPairs[keyIndex].key, "") != 0 && keyIndex < NUM_KEYS)
        keyIndex++;

    if (keyIndex < NUM_KEYS && strcmp(pod->kvPairs[keyIndex].key, key) == 0)  // if key exists
    {
        KEY_PAIR* currKey = &(pod->kvPairs[keyIndex]);

    //Loop to find next available spot to put new value or reach end of 
        int valueIndex = 0;  
        while (valueIndex < NUM_VALUES && strcmp(currKey->values[valueIndex].value, "") != 0)
            valueIndex++; 

        if (valueIndex < NUM_VALUES ) //add value at empty spot if in bounds                                  
             memcpy(currKey->values[valueIndex].value, value ,255);

        else // space full, evict oldest value
        {
            int oldest = currKey->headIndex;
            currKey->headIndex = (oldest + 1) % NUM_VALUES;                 //update head index to next oldest
            memset(currKey->values[oldest].value, 0, 256);                  //wipe old data
            memcpy(currKey->values[oldest].value, value ,255);    //put new data
        }
    }

    else if (keyIndex < NUM_KEYS && strcmp(pod->kvPairs[keyIndex].key, "") == 0) //if key doesn't exist but there is space in the pod
    {               
        memcpy(pod->kvPairs[keyIndex].key, key ,31);                 //put new key
        memcpy(pod->kvPairs[keyIndex].values[0].value, value ,255); //put new value
    }

    else //remove previous key in fifo manner and overwrite it (if input key doesn't exist)
    {
        int oldest = pod->headIndex;
        pod->headIndex = (pod->headIndex++) % NUM_KEYS;         //update head index to next oldest key
        memset(pod->kvPairs[oldest].key, 0, 32);                //wipe old key
        for(int i = 0; i < NUM_VALUES;i++)                      //wipe old values
            memset(pod->kvPairs[oldest].values[i].value, 0, 256);    
        
        memcpy(pod->kvPairs[oldest].key, key ,31);
        memcpy(pod->kvPairs[oldest].values[0].value, value ,255);
        pod->kvPairs[oldest].headIndex = 0;                     //set the head index in new value array
        pod->kvPairs[oldest].currentReadValue = 0;              //set the readint pointer index in new value array
    }

    //Signal semaphore:  release lock for writers and readers
    if (sem_post(W_Sem) == -1)
    {
        perror("Failed to signal semaphore for Writer");
        status = -1;
    }

    sem_close(W_Sem);
    return status;    
}

char* findKV(STORE* store, char* inputKey)
{
    //check if key is valid
    if(inputKey == NULL || inputKey[0] == '\0') 
    {
        printf("Error: Invalid Key\n");
        return NULL;
    }

    //truncate Key to fit the space available
    char * key = (char*) malloc(sizeof(char)*32);
    strncpy(key, inputKey, 31);

    char* returnedValue = (char*) calloc(256,sizeof(char)); //create a copy (null) of value to be returned
    unsigned long podIndex = generate_hash(key) % NUM_PODS;

    POD *pod = &(store->pods[podIndex]);

    /*--------------------------------------------------------------------------------*/
    //Lock the pod
    sem_t *W_Sem, *R_Sem; 
    if ( (W_Sem = sem_open(__KV_WRITERS_SEMAPHORE__,O_RDWR | O_CREAT, 0777, 1)) == SEM_FAILED)
    {
        perror("Failed to open semaphore for Writer");
        return NULL;
    }
    if ( (R_Sem = sem_open(__KV_READERS_SEMAPHORE__,O_RDWR | O_CREAT, 0777, 1)) == SEM_FAILED)
    {
        perror("Failed to open semaphore for Reader");
        return NULL;
    }
    sem_wait(R_Sem);
    pod->num_readers++;
    if (pod->num_readers == 1) // 1st reader to lock out the writer
        sem_wait(W_Sem);

    //unlock the reader semaphore so others can read
    if (sem_post(R_Sem) == -1)
        perror("Failed to signal semphore mySem");
    /*--------------------------------------------------------------------------------*/

    //Loop to find the key or hit empty slot
    int keyIndex = 0;
    while (strcmp(pod->kvPairs[keyIndex].key, key) != 0 && strcmp(pod->kvPairs[keyIndex].key, "") != 0 && keyIndex < NUM_KEYS)
        keyIndex++;
    if (keyIndex == NUM_KEYS || strcmp(pod->kvPairs[keyIndex].key, "") == 0)
    {
        printf("Error: Key not found in KV-Store\n");
        signalSemaphore(W_Sem, R_Sem, pod);
        return NULL; 
    }
    else
    {
        KEY_PAIR* currKey = &(pod->kvPairs[keyIndex]);

        int currentReadIndex = currKey->currentReadValue;
        if (strcmp(currKey->values[currentReadIndex].value, "") == 0) //if empty string (end of value list) go back to the 1st value;
        {
            currKey->currentReadValue = 1; //move back to the beginning (read the 1st value)
            strcpy(returnedValue, currKey->values[0].value); //copy value
            signalSemaphore(W_Sem, R_Sem, pod);
            return returnedValue;
        }
        strcpy(returnedValue, currKey->values[currentReadIndex].value); //copy value if found
        currKey->currentReadValue = (currKey->currentReadValue + 1) % NUM_VALUES;//Update the read index to the next in line        
        
        signalSemaphore(W_Sem, R_Sem, pod);
        return returnedValue;
    }
}

char** findAllKV(STORE* store, char* key)
{
    //check if key is valid
    if(key == NULL || key[0] == '\0') 
    {
        printf("Error: Invalid Key\n");
        return NULL;
    }

    unsigned long podIndex = generate_hash(key) % NUM_PODS;
    POD *pod = &(store->pods[podIndex]);

    /*--------------------------------------------------------------------------------*/
    //Lock the pod
    sem_t *W_Sem, *R_Sem; 
    if ( (W_Sem = sem_open(__KV_WRITERS_SEMAPHORE__,O_RDWR | O_CREAT, 0777, 1)) == SEM_FAILED)
    {
        perror("Failed to open semaphore for Writer");
        return NULL;
    }
    if ( (R_Sem = sem_open(__KV_READERS_SEMAPHORE__,O_RDWR | O_CREAT, 0777, 1)) == SEM_FAILED)
    {
        perror("Failed to open semaphore for Reader");
        return NULL;
    }
    sem_wait(R_Sem);

    pod->num_readers++;
    if (pod->num_readers == 1) // 1st reader to lock out the writer
        sem_wait(W_Sem);

    //unlock the reader semaphore so others can read
    if (sem_post(R_Sem) == -1)
        perror("Failed to signal semphore mySem");

    /*--------------------------------------------------------------------------------*/    

    //Loop to find the key or hit empty slot
    int keyIndex = 0;
    while (strcmp(pod->kvPairs[keyIndex].key, key) != 0 && strcmp(pod->kvPairs[keyIndex].key, "") != 0)
        keyIndex++;
    if (strcmp(pod->kvPairs[keyIndex].key, "") == 0)
    {
        signalSemaphore(W_Sem, R_Sem, pod);
        return NULL;     
    }

    KEY_PAIR* currKey = &(pod->kvPairs[keyIndex]);

    //read from start to end until reach null starting to reread the same value
    int i = 0;
    char** allValues = (char **) malloc(sizeof(char *) * NUM_VALUES);
    //initialise return value to null
    for(int i=0; i< NUM_VALUES; i++)
        allValues[i] = (char *) calloc(256, sizeof(char));
    while (i <= NUM_VALUES && strcmp(currKey->values[i].value, "") != 0) //calculate the number of values to  copy
        i++;

    int end = i;
    
    signalSemaphore(W_Sem, R_Sem, pod);

    //copy and truncate Values to fit the space available
    char** returnedValues = (char **) malloc(sizeof(char *) * NUM_VALUES+1);
    for(i = 0; i < end; i++)
    {
        returnedValues[i] = (char *) calloc(256, sizeof(char));
        memcpy(returnedValues[i], currKey->values[i].value, 255);
    }
    return returnedValues;    
}

void signalSemaphore(sem_t *W_Sem, sem_t *R_Sem, POD* pod)
{   
    sem_wait(R_Sem); //lock to block multiple changes to counter
    
    pod->num_readers--; //signal() the semaphore: decrement the semaphore
    if (pod->num_readers == 0) // last reader unlocks the writer
    {
        sem_post(W_Sem);         
        sem_close(W_Sem);
    }
    if (sem_post(R_Sem) == -1) // unlock the access to reader counter
        perror("Failed to signal semphore mySem");

    sem_close(R_Sem);
    
}


