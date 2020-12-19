#include "message_queue.h"
#include "../mutex.h"
#include "sem.h"
#include <string.h>

void message_queue_init(Message_Queue *queue, unsigned messages_limit)
{
	queue->in = NULL;
	queue->out = NULL;
	queue->is_destroyed = 0;
	mutex_try_init(&queue->mutex_access);
	sem_try_init(&queue->sem_put, 0, messages_limit);
	sem_try_init(&queue->sem_get, 0, 0);
}

size_t message_queue_put(Message_Queue *queue, char *text)
{
	sem_try_wait(&queue->sem_put);
	if (queue->is_destroyed)
	{
		sem_try_post(&queue->sem_put);
		return 0;
	}
	mutex_try_lock(&queue->mutex_access);

	Message *new_message = malloc(sizeof(Message));
	strncpy(new_message->text, text, sizeof(new_message->text));
	new_message->text[sizeof(new_message->text)] = '\0';
	new_message->prev = NULL;
	new_message->next = queue->in;

	if (queue->in == NULL)
	{
		queue->in = new_message;
		queue->out = new_message;
	}
	else
	{
		queue->in->prev = new_message;
		queue->in = new_message;
	}

	mutex_try_unlock(&queue->mutex_access);
	sem_try_post(&queue->sem_get);
	return strlen(new_message->text) + 1;
}

size_t message_queue_get(Message_Queue *queue, char *buffer, size_t buffer_length)
{
	sem_try_wait(&queue->sem_get);
	if (queue->is_destroyed)
	{
		sem_try_post(&queue->sem_get);
		return 0;
	}
	mutex_try_lock(&queue->mutex_access);

	Message *message = queue->out;
	if (queue->in == message)
	{
		queue->in = NULL;
		queue->out = NULL;
	}
	else
	{
		queue->out = message->prev;
		queue->out->next = NULL;
	}

	strncpy(buffer, message->text, buffer_length);
	buffer[buffer_length - 1] = '\0';
	free(message);

	mutex_try_unlock(&queue->mutex_access);
	sem_try_post(&queue->sem_put);

	return strlen(buffer);
}

void message_queue_drop(Message_Queue *queue)
{
	queue->is_destroyed = 1;
	sem_try_post(&queue->sem_put);
	sem_try_post(&queue->sem_get);

	Message *message = queue->in;
	while (message)
	{
		Message *buf = message->next;
		free(message);
		message = buf;
	}
}

void message_queue_destroy(Message_Queue *queue)
{
	sem_try_destroy(&queue->sem_put);
	sem_try_destroy(&queue->sem_get);
}