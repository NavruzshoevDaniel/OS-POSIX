#include "message_queue.h"
#include "cond.h"
#include "../mutex.h"
#include <stdlib.h>
#include <string.h>

void message_queue_init(Message_Queue *queue, unsigned messages_limit)
{
	queue->in = NULL;
	queue->out = NULL;
	queue->is_destroyed = 0;
	queue->messages_limit = messages_limit;
	queue->messages_count = 0;

	cond_try_init(&queue->cond_access);
	mutex_try_init(&queue->mutex_access);
}

size_t message_queue_put(Message_Queue *queue, char *text)
{
	mutex_try_lock(&queue->mutex_access);
	while (!queue->is_destroyed &&
		   queue->messages_count >= queue->messages_limit)
	{
		cond_try_wait(&queue->cond_access, &queue->mutex_access);
	}
	if (queue->is_destroyed)
	{
		mutex_try_unlock(&queue->mutex_access);
		return 0;
	}

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

	if (!queue->messages_count)
		pthread_cond_signal(&queue->cond_access);
	queue->messages_count++;
	mutex_try_unlock(&queue->mutex_access);
	return strlen(new_message->text) + 1;
}

size_t message_queue_get(Message_Queue *queue, char *buffer, size_t buffer_length)
{
	mutex_try_lock(&queue->mutex_access);
	while (!queue->messages_count && !queue->is_destroyed)
		cond_try_wait(&queue->cond_access, &queue->mutex_access);

	if (queue->is_destroyed)
	{
		mutex_try_unlock(&queue->mutex_access);
		return 0;
	}

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

	if (queue->messages_count == queue->messages_limit)
		cond_try_signal(&queue->cond_access);
	queue->messages_count--;
	mutex_try_unlock(&queue->mutex_access);

	strncpy(buffer, message->text, buffer_length);
	buffer[buffer_length - 1] = '\0';
	free(message);

	return strlen(buffer);
}

void message_queue_drop(Message_Queue *queue)
{
	queue->is_destroyed = 1;
	cond_try_broadcast(&queue->cond_access);

	mutex_try_lock(&queue->mutex_access);
	Message *message = queue->in;
	while (message)
	{
		Message *buf = message->next;
		free(message);
		message = buf;
	}
	mutex_try_unlock(&queue->mutex_access);
}

void message_queue_destroy(Message_Queue *queue)
{
	cond_try_destroy(&queue->cond_access);
	mutex_try_destroy(&queue->mutex_access);
}