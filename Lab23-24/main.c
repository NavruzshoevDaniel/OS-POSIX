#ifdef SEM_IMPEMENTATION
#include "sem_implementation/message_queue.h"
#else
#include "cond_implementation/message_queue.h"
#endif

#include "console_app_tools.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define PRODUCERS_COUNT 5
#define CONSUMERS_COUNT 4
#define PRODUCER_PUT_DELAY_SEC 2
#define CONSUMER_GET_DELAY_SEC 4
#define QUEUE_DROP_TIMING_SEC 20
#define MESSAGES_LIMIT 10

pthread_t threads[PRODUCERS_COUNT + CONSUMERS_COUNT];
Message_Queue queue;

int get_thread_id()
{
	pthread_t self = pthread_self();
	for (int i = 0; i < PRODUCERS_COUNT + CONSUMERS_COUNT; i++)
		if (pthread_equal(self, threads[i]))
			return i < PRODUCERS_COUNT ? i : i - PRODUCERS_COUNT;
	return -1;
}

void *producer_run()
{
	int id = get_thread_id();
	char message_text[STRING_LEN_LIMIT + 1];
	int message_number = 1;

	while (1)
	{
		sprintf(message_text, "Hello from PRODUCER#%d [%d]", id, message_number++);
		sleep(PRODUCER_PUT_DELAY_SEC);
		if (!message_queue_put(&queue, message_text))
			pthread_exit(NULL);
		printf("%s[PRODUCER#%d] Put: %s\n", YELLOW_COLOR, id, message_text);
	}
}

void *consumer_run()
{
	int id = get_thread_id();
	char message_text_buf[STRING_LEN_LIMIT + 1];

	while (1)
	{
		sleep(CONSUMER_GET_DELAY_SEC);
		if (!message_queue_get(&queue, message_text_buf, STRING_LEN_LIMIT + 1))
			pthread_exit(NULL);
		else
			printf("%s[CONSUMER#%d] Get: %s\n", MAGENTA_COLOR, id, message_text_buf);
	}
}

int main()
{
	message_queue_init(&queue, MESSAGES_LIMIT);
	printf("%s[PARENT] Init\n", PARENT_COLOR);

	for (int i = 0; i < PRODUCERS_COUNT; i++)
		if (pthread_create(&threads[i], NULL, producer_run, NULL))
			throw_and_exit("pthread_create");
	for (int i = PRODUCERS_COUNT; i < PRODUCERS_COUNT + CONSUMERS_COUNT; i++)
		if (pthread_create(&threads[i], NULL, consumer_run, NULL))
			throw_and_exit("pthread_create");

	sleep(QUEUE_DROP_TIMING_SEC);

	message_queue_drop(&queue);
	printf("%s[PARENT] Drop\n", PARENT_COLOR);
	for (int i = 0; i < PRODUCERS_COUNT + CONSUMERS_COUNT; i++)
		if (pthread_join(threads[i], NULL))
			throw_and_exit("pthread_join");
	message_queue_destroy(&queue);
	printf("%s[PARENT] Destroy\n", PARENT_COLOR);
	exit(EXIT_SUCCESS);
}
