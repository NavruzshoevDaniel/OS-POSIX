#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>

#define STRING_LEN_LIMIT 80

typedef struct Message
{
	struct Message *prev, *next;
	char text[STRING_LEN_LIMIT + 1];
} Message;

typedef struct
{
	Message *in, *out;
	sem_t sem_put, sem_get;
	pthread_mutex_t mutex_access;
	unsigned char is_destroyed;
	unsigned messages_limit;
} Message_Queue;

void message_queue_init(Message_Queue *queue, unsigned messages_limit);
size_t message_queue_put(Message_Queue *queue, char *text);
size_t message_queue_get(Message_Queue *queue, char *buffer, size_t buffer_length);
void message_queue_drop(Message_Queue *queue);
void message_queue_destroy(Message_Queue *queue);