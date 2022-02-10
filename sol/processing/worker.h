#ifndef __WORKER_H_
#define __WORKER_H_

#include "process.h"

struct Worker : Process
{
	/* The coordinator of the worker. It is the only process through which
	 * this type of instance can communicate
	 */
	int leader;

	Worker(int rank);
	void get_topology_from_leader();

	void receive_calculate_and_send_results_to_leader();
	void process_range_of_array(int *v, int dim);
};

/**
 * Function executed by a worker process
 */
void run_as_worker(int rank, int no_processes);

#endif
