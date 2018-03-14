#include <sys/types.h>

#define MAX_LINE_LENGTH 512
#define MAX_CONFIG_LINE_LENGTH 40
#define MAX_MSG_PER_JOB 20
#define MAX_MESSAGES 100000
#define MAX_MESSAGE_SIZE 8192
#define MAX_NUM_QUEUES 5000
#define QUEUE_NAME "/test_queue"
#define SLEEP_TIME_IN_SEC 2
#define TESTING 1

#if TESTING == 1
    #define CONFIGURATION_FILE "./mmq.conf"
    #define REGISTERED_QUEUES "./registered_queues.conf"
#else
    #define CONFIGURATION_FILE "/etc/mmq.conf"
    #define REGISTERED_QUEUES "/etc/registered_queues.conf"
#endif

union msg_parameter {
    
    int int_val;
    float float_val;
    char *string_val;
};

/* Queues loaded from registered_queues.conf */
struct registered_queue {

    char *name;
    char *command;
    mqd_t mq;
    struct mq_attr attr; 
};

struct buffer_msg {
 
    char *associated_queue;
    union msg_parameter parameters[MAX_MSG_PER_JOB];
};

/* Queue configuration properties. */
struct configuration {

    int maximum_message_size;
    int maximum_messages;
    int sleep_time_in_sec;
    char queue_file[20];
};
