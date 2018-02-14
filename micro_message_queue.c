#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <mqueue.h>

#define CONFIGURATION_FILE "/etc/micro_message_queue.conf"
#define LOG_FILE "/var/log/micro_message_queue.conf"
#define MAX_MESSAGE_SIZE 8192
#define QUEUE_NAME "/test_queue"
#define SLEEP_TIME 2

mqd_t mq;
struct mq_attr attr;
struct sigaction s_action_close;
struct sigaction s_action_msg_received;
struct sigevent msg_received_event;

int process_message(char *buf) { } 
void handle_error(char msg[]) { perror(msg); exit(EXIT_FAILURE); }

/* 
TODO: Log the closing of the queues and modify to check for ANY registered queue in the configuration file so they can be cleaned up.
*/
void close_queues(int signal_number) {
    
    mq_close(mq);
    mq_unlink(QUEUE_NAME);
    exit(EXIT_SUCCESS);
}

/* 
TODO: Modify existing buffer.
Read messages from the buffer until there are no more messages or an 
external signal interrupts the process.  Use a union so that can handle multiple data types.
*/
void consume_messages() 
{
    char *buf;
    ssize_t bytes_read;
    buf = malloc(attr.mq_msgsize);

    if(mq_getattr(mq, &attr) == -1) { handle_error("Failed to get message queue attributes."); }

    while(attr.mq_curmsgs > 0) {

    	if(buf == NULL) { handle_error("Buffer memory allocation failed."); }

        while(bytes_read < attr.mq_msgsize) {
    
            bytes_read = mq_receive(mq, buf, attr.mq_msgsize, NULL);
            if(bytes_read == -1) { handle_error("Failed to read current message.");  }
        }
	process_message(buf);
    }

    free(buf);
}

int main(int argc, char **argv)
{
    /* TODO: Read list of queue names and their associated actions from a file. */

    /* Initialize the queue attributes. */
    attr.mq_flags = O_NONBLOCK;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = MAX_MESSAGE_SIZE;
    attr.mq_curmsgs = 0;

    mq = mq_open(QUEUE_NAME, O_CREAT | O_RDWR, 0644, &attr);

    /* Initialize the sigaction attributes for redefining sigaction handler.  */
    s_action_close.sa_handler = close_queues;

    sigaction(SIGINT, &s_action_close, NULL);
    sigaction(SIGHUP, &s_action_close, NULL);
    sigaction(SIGILL, &s_action_close, NULL);

    while(1) {
        if (attr.mq_curmsgs > 0) { consume_messages(); } 
        sleep(SLEEP_TIME); 
    }

    return 0;
}
