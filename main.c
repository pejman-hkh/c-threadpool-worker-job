#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <signal.h>

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
} pool;

pool tpool;

void pool_init( pool *p, int max ) {

	pthread_mutex_init ( &(p->queue_mutex), NULL);
	pthread_cond_init ( &(p->queue_cond), NULL);

	p->workers = malloc(sizeof(pthread_t) * max);
	p->stop = 0;
	p->max = max;
}

void pool_free( pool *p ) {
	free( p->workers );
}

void queue_init( queue *q, int max ) {
	q->key = malloc(sizeof(void*) * max);
	q->len = 0;
}

void queue_add( queue *q, void *key ) {
	q->key[q->len] = key;

	q->len++;
}

void queue_free( queue *q ) {
	for( int i = 0; i < q->len; i++ ) {
		free(q->key[i] );
	}
	free( q->key );
}

void *queue_pop_front( queue *q ) {
	void *ret = q->key[0];
	for( int i = 1; i < q->len; i++ ) {
		q->key[i-1] = q->key[i];
	}
	q->len--;
	return ret;
}


void *task_add( task *t, void(*task)(void *), void *p ) {
	t->fn = task;
	t->param = p;
}


void new_worker() {
	pid_t tid = syscall(SYS_gettid);
	for(;;) {

	    pthread_mutex_lock(&(tpool.queue_mutex));
	    //pthread_cond_wait(&(tpool.queue_cond),&(tpool.queue_mutex));
	    //pthread_cond_signal(&(tpool.queue_cond));

		if( tpool.stop ) {
			//printf("breaked\n");
			pthread_mutex_unlock(&(tpool.queue_mutex));
			break;
		}

		if( tpool.queue.len > 0 && ! tpool.stop ) {
			task *t = queue_pop_front( &(tpool.queue) );
			pthread_mutex_unlock(&(tpool.queue_mutex));
			t->fn(t->param);
			free(t);
		}
	    pthread_mutex_unlock(&(tpool.queue_mutex));
	}	
}

void worker_init( int count ) {

	pool_init(&tpool, count);
	queue_init( &(tpool.queue), count );

	for( int i = 0; i < count; i++ ) {
		pthread_t thread_id; 
		pthread_create(&thread_id, NULL, new_worker, NULL);
		tpool.workers[i] = thread_id;
	}

}

void worker_add_job( void(*task)(void *), void *p ) {
	pthread_mutex_lock(&(tpool.queue_mutex));
	//pthread_cond_signal(&(tpool.queue_cond));
	tpool.task = malloc(sizeof(task));
	task_add(tpool.task, task, p);
	queue_add( &(tpool.queue), tpool.task );
	pthread_mutex_unlock(&(tpool.queue_mutex));
}

void worker_stop() {
	//if(tpool.stop == 0 ) {	
		printf("in stop\n");
		pthread_mutex_lock(&(tpool.queue_mutex));
		//pthread_cond_wait(&(tpool.queue_cond),&(tpool.queue_mutex));
		tpool.stop = 1;
		pthread_mutex_unlock(&(tpool.queue_mutex));
		queue_free(&(tpool.queue));
		pool_free(&tpool);
	//}
}

void worker_join() {
	for( int i = 0; i < tpool.max;i++ ) {
		pthread_join(tpool.workers[i], NULL);
	}
}

void test( void *p ) {
	printf("%d\n", p );
	sleep(1);
}

void signal_handler() {
	worker_stop();
}

int main(int argc, char const *argv[])
{
	signal(SIGINT, signal_handler);

	worker_init( 4 );

	for( int i = 1; i <= 10; i++ ) {
		worker_add_job( test, i );
	}

	worker_join();

	return 0;
}