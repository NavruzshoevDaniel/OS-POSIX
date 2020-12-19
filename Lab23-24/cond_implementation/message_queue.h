#include <pthread.h>

#define STRING_LEN_LIMIT 80

typedef struct Message
{
	struct Message *prev, *next;
	char text[STRING_LEN_LIMIT + 1];
} Message;

typedef struct
{
	Message *in, *out;
	pthread_mutex_t mutex_access;
	pthread_cond_t cond_access;
	unsigned messages_limit;
	unsigned messages_count;
	unsigned char is_destroyed;
} Message_Queue;

void message_queue_init(Message_Queue *queue, unsigned messages_limit);
size_t message_queue_put(Message_Queue *queue, char *text);
size_t message_queue_get(Message_Queue *queue, char *buffer, size_t buffer_length);
void message_queue_drop(Message_Queue *queue);
void message_queue_destroy(Message_Queue *queue);