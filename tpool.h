#ifndef H_TPOOL
#define H_TPOOL 1

typedef struct queue
{
	void **key;
	int len;
} queue;

typedef struct task
{
	void(*fn)(void *);
	void *param;
} task;

typedef struct pool
{
	queue queue;
	pthread_mutex_t queue_mutex;
	pthread_cond_t queue_cond;
	int stop;
	pthread_t *workers;
	task *task;
	int max;
	int join_all;
} pool;
pool tpool;

void worker_init( int count );
void worker_add_job( void(*task)(void *), void *p );
void worker_join();
void worker_stop();
#endif