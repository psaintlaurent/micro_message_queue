#include <sys/types.h>

#define MAX_LINE_LENGTH 512
#define MAX_CONFIG_LINE_LENGTH 40
#define MAX_MESSAGES 100000
#define MAX_MESSAGE_SIZE 8192
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
    float float_value;
    char *string_value;
};

/* Queues loaded from registered_queues.conf */
struct registered_queues {
    
    unsigned int queue_id;
    char name[20];
    char associated_job[20];
};

struct job {
    
    unsigned int id;
    char type[5];
    char name[20];
    union msg_parameter parameters[];
};

/* Queue configuration properties. */
struct configuration {

    int maximum_message_size;
    int maximum_messages;
    int sleep_time_in_sec;
    char *queue_file;
};
