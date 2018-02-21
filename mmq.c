#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <mqueue.h>
#include "mmq.h"

#define TESTING 1

#if TESTING == 1
    #define CONFIGURATION_FILE "./mmh.conf"
    #define REGISTERED_QUEUES "./registered_queues.conf"
#else
    #define CONFIGURATION_FILE "/etc/mmh.conf"
    #define REGISTERED_QUEUES "/etc/registered_queues.conf"
#endif

#define MAX_LINE_LENGTH 512
#define MAX_MESSAGES 100000
#define MAX_MESSAGE_SIZE 8192
#define QUEUE_NAME "/test_queue"
#define SLEEP_TIME_IN_SEC 2

mqd_t mq;
struct mq_attr attr;
struct sigaction s_action_close;
struct sigevent msg_received_event;
struct configuration config;

int process_message(struct job *buffer_message) { }

void handle_error(char msg[]) { perror(msg); exit( EXIT_FAILURE ); }

int load_configuration() {
    
    FILE *fp;
    char *buf;
    int output;
    buf = malloc(MAX_LINE_LENGTH);
    
    if((fp = fopen(CONFIGURATION_FILE, "r")) != NULL) {

        output = -1;
    } else {
    
        while((buf = fgets(buf, sizeof(buf), fp)) != NULL) {
            
            /* load configuration data from CONFIGURATION_FILE */
        }
    }
}

/* 
TODO: Log the closing of the queues and modify to check for ANY registered queue in the configuration file so they can be cleaned up.
*/
void close_queues(int signal_number) {

    mq_close(mq);
    mq_unlink(QUEUE_NAME);
    exit( EXIT_SUCCESS );
}

/* 
TODO: Modify existing buffer.
Read messages from the buffer until there are no more messages or an 
external signal interrupts the process.  Use a union so that messages of multiple data types can be utilized.
*/
void consume_messages() {

    void *buf;
    ssize_t bytes_read;
    struct job buffer_message;
    buf = malloc(attr.mq_msgsize);
    
    
    if(mq_getattr(mq, &attr) == -1) { handle_error("Failed to get message queue attributes."); }
    
    while(attr.mq_curmsgs > 0) {

    	if(buf == NULL) { handle_error("Buffer memory allocation failed."); }

        while(bytes_read < attr.mq_msgsize) {
    
            bytes_read = mq_receive(mq, buf, attr.mq_msgsize, NULL);
            if(bytes_read == -1) { handle_error("Failed to read current message.");  }
        }
        process_message(&buffer_message);
    }

    free(buf);
}

int main(int argc, char **argv) {
    
    
    load_configuration();
    
    /* TODO: Read list of queue names and their associated actions from a file. */
    /* Initialize the queue attributes. */
    attr.mq_flags = O_NONBLOCK;
    attr.mq_maxmsg = MAX_MESSAGES;
    attr.mq_msgsize = MAX_MESSAGE_SIZE;

    mq = mq_open(QUEUE_NAME, O_CREAT | O_RDWR, 0644, &attr);
    /* Initialize the sigaction attributes for redefining sigaction handler.  */
    s_action_close.sa_handler = close_queues;
    sigaction(SIGINT, &s_action_close, NULL);
    sigaction(SIGHUP, &s_action_close, NULL);
    sigaction(SIGILL, &s_action_close, NULL);

    while(1) {

        if (attr.mq_curmsgs > 0) { consume_messages(); }
        sleep( SLEEP_TIME_IN_SEC );
    }

    return 0;
}
