/*
 * A race conditions free implementation of module inc_mod
 *
 * \author (2016) Artur Pereira <artur at ua.pt>
 * \author (2018) Pedro Teixeira (POSIX libraries)
 */

#include "bwdelay.h"

#include <stdlib.h>
#include <stdio.h>

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h> 
#include <fcntl.h>    
#include <semaphore.h>
#include <unistd.h>

const long key = 0x1111L;

/* time delay length */
#define INC_TIME 500

/* Internal data structure */
typedef struct
{
    int var1, var2;
    sem_t* semid;
} SharedDataType;

static int shmfd = -1;
static SharedDataType * p = NULL;

/* configurations */
#define SHM_NAME "/inc_shm"
#define SHM_PERM 0600
#define SHM_LEN  sizeof(SharedDataType)

#define SEM_NAME "/inc_sem"
#define SEM_PERM 0600

/* manipulation functions */

static void lock()
{
    if (sem_wait(p->semid) == -1)
    {
        perror("lock");
        exit(EXIT_FAILURE);
    }
}

static void unlock()
{
    if (sem_post(p->semid) == -1)
    {
        perror("unlock");
        exit(EXIT_FAILURE);
    }
}

void v_create()
{
    /* create the shared memory */
    shmfd = shm_open(SHM_NAME, O_CREAT | O_EXCL | O_RDWR, SHM_PERM);
    if (shmfd == -1)
    {
        perror("Fail creating shared data");
        exit(EXIT_FAILURE);
    }

    int resTruncate = ftruncate(shmfd, SHM_LEN);
    if (resTruncate == -1)
    {
        perror("Fail truncating shared data");
        exit(EXIT_FAILURE);
    }

    /* attach shared memory to process addressing space */ 
    p = (SharedDataType*) mmap(NULL, sizeof(SharedDataType), PROT_READ | PROT_WRITE | PROT_EXEC, MAP_SHARED, shmfd, 0);
    if (p == (void*)-1)
    {
        perror("Fail connecting (first time) to shared data");
        exit(EXIT_FAILURE);
    }

    /* create access locker */
    p -> semid = sem_open (SEM_NAME, O_CREAT | O_EXCL | O_RDWR, SEM_PERM, 0); 
    if (p->semid == SEM_FAILED)
    {
        perror("Fail creating locker semaphore");
        exit(EXIT_FAILURE);
    }

    /* unlock shared data structure */
    /* to increment the semaphore */
    unlock();

    /* detach shared memory from process addressing space */
    int unmap_res = munmap(p, sizeof(SharedDataType));
    p = NULL;
    if (unmap_res == - 1)
    {
        perror("Fail detaching shared memory from process addressing space.");
        exit(EXIT_FAILURE);
    }
}

void v_connect()
{
    /* get access to the shared memory */
    shmfd = shm_open(SHM_NAME, O_RDWR, SHM_PERM);
    if (shmfd == -1)
    {
        perror("Fail connecting to shared data");
        exit(EXIT_FAILURE);
    }

    /* attach shared memory to process addressing space */ 
    p = (SharedDataType *)mmap(NULL, sizeof(SharedDataType), PROT_READ | PROT_WRITE, MAP_SHARED, shmfd, 0);
    if (p == (void*)-1)
    {
        perror("Fail connecting to shared data");
        exit(EXIT_FAILURE);
    }
}

void v_destroy()
{
    /* destroy the locker semaphore */
    int sem_unlink_res = sem_unlink(SEM_NAME);
    if (sem_unlink_res == -1)
    {
        perror("Fail destroy the locker semaphore.");
        exit(EXIT_FAILURE);
    }
    
    /* detach shared memory from process addressing space */
    int unmap_res = munmap(p, sizeof(SharedDataType));
    p = NULL;
    if (unmap_res == -1)
    {
        perror("Fail detaching shared memory from process addressing space.");
        exit(EXIT_FAILURE);
    }

    /* ask OS to destroy the shared memory */
    int shm_unlink_res = shm_unlink(SHM_NAME);
    if (shm_unlink_res == -1)
    {
        perror("Fail detaching shared memory from process addressing space.");
        exit(EXIT_FAILURE);
    }
    shmfd = -1;
}

/* set shared data with new values */
void v_set(int value1, int value2)
{
    lock();

	p->var1 = value1;
	p->var2 = value2;
    
    unlock();
}

/* get current values of shared data */
void v_get(int * value1p, int * value2p)
{
    lock();

	*value1p = p->var1;
	*value2p = p->var2;

    unlock();
}

/* increment both variables of the shared data */
void v_inc(void)
{
    lock();
    
    /* variables are incremented as in the unsafe version, but without race conditions */
    int v1 = p->var1 + 1;
    bwRandomDelay(INC_TIME);
    p->var1 = v1;

    p->var2++;

    unlock();
}

