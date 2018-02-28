#include <inttypes.h>
#include <mqueue.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "mmq.h"

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

static int load_registered_queues() {

    FILE *fp;
    char *buf;
    int output=0;
    int i=0;
    
    buf = malloc(MAX_CONFIG_LINE_LENGTH);
    fp = fopen(config.queue_file, "r");
	
    if(fp) {
    	
        while((fgets(buf, MAX_CONFIG_LINE_LENGTH, fp)) != NULL) {
        
            rqueues[i].name = strtok(buf, "\t");
            rqueues[i].command = strtok(NULL, "\t");
            rqueues[i].attr.mq_flags = O_NONBLOCK;
			rqueues[i].attr.mq_flags = config.maximum_messages;
			rqueues[i].attr.mq_flags = config.maximum_message_size;
			rqueues[i].mq = mq_open(rqueues[i].name, O_CREAT | O_RDWR, 0644, &rqueues[i].attr);  //These should eventually be opened read only
        }
        fclose(fp);
    } else { output = -1; }

    return output;
}

static void unload_registered_queues() {

	int i;
	int output=0;
	char *name;

	for(i=0;i<=sizeof(rqueues);i++) {

		if(output == 0) { output = mq_unlink(rqueues[i].name); } 
		else { break; }
	}
}

/* 
TODO: Use a union so that messages of multiple data types can be utilized.
*/
void consume_messages() {

	int i=0;
    void *buf;
    ssize_t bytes_read;
    struct job buffer_message;
    
    /* 
		Note to self, a few days into the future: The way this is coded *will* lead to queues being blocked under the right circumstances
		but I need to get a simple version going.  Ideally I'll use mq_notify so I don't need to cycle through all of the queues just the ones
		that are open.
    */
    
    for(i=0;i<=sizeof(rqueues);i++) {
    
    	while(rqueues[i].attr.mq_curmsgs > 0) {
    
			buf = malloc(rqueues[i].attr.mq_msgsize);

			if(buf == NULL) { handle_error("Buffer memory allocation failed."); }

		    while(bytes_read < rqueues[i].attr.mq_msgsize) {
		
		        bytes_read = mq_receive(rqueues[i].mq, buf, rqueues[i].attr.mq_msgsize, NULL);
		        if(bytes_read == -1) { handle_error("Failed to read current message.");  }
		    }
		    process_message(&buffer_message);
    	}
    	free(buf);
    }
}

int main(int argc, char **argv) {
    
    FILE *queue_file;
    
    if(load_configuration() == -1) { exit(1); }
    if(load_registered_queues() == -1) { exit(1); }

    /* Initialize the sigaction attributes for redefining sigaction handler.  */
    s_action_close.sa_handler = unload_registered_queues;
    sigaction(SIGINT, &s_action_close, NULL);
    sigaction(SIGHUP, &s_action_close, NULL);
    sigaction(SIGILL, &s_action_close, NULL);

	/* TODO: Have message consumption triggered by mq_notify. */
    while(1) {

        consume_messages();
        sleep( config.sleep_time_in_sec );
    }

    return 0;
}
