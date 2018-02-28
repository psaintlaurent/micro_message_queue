#include <inttypes.h>
#include <mqueue.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "mmq.h"

mqd_t mq;
struct mq_attr attr;
struct sigaction s_action_close;
struct sigevent msg_received_event;
struct configuration config;
struct registered_queue rqueues[MAX_NUM_QUEUES];

int process_message(struct job *buffer_message) { }
void handle_error(char msg[]) { perror(msg); exit( EXIT_FAILURE ); }

int load_configuration() {

    FILE *fp;
    char *buf;
    char *kptr;
    char *vptr;
    int output=1;

    buf = malloc(MAX_CONFIG_LINE_LENGTH);
    fp = fopen(CONFIGURATION_FILE, "r");
    
    if(fp) {

        while((fgets(buf, MAX_CONFIG_LINE_LENGTH, fp)) != NULL) {

            kptr = strtok(buf, "= ");
            vptr = strtok(NULL, "= ");
            if(strcmp(kptr, "maximum_message_size") == 0) { config.maximum_message_size = strtol(vptr, NULL, 10); continue; }
            if(strcmp(kptr, "maximum_messages") == 0) { config.maximum_messages = strtol(vptr, NULL, 10); continue; }
            if(strcmp(kptr, "queue_file") == 0) { strncpy(config.queue_file, vptr, strlen(vptr)-1); continue; }
            if(strcmp(kptr, "sleep_time_in_sec") == 0) { config.sleep_time_in_sec = strtol(vptr, NULL, 10); continue; }
            output = -1; break;
        }
        
    	fclose(fp);
    } else { output = -1; }

    return output;
}


int load_registered_queues() {

    FILE *fp;
    char *buf;
    int output=1;
    int i=0;
    
    buf = malloc(MAX_CONFIG_LINE_LENGTH);
    fp = fopen(config.queue_file, "r");

    if(fp) {
    	
        while((fgets(buf, MAX_CONFIG_LINE_LENGTH, fp)) != NULL) {
        
            rqueues[i].name = strtok(buf, "\t");
            rqueues[i].job = strtok(NULL, "\t");
        }
        fclose(fp);
    } else { output = -1; }

    return output;
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
    
    FILE *queue_file;
    
    if(load_configuration() == -1) { exit(1); }
    if(load_registered_queues() == -1) { exit(1); }

	/* TODO: Iterate through the registered queues and open each one creating a struct for storing attributes. */
    attr.mq_flags = O_NONBLOCK;
    attr.mq_maxmsg = config.maximum_messages;
    attr.mq_msgsize = config.maximum_message_size;

    mq = mq_open(QUEUE_NAME, O_CREAT | O_RDWR, 0644, &attr);
    
    /* Initialize the sigaction attributes for redefining sigaction handler.  */
    s_action_close.sa_handler = close_queues;
    sigaction(SIGINT, &s_action_close, NULL);
    sigaction(SIGHUP, &s_action_close, NULL);
    sigaction(SIGILL, &s_action_close, NULL);

    while(1) {

        if (attr.mq_curmsgs > 0) { consume_messages(); }
        sleep( config.sleep_time_in_sec );
    }

    return 0;
}
