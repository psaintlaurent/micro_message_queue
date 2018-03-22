#include <errno.h>
#include <inttypes.h>
#include "mmq.h"
#include <mqueue.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>


int rq_elem_count=0;
struct configuration config;
struct registered_queue rqueue[MAX_NUM_QUEUES];

void handle_error(char msg[]) { perror(msg); exit( EXIT_FAILURE ); }

int load_configuration() {

    FILE *fp;
    int output=1;
    char *buf, *kptr, *vptr;

    buf = malloc(MAX_CONFIG_LINE_LENGTH);
    fp = fopen(CONFIGURATION_FILE, "r");
    
    if(fp) {

        while((fgets(buf, MAX_CONFIG_LINE_LENGTH, fp)) != NULL) {
           
            kptr = strtok(buf, "= ");
            vptr = strtok(NULL, "= ");
            
            if(strcmp(kptr, "maximum_message_size") == 0) { config.maximum_message_size = strtol(vptr, NULL, 10); continue; }
            if(strcmp(kptr, "maximum_messages") == 0) { config.maximum_messages = strtol(vptr, NULL, 10); continue; }
            if(strcmp(kptr, "sleep_time_in_sec") == 0) { config.sleep_time_in_sec = strtol(vptr, NULL, 10); continue; }
            if(strcmp(kptr, "queue_file") == 0) {

                config.queue_file = malloc(strlen(vptr));
                strncpy(config.queue_file, vptr, strlen(vptr)-1);
                continue;
            }
            output = -1; break; 
        }
    	fclose(fp);

    } else { output = -1; }

    return output;
}

/* 
TODO: Use a union so that messages of multiple data types can be utilized.
*/
static void consume_messages(union sigval sv) {

    int i=0, mqn;
    void *buf;
    char *mqd_attr;
    ssize_t bytes_read;
    struct mq_attr attr;
    struct sigevent sev;
    mqd_t mq = *((mqd_t *) sv.sival_ptr);

    /* Read message queue as a file descriptor. */
    read(mq, mqd_attr, 1);
    while(mqd_attr != "\0") {

        mqd_attr++;
        read(mq, mqd_attr, 1);
    }

    if(mq_getattr(mq, &attr) == 0) {
        
	    while(attr.mq_curmsgs > 0) {
        
	        buf = malloc(attr.mq_msgsize);
	        if(buf == NULL) { handle_error("Buffer memory allocation failed."); }
 
	        while(bytes_read < attr.mq_msgsize) {
            
	            bytes_read = bytes_read + mq_receive(mq, buf, attr.mq_msgsize, NULL);
	            if(bytes_read == -1) { handle_error("Failed to read current message.");  }
	        }
	        process_message(&buf);
	    }
	    free(buf);

        /* Re-register the process for notifcations from this queue. */
        sev.sigev_notify = SIGEV_THREAD;
        /* It seems as if this is completely ignored for mq_notify */
        sev.sigev_signo = SA_SIGINFO; 
        sev.sigev_notify_function = consume_messages;
        sev.sigev_value.sival_ptr = &mq;
        mqn = mq_notify(mq, &sev);
        if(mqn == -1) { handle_error("Re-registering message queue."); }

    } else { handle_error("Failed to get message queue attributes."); return; }
}

/* 
Process the message with the first string being the path to the file to be executed. 
Everything else being the arguments passed to the file.

If I'm reading the documentation correctly the message queues should be destroyed during the execve run
I'll find out later.
*/
int process_message(char *buf) {

    int buffer_size = strlen(buf);
    int output = -1;
    

    if(buffer_size > 0 && buffer_size < MAX_LINE_LENGTH + 1) {

        char *args[] = { strtok(buf, " "), strtok(NULL, "\0") };
        char *env[] = { NULL };
        output = execve(args[0], args, env);
        if(output == -1) {
        
            printf("Error executing the command %s %s with %s.", args[0], args[1], strerror(errno));
        }
    } else { printf("Error executing command, invalid number of arguments."); }
    
    return output;
}

static int load_registered_queues() {

    FILE *fp;
    char *buf;
    struct rlimit rlm;
    int i=0, output=0;

    buf = malloc(MAX_CONFIG_LINE_LENGTH);
    fp = fopen(config.queue_file, "r");

    /* 

        Set the resource limit for the total size of all message queues 
        allowed for this process in bytes.
        This is necessary because creating message queues via mq_open leads to 
        the error, "EMFILE Too many open files." which would lead you to believe
        that the number of message queues or the the number of 
        open file descriptors was the issue which was not the case.
        Increasing RLIMIT_NOFILE does not address the issue but increasing RLIMIT_MSGQUEUE
        does.

        Setting this to infinity is probably not ideal but for now this will work.
    */
    rlm.rlim_cur = RLIM_INFINITY;
    rlm.rlim_max = RLIM_INFINITY;
    if(setrlimit(RLIMIT_MSGQUEUE, &rlm) == -1) {

        printf("Failed setting the resource limit for RLIMIT_MSGQUEUE with the error %s", strerror(errno));
        exit(1);
    }
	
    if(fp) {
    	
        while((fgets(buf, MAX_CONFIG_LINE_LENGTH, fp)) != NULL) {
 
            struct sigevent sv;
 
            rqueue[i].name = strtok(buf, "\t");
            rqueue[i].command = strtok(NULL, "\t");
            rqueue[i].attr.mq_flags = O_NONBLOCK;
            rqueue[i].attr.mq_maxmsg = config.maximum_messages;
            rqueue[i].attr.mq_msgsize = config.maximum_message_size;            
            /* These should eventually be opened read only. */
            rqueue[i].mq = mq_open(rqueue[i].name, O_CREAT|O_RDWR, 0644, &rqueue[i].attr); 

            if(rqueue[i].mq == (mqd_t) -1) { 
                printf("mq_open %s %d %d %s \n", 
                        rqueue[i].name, 
                        config.maximum_messages, 
                        config.maximum_message_size, 
                        strerror(errno)); 
                exit(1);
            }

            /* Setup sigevent struct for call to mq_notify. */
            sv.sigev_notify = SIGEV_THREAD;
            /* It seems as if this is completely ignored for mq_notify 
                since the function signature for the notification function doesn't change. */
            sv.sigev_signo = SA_SIGINFO;
            sv.sigev_notify_function = consume_messages;
            sv.sigev_value.sival_ptr = &rqueue[i].mq;
            mq_notify(rqueue[i].mq, &sv);
            buf = malloc(MAX_CONFIG_LINE_LENGTH); 
            i++;
        } 
    } else { output = -1; }

    rq_elem_count = i;

    return output;
}

static void unload_registered_queues() {

    int idx;
    for(idx=0;idx<rq_elem_count;idx++) { mq_unlink(rqueue[idx].name); }
}

int main(int argc, char **argv) {
    
    FILE *queue_file;
    struct sigaction s_action_close;

    if(load_configuration() == -1) { exit(1); }
    if(load_registered_queues() == -1) { exit(1); }
   
    /* Initialize the sigaction attributes for redefining sigaction handler. */
    s_action_close.sa_handler = unload_registered_queues;    
    sigaction(SIGINT, &s_action_close, NULL);
    sigaction(SIGHUP, &s_action_close, NULL);
    sigaction(SIGILL, &s_action_close, NULL);

    while(1) { sleep( config.sleep_time_in_sec ); }

    return 0;
}
