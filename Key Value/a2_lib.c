#include "a2_lib.h"
#include "a2_lib.h"


int kv_store_create(char *kv_store_name)
{   /*
    - open shared memory segment
    - ftruncate() to set the KVStore size
    - mmap() to process virtual memory
    - initialize bookkeeping information
    */
    int fd = shm_open(kv_store_name, O_CREAT|O_RDWR/*|O_EXCL*/, S_IRWXU);
	if (fd == -1)
    {
        perror("Error opening or creating shared memory\n");
    }

    ftruncate(fd, __KEY_VALUE_STORE_SIZE__); //resize the shared memory object to fit the KV-Store

    STORE *mappedAddr = (STORE *) mmap(NULL, __KEY_VALUE_STORE_SIZE__, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    initialiseStore(mappedAddr);             
	close(fd);
	if (munmap(mappedAddr, __KEY_VALUE_STORE_SIZE__) == -1) return -1;
    
    //create semaphores
    sem_t *W_Sem = sem_open(__KV_WRITERS_SEMAPHORE__, O_RDWR | O_CREAT, 0644, 10);
    if (W_Sem == SEM_FAILED)
    {
        perror("Failed to create semaphore for Writer");
        return -1;
    }
    sem_t *R_Sem = sem_open(__KV_READERS_SEMAPHORE__, O_RDWR | O_CREAT, 0644, 10);
    if (R_Sem == SEM_FAILED)
    {
        perror("Failed to create semaphore for Reader");
        return -1;
    }
    return 0;
}

int kv_store_write(char *key, char *value)
{	/*
    - open shared memory segment
    - mmap() to process virtual memory
    - calculate hash of key (if using hash)
    - store the K-V pair in the right location
    - many values are possible a for single key (store all of them)
    */
	
	int fd = shm_open(__KV_STORE_NAME__, O_RDWR, 0);
    if (fd < 0)
	{
       perror("Error opening shm\n");
	   return -1;
	}
    //map the shared memory object (KV-STORE) starting at *mappedAddr
    STORE* mappedAddr = mmap(NULL, __KEY_VALUE_STORE_SIZE__, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    int status = insertKV(mappedAddr, key, value);
    
    //close fd and free memory
	close(fd);
	if (munmap(mappedAddr, __KEY_VALUE_STORE_SIZE__) == -1) return -1;	//free memory

	return status; //returns -1 if failed to insert
}

char *kv_store_read(char *key)
{   /*
    - ensure exclusive LOCK over writers, but readers can join in
    - check if the key exists in the KV-Store
    - if key not found before returning release LOCKS (cleanup)
    - if there are many records: keep returning the next one
    */

	int fd = shm_open(__KV_STORE_NAME__, O_RDWR, 0);
    if (fd < 0)
	{
       perror("Error opening shm\n");
	   return NULL;
	}
 
    //map only the portion from fstat() into the virtual address space
    STORE *mappedAddr = mmap(NULL, __KEY_VALUE_STORE_SIZE__, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

	char* readValue = findKV(mappedAddr, key); //read the value
    
	close(fd);
	if (munmap(mappedAddr, __KEY_VALUE_STORE_SIZE__) == -1) return NULL;	

	return readValue;
}

char **kv_store_read_all(char *key)
{
	int fd = shm_open(__KV_STORE_NAME__, O_RDWR, 0);
    if (fd < 0)
	{
       perror("Error opening shm\n");
	   return NULL;
	}
	
    //map only the portion from fstat() into the virtual address space
    STORE *mappedAddr = mmap(NULL, __KEY_VALUE_STORE_SIZE__, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

	char** readValues = findAllKV(mappedAddr, key); //read the value
	    
	close(fd);
	if (munmap(mappedAddr, __KEY_VALUE_STORE_SIZE__) == -1)   return NULL;
	
	return readValues;
}



/* -------------------------------------------------------------------------------
	MY MAIN:: Use it if you want to test your implementation (the above methods)
	with some simple tests as you go on implementing it (without using the tester)
	-------------------------------------------------------------------------------

int main() {
	return EXIT_SUCCESS;
}
*/