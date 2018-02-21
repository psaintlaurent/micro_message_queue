#include <sys/types.h>

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


/* Queue configuration properties */
struct configuration {
    
    

};
