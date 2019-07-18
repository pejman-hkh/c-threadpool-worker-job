#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <signal.h>
#include "tpool.h"

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

	worker_init( 5 );

	for( int i = 1; i <= 20; i++ ) {
		worker_add_job( test, i );
	}

	worker_join();

	return 0;
}