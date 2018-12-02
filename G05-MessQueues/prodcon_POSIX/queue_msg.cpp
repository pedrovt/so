/*
 *  @brief A simple queue, whose elements are pairs of integers,
 *      one being the id of the producer and the other the value produced
 *
 * @remarks safe, non busy waiting version
 *
 *  The following operations are defined:
 *     \li insertion of a value
 *     \li retrieval of a value.
 *
 * \author (2016) Artur Pereira <artur at ua.pt>
 * \author (2018) Pedro Teixeira (now using POSIX libraries)
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/msg.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <mqueue.h>

#include "queue.h"

/*
 *  \brief Shared data structure.
 */
typedef struct Item
{
    unsigned int id;     ///< id of the producer
    unsigned int value;  ///< value stored
} Item;

typedef struct Message
{
    long type;
    Item item;
} Message;

/** \brief internal storage region of queue type */
static mqd_t msgid;

/** \brief basic config of the queue */
//static const char* queueName = "/queue";    //or cast to (char*)
#define MSQ_NAME         "/queue"
#define MSQ_MAX_MSG_LEN  sizeof(Item) 
#define MSQ_MAX_MSG_NUM  100


/* ************************************************* */

/* Initialization of the communication channel */
void queueCreate(void)
{
    /* create the message queue */
    struct mq_attr attributes;
    attributes.mq_flags   = 0;
    attributes.mq_curmsgs = 100;
    attributes.mq_msgsize = MSQ_MAX_MSG_LEN;
    attributes.mq_maxmsg  = MSQ_MAX_MSG_NUM;

    msgid = mq_open(MSQ_NAME, O_RDWR | O_CREAT | O_EXCL, 0600, attributes);

    if (msgid == -1)
    {
        perror("Fail creating message queue");
        exit(EXIT_FAILURE);
    }

    /* for debug purposes */
    struct mq_attr read;
    if (mq_getattr(msgid, &read) == -1)
    {
        perror("Error retrieving attributes");
        exit(EXIT_FAILURE);
    }
    printf("\nITEM SIZE: %ld\n", sizeof(Item));
    printf("MAX # MESSAGES : %ld\n", read.mq_maxmsg);
    printf("MSG SIZE : %ld\n\n", read.mq_msgsize);
    // results are MAX # MESSAGES : 10 and MSG SIZE: 8192 ?? 
}

/* ************************************************* */

void queueConnect()
{
    /* ask OS to open the message queue */
    int open_res = mq_open(MSQ_NAME, O_RDWR);
    if (open_res == -1) {
        perror("Fail connecting to message queue");
        exit(EXIT_FAILURE);
    }
}

/* ************************************************* */

void queueDisconnect()
{
    /* ask OS to close the message queue */
    int close_res = mq_close(msgid);
    if (close_res == -1) {
        perror("Fail closing message queue");
        exit(EXIT_FAILURE);
        // todo unlink?
    }
}

/* ************************************************* */

void queueDestroy()
{
    /* ask OS to destroy the message queue */
    int unlink_res = mq_unlink(MSQ_NAME);
    if (unlink_res == -1) {
        perror("Fail unlinking message queue");
        exit(EXIT_FAILURE);
    }

    msgid = -1;
}

/* ************************************************* */

/* Insertion of a pait <id, value> into the message queue  */
void queueSend(unsigned int id, unsigned int value)
{
    /* construct message to be sent */
    Item msg = {id, value};

    /* send the message */
    int send_res = mq_send(msgid, (char*) &msg, sizeof(Item), 1);
    if (send_res == -1) {
        perror("Fail inserting message into the message queue");
        exit(EXIT_FAILURE);
    }
}

/* ************************************************* */

/* Retrieval of a pair <id, value> from the queue */

void queueRetrieve (unsigned int * idp, unsigned int * valuep)
{
    /* get message from queue */
    Item msg;
    
    /* receive the message */
    int receive_res = mq_receive(msgid, (char*) &msg, sizeof(Item), NULL);
    if (receive_res == -1) {
        perror("Fail retrieving message from the message queue");
        exit(EXIT_FAILURE);
    }

    /* return values */
    *idp    = msg.id;
    *valuep = msg.value;
}

/* ************************************************* */

