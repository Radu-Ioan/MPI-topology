#ifndef __COORDINATOR_H_
#define __COORDINATOR_H_

#include "process.h"

/**
 * Function executed by a coordinator process
 */
void run_as_coordinator(int rank, int no_processes, int error,
		int array_dim = -1);

struct Coordinator : Process
{
	/* holds the data about 0-1 edge: false if it's up, true otherwise */
	bool broken_edge;

	Coordinator(int rank, int edge);
	~Coordinator();

	void read_cluster();
	void send_cluster_to_other_leaders();
	void receive_clusters_from_other_leaders();
	void send_topology_into_cluster();

	/* in this function the 1 and 0 send their clusters to 2, and 2 receives
	 * them
	 */
	void send_cluster_to_leaders_weak_edge();
	/* here 2 sends its cluster to 1 and 0, and they receive it */
	void receive_cluster_from_leaders_weak_edge();
};


namespace initiator_process
{
	/**
	 * These functions are run only by the 0 process, i.e the process who
	 * generates the array
	 */

	/*
	 * In this function, process 0 starts to generate the vector and share the
	 * respective data with the other procs
	 */
	void start_calculation(Coordinator &proc, int array_dim);

	/* Process 0 sends data to the other coordinators */
	void send_stuff_to_leaders(Coordinator &proc, int *array, int array_dim);

	/* Process 0 sends data to its workers */
	void send_stuff_to_workers(Coordinator &proc, int *array, int array_dim);

	/* Proc 0 gets the results and fills the array */
	void receive_results_from_workers(Coordinator &proc, int *array,
				int array_dim);

	namespace bad_link
	{
		void send_stuff_to_leaders(Coordinator &proc, int *array,
				int array_dim);
		void receive_results(Coordinator &proc, int *array, int array_dim);
	}

	void print_result(int *array, int size);
}


namespace dependent_process
{
	/*
	* function used by 1 and 2 processes to receive and transmit the data got
	* from 0;
	* similar to start_calculation for process 0
	*/
	void forward_data(Coordinator &proc);

	/**
	 * receives data from coordinator 0, forwards it to the workers and saves 
	 * the total array_size received
	 * @param proc the process who forwards data
	 * @param array_size address to store the dimension got from 0 process
	 */
	void forward_stuff_to_workers(Coordinator &proc, int *array_size);

	/* the processes 1 and 2 send back the results to proc 0 */
	void receive_and_send_results_to_initiator(Coordinator &proc,
			int array_size);

	namespace bad_link
	{
		namespace pivot
		{
			/**
			 * receives data from coordinator 0, forwards it to other waiting
			 * coordinators (i.e proc 1) and to its workers and saves the total
			 * array_size received
			 * @param proc the process who forwards data
			 * @param array_size address to store the dimension got from 0
			 * process
			 */
			void forward_data(Coordinator &proc, int *array_size);

			/* the 2 proc gets data from 1, forwards to 0, and then sends the
			 * results from its cluster too */
			void send_back_results(Coordinator &proc, int array_size);
		}

		namespace waiting_process
		{
			/**
			 * Proc 1 gets data from 2 and forwards it to its cluster.
			 * Also, saves the array size at the given address which is sent
			 * as argument
			 */
			void get_data_and_send_it_to_workers(Coordinator &proc,
						int *array_size);
			void send_back_results(Coordinator &proc, int array_size);
		}
	}
}


#endif
