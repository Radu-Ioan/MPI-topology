#include "coordinator.h"

#include <mpi.h>
#include <fstream>
#include <iostream>
#include <string>
#include <cmath>

using namespace std;


void run_as_coordinator(int rank, int no_processes, int error, int array_dim)
{
	Coordinator process(rank, error);
	process.read_cluster();
	process.send_cluster_to_other_leaders();
	process.receive_clusters_from_other_leaders();
	process.confirm_topology();
	process.send_topology_into_cluster();

	if (rank == 0) {
		initiator_process::start_calculation(process, array_dim);
	} else {
		dependent_process::forward_data(process);
	}
}

Coordinator::Coordinator(int rank, int edge)
			: Process(rank), broken_edge(edge) {}

void Coordinator::read_cluster()
{
	string file = "cluster";
	file += (rank == 0) ? "0" : (rank == 1) ? "1" : "2";
	file += ".txt";

	ifstream in(file);
	in >> cluster_size[rank];

	cluster[rank] = new int[cluster_size[rank]];

	for (int i = 0; i < cluster_size[rank]; i++)
		in >> cluster[rank][i];

	in.close();
}

void Coordinator::send_cluster_to_other_leaders()
{
	if (broken_edge) {
		send_cluster_to_leaders_weak_edge();
		return;
	}

	int dest1 = (rank + 1) % 3;
	int dest2 = (rank + 2) % 3;

	MPI_Send(cluster_size + rank, 1, MPI_INT, dest1, 0, MPI_COMM_WORLD);
	inform_communication(rank, dest1);
	MPI_Send(cluster[rank], cluster_size[rank], MPI_INT, dest1, 0,
		MPI_COMM_WORLD);
	inform_communication(rank, dest1);

	MPI_Send(cluster_size + rank, 1, MPI_INT, dest2, 0, MPI_COMM_WORLD);
	inform_communication(rank, dest2);
	MPI_Send(cluster[rank], cluster_size[rank], MPI_INT, dest2, 0,
		MPI_COMM_WORLD);
	inform_communication(rank, dest2);
}

void Coordinator::receive_clusters_from_other_leaders()
{
	if (broken_edge) {
		receive_cluster_from_leaders_weak_edge();
		return;
	}

	int src1 = (rank + 1) % 3;
	int src2 = (rank + 2) % 3;

	MPI_Status status;

	MPI_Recv(cluster_size + src1, 1, MPI_INT, src1, 0, MPI_COMM_WORLD,
		&status);
	cluster[src1] = new int[cluster_size[src1]];
	MPI_Recv(cluster[src1], cluster_size[src1], MPI_INT, src1, 0,
		MPI_COMM_WORLD, &status);

	MPI_Recv(cluster_size + src2, 1, MPI_INT, src2, 0, MPI_COMM_WORLD,
		&status);
	cluster[src2] = new int[cluster_size[src2]];
	MPI_Recv(cluster[src2], cluster_size[src2], MPI_INT, src2, 0,
		MPI_COMM_WORLD, &status);
}

void Coordinator::send_cluster_to_leaders_weak_edge()
{
	MPI_Status status;

	if (rank < 2) {
		MPI_Send(cluster_size + rank, 1, MPI_INT, 2, 0, MPI_COMM_WORLD);
		inform_communication(rank, 2);
		MPI_Send(cluster[rank], cluster_size[rank], MPI_INT, 2, 0,
			MPI_COMM_WORLD);
		inform_communication(rank, 2);
	} else {
		// 2 is the articulation point and at the first stage, has to wait for
		// receiving the data and deliver it in order at the next step
		for (int i = 0; i <= 1; i++) {
			MPI_Recv(cluster_size + i, 1, MPI_INT, i, 0, MPI_COMM_WORLD,
				&status);
			cluster[i] = new int[cluster_size[i]];
			MPI_Recv(cluster[i], cluster_size[i], MPI_INT, i, 0,
				MPI_COMM_WORLD, &status);
		}
	}
}

void Coordinator::receive_cluster_from_leaders_weak_edge()
{
	MPI_Status status;

	// 0 waits first for cluster 1, and then expects to receive data from
	// cluster 2; similarly, 1 expects to receive cluster 0 and then 2;
	// in both cases, the source is 2
	if (rank < 2) {
		MPI_Recv(cluster_size + 1 - rank, 1, MPI_INT, 2, 0, MPI_COMM_WORLD,
			&status);
		cluster[1 - rank] = new int[cluster_size[1 - rank]];
		MPI_Recv(cluster[1 - rank], cluster_size[1 - rank], MPI_INT, 2, 0,
			MPI_COMM_WORLD, &status);

		MPI_Recv(cluster_size + 2, 1, MPI_INT, 2, 0, MPI_COMM_WORLD,
			&status);
		cluster[2] = new int[cluster_size[2]];
		MPI_Recv(cluster[2], cluster_size[2], MPI_INT, 2, 0, MPI_COMM_WORLD,
			&status);
	} else {
		for (int i = 0; i <= 1; i++) {
			MPI_Send(cluster_size + 1 - i, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
			inform_communication(rank, i);
			MPI_Send(cluster[1 - i], cluster_size[1 - i], MPI_INT, i, 0,
				MPI_COMM_WORLD);
			inform_communication(rank, i);

			MPI_Send(cluster_size + 2, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
			inform_communication(rank, i);
			MPI_Send(cluster[2], cluster_size[2], MPI_INT, i, 0,
				MPI_COMM_WORLD);
			inform_communication(rank, i);
		}
	}
}

void Coordinator::send_topology_into_cluster()
{
	for (int i = 0; i < cluster_size[rank]; i++) {
		for (int j = 0; j < 3; j++) {
			MPI_Send(cluster_size + j, 1, MPI_INT, cluster[rank][i], 0,
				MPI_COMM_WORLD);
			inform_communication(rank, cluster[rank][i]);
			MPI_Send(cluster[j], cluster_size[j], MPI_INT, cluster[rank][i],
				0, MPI_COMM_WORLD);
			inform_communication(rank, cluster[rank][i]);
		}
	}
}

Coordinator::~Coordinator()
{
	for (int i = 0; i < 3; i++)
		delete[] cluster[i];
}

void initiator_process::start_calculation(Coordinator &proc, int array_dim)
{
	int *array = new int[array_dim];
	for (int i = 0; i < array_dim; i++)
		array[i] = i;

	if (!proc.broken_edge) {
		initiator_process::send_stuff_to_leaders(proc, array, array_dim);
		// finally, send the stuff to the workers of itself
		initiator_process::send_stuff_to_workers(proc, array, array_dim);
		initiator_process::receive_results_from_workers(proc, array,
				array_dim);
	} else {
		initiator_process::bad_link::send_stuff_to_leaders(proc, array,
				array_dim);
		initiator_process::send_stuff_to_workers(proc, array, array_dim);
		initiator_process::bad_link::receive_results(proc,
				array, array_dim);
	}

	initiator_process::print_result(array, array_dim);
	delete[] array;
}

void initiator_process::send_stuff_to_leaders(Coordinator &proc, int *array,
	int array_dim)
{
	int no_workers = 0;
	for (int i = 0; i < 3; i++)
		no_workers += proc.cluster_size[i];

	int length = (int) ceil((double) array_dim / (double) no_workers);

	// send from 0 to 1 and 2 the vector ranges required by the workers
	for (int leader = 1; leader <= 2; ++leader) {
		// first send the array size so the receptors know what dimension of
		// data is following to be sent
		MPI_Send(&array_dim, 1, MPI_INT, leader, 0, MPI_COMM_WORLD);
		proc.inform_communication(proc.rank, leader);

		for (int i = 0; i < proc.cluster_size[leader]; i++) {
			int id = proc.cluster[leader][i] - 3;
			int begin = id * length;
			int end = min(begin + length - 1, array_dim - 1);
			int size = end - begin + 1;
			if (size > 0) {
				MPI_Send(array + begin, size, MPI_INT, leader, 0,
						MPI_COMM_WORLD);
				proc.inform_communication(proc.rank, leader);
			}
		}
	}
}

void initiator_process::send_stuff_to_workers(Coordinator &proc, int *array,
			int array_dim)
{
	int no_workers = 0;
	for (int i = 0; i < 3; i++)
		no_workers += proc.cluster_size[i];

	int length = (int) ceil((double) array_dim / (double) no_workers);

	for (int i = 0; i < proc.cluster_size[proc.rank]; ++i) {
		MPI_Send(&array_dim, 1, MPI_INT, proc.cluster[proc.rank][i], 0,
			MPI_COMM_WORLD);
		proc.inform_communication(proc.rank, proc.cluster[proc.rank][i]);

		// id relative to the portion that must be processed by the worker
		int id = proc.cluster[proc.rank][i] - 3;
		int begin = id * length;
		int end = min(begin + length - 1, array_dim - 1);
		int size = end - begin + 1;

		if (size > 0) {
			// forward the range to the worker
			MPI_Send(array + begin, size, MPI_INT, id + 3, 0, MPI_COMM_WORLD);
			proc.inform_communication(proc.rank, id + 3);
		}
	}
}

void initiator_process::receive_results_from_workers(Coordinator &proc,
			int *array, int array_dim)
{
	int no_workers = 0;
	for (int i = 0; i < 3; i++)
		no_workers += proc.cluster_size[i];

	int length = (int) ceil((double) array_dim / (double) no_workers);
	MPI_Status status;

	for (int leader = 2; leader >= 0; --leader) {
		for (int i = 0; i < proc.cluster_size[leader]; i++) {
			int id = proc.cluster[leader][i] - 3;
			int begin = id * length;
			int end = min(begin + length - 1, array_dim - 1);
			int size = end - begin + 1;

			if (size > 0) {
				int src = (leader == 0) ? id + 3 : leader;
				MPI_Recv(array + begin, size, MPI_INT, src, 0, MPI_COMM_WORLD,
						&status);
			}
		}
	}
}

void initiator_process::print_result(int *array, int size)
{
	string result = "Rezultat: ";
	for (int i = 0; i < size; i++)
		result += to_string(array[i]),
		result += ' ';
	printf("%s\n", result.c_str());
}

void dependent_process::forward_data(Coordinator &proc)
{
	int array_size;

	if (!proc.broken_edge) {
		dependent_process::forward_stuff_to_workers(proc, &array_size);
		dependent_process::receive_and_send_results_to_initiator(proc,
					array_size);
	} else {
		if (proc.rank == 2) {
			dependent_process::bad_link::pivot::forward_data(proc,
					&array_size);
			dependent_process::bad_link::pivot::send_back_results(proc,
					array_size);
		} else {
			dependent_process::bad_link::waiting_process
				::get_data_and_send_it_to_workers(proc, &array_size);
			dependent_process::bad_link::waiting_process
				::send_back_results(proc, array_size);
		}
	}
}

void dependent_process::forward_stuff_to_workers(Coordinator &proc,
			int *array_size)
{
	int no_workers = 0;
	for (int i = 0; i < 3; i++)
		no_workers += proc.cluster_size[i];

	int src = 0;
	int array_dim;
	MPI_Status status;
	MPI_Recv(&array_dim, 1, MPI_INT, src, 0, MPI_COMM_WORLD, &status);
	*array_size = array_dim;

	// the size of a range processed by a worker
	int length = (int) ceil((double) array_dim / (double) no_workers);
	int *range = new int[length];

	for (int i = 0; i < proc.cluster_size[proc.rank]; i++) {
		// id relative to the portion that must be processed by the worker
		int id = proc.cluster[proc.rank][i] - 3;
		int begin = id * length;
		int end = min(begin + length - 1, array_dim - 1);
		int size = end - begin + 1;

		int dest = id + 3;
		MPI_Send(&array_dim, 1, MPI_INT, dest, 0, MPI_COMM_WORLD);
		proc.inform_communication(proc.rank, dest);

		if (size > 0) {
			MPI_Recv(range, size, MPI_INT, src, 0, MPI_COMM_WORLD, &status);
			// forward the range to the worker
			MPI_Send(range, size, MPI_INT, dest, 0, MPI_COMM_WORLD);
			proc.inform_communication(proc.rank, dest);
		}
	}
	delete[] range;
}

void dependent_process::receive_and_send_results_to_initiator(
						Coordinator &proc, int array_size)
{
	int no_workers = 0;
	for (int i = 0; i < 3; i++)
		no_workers += proc.cluster_size[i];
	int length = (int) ceil((double) array_size / (double) no_workers);

	int *ranges[proc.cluster_size[proc.rank]];
	MPI_Status status;

	for (int i = 0; i < proc.cluster_size[proc.rank]; i++) {
		// id relative to the portion that must be processed by the worker
		int id = proc.cluster[proc.rank][i] - 3;
		int begin = id * length;
		int end = min(begin + length - 1, array_size - 1);
		int size = end - begin + 1;

		if (size > 0) {
			ranges[i] = new int[size];
			MPI_Recv(ranges[i], size, MPI_INT, id + 3, 0, MPI_COMM_WORLD,
				&status);
		}
	}

	int dest = 0;

	for (int i = 0; i < proc.cluster_size[proc.rank]; i++) {
		int id = proc.cluster[proc.rank][i] - 3;
		int begin = id * length;
		int end = min(begin + length - 1, array_size - 1);
		int size = end - begin + 1;

		if (size > 0) {
			MPI_Send(ranges[i], size, MPI_INT, dest, 0, MPI_COMM_WORLD);
			proc.inform_communication(proc.rank, dest);
			delete[] ranges[i];
		}
	}
}


// send data for 1, then for 2: both towards 2
void initiator_process::bad_link::send_stuff_to_leaders(Coordinator &proc,
			int *array, int array_dim)
{
	int no_workers = 0;
	for (int i = 0; i < 3; i++)
		no_workers += proc.cluster_size[i];

	int length = (int) ceil((double) array_dim / (double) no_workers);

	int dest = 2;

	MPI_Send(&array_dim, 1, MPI_INT, dest, 0, MPI_COMM_WORLD);
	proc.inform_communication(proc.rank, dest);

	// send to 2 for 1, and then for 2
	for (int coord = 1; coord <= 2; ++coord) {
		for (int i = 0; i < proc.cluster_size[coord]; i++) {
			int id = proc.cluster[coord][i] - 3;
			int begin = id * length;
			int end = min(begin + length - 1, array_dim - 1);
			int size = end - begin + 1;

			if (size > 0) {
				MPI_Send(array + begin, size, MPI_INT, dest, 0,
					MPI_COMM_WORLD);
				proc.inform_communication(proc.rank, dest);
			}
		}
	}
}

void initiator_process::bad_link::receive_results(Coordinator &proc,
		int *array, int array_dim)
{
	int no_workers = 0;
	for (int i = 0; i < 3; i++)
		no_workers += proc.cluster_size[i];

	int length = (int) ceil((double) array_dim / (double) no_workers);
	MPI_Status status;

	// works even for 0, 1, 2, but I preferred the order below
	for (int p : {1, 2, 0}) {
		for (int i = 0; i < proc.cluster_size[p]; i++) {
			int id = proc.cluster[p][i] - 3;
			int begin = id * length;
			int end = min(begin + length - 1, array_dim - 1);
			int size = end - begin + 1;

			if (size > 0) {
				int src = (proc.rank == p) ? id + 3 : 2;
				MPI_Recv(array + begin, size, MPI_INT, src, 0, MPI_COMM_WORLD,
					&status);
			}
		}
	}
}

void dependent_process::bad_link::pivot::forward_data(Coordinator &proc,
				int *array_size)
{
	int src = 0;
	MPI_Status status;
	MPI_Recv(array_size, 1, MPI_INT, src, 0, MPI_COMM_WORLD, &status);

	int no_workers = 0;
	for (int i = 0; i < 3; i++)
		no_workers += proc.cluster_size[i];

	int length = (int) ceil((double) *array_size / (double) no_workers);
	int *range = new int[length];

	for (int p = 1; p <= 2; p++) {
		if (p != proc.rank) {
			MPI_Send(array_size, 1, MPI_INT, p, 0, MPI_COMM_WORLD);
			proc.inform_communication(proc.rank, p);
		}

		for (int i = 0; i < proc.cluster_size[p]; i++) {
			int id = proc.cluster[p][i] - 3;
			int begin = id * length;
			int end = min(begin + length - 1, *array_size - 1);
			int size = end - begin + 1;

			int dest = (p == proc.rank) ? id + 3 : p;
			if (dest != p) {
				// if p is the current process, then first I have to send the
				// array_size to the worker
				MPI_Send(array_size, 1, MPI_INT, dest, 0, MPI_COMM_WORLD);
				proc.inform_communication(proc.rank, dest);
			}

			if (size > 0) {
				MPI_Recv(range, size, MPI_INT, src, 0, MPI_COMM_WORLD,
					&status);
				MPI_Send(range, size, MPI_INT, dest, 0, MPI_COMM_WORLD);
				proc.inform_communication(proc.rank, dest);
			}
		}
	}

	delete[] range;
}

void dependent_process::bad_link::pivot::send_back_results(Coordinator &proc,
				int array_size)
{
	int no_workers = 0;
	for (int i = 0; i < 3; i++)
		no_workers += proc.cluster_size[i];

	int length = (int) ceil((double) array_size / (double) no_workers);
	int *range = new int[length];

	MPI_Status status;
	int dest = 0;

	for (int p = 1; p <= 2; p++) {
		for (int i = 0; i < proc.cluster_size[p]; i++) {
			int id = proc.cluster[p][i] - 3;
			int begin = id * length;
			int end = min(begin + length - 1, array_size - 1);
			int size = end - begin + 1;

			if (size > 0) {
				int src = (p == proc.rank) ? id + 3 : p;
				MPI_Recv(range, size, MPI_INT, src, 0, MPI_COMM_WORLD,
						&status);
				MPI_Send(range, size, MPI_INT, dest, 0, MPI_COMM_WORLD);
				proc.inform_communication(proc.rank, dest);
			}
		}
	}

	delete[] range;
}

void dependent_process::bad_link::waiting_process
		::get_data_and_send_it_to_workers(Coordinator &proc, int *array_size)
{
	int src = 2;
	MPI_Status status;
	MPI_Recv(array_size, 1, MPI_INT, src, 0, MPI_COMM_WORLD, &status);

	int no_workers = 0;
	for (int i = 0; i < 3; i++)
		no_workers += proc.cluster_size[i];

	int length = (int) ceil((double) *array_size / (double) no_workers);
	int *range = new int[length];

	for (int i = 0; i < proc.cluster_size[proc.rank]; i++) {
		int id = proc.cluster[proc.rank][i] - 3;
		int begin = id * length - 3;
		int end = min(begin + length - 1, *array_size - 1);
		int size = end - begin + 1;

		int worker_dest = id + 3;
		MPI_Send(array_size, 1, MPI_INT, worker_dest, 0, MPI_COMM_WORLD);
		proc.inform_communication(proc.rank, worker_dest);

		if (size > 0) {
			MPI_Recv(range, size, MPI_INT, src, 0, MPI_COMM_WORLD, &status);
			MPI_Send(range, size, MPI_INT, worker_dest, 0, MPI_COMM_WORLD);
			proc.inform_communication(proc.rank, worker_dest);
		}
	}

	delete[] range;
}

void dependent_process::bad_link::waiting_process
		::send_back_results(Coordinator &proc, int array_size)
{
	int dest = 2;
	MPI_Status status;
	int no_workers = 0;
	for (int i = 0; i < 3; i++)
		no_workers += proc.cluster_size[i];

	int length = (int) ceil((double) array_size / (double) no_workers);
	int *ranges[proc.cluster_size[proc.rank]];

	for (int i = 0; i < proc.cluster_size[proc.rank]; i++) {
		int id = proc.cluster[proc.rank][i] - 3;
		int begin = id * length;
		int end = min(begin + length - 1, array_size - 1);
		int size = end - begin + 1;

		if (size > 0) {
			ranges[i] = new int[size];
			MPI_Recv(ranges[i], size, MPI_INT, id + 3, 0, MPI_COMM_WORLD,
					&status);
		}
	}

	for (int i = 0; i < proc.cluster_size[proc.rank]; i++) {
		int id = proc.cluster[proc.rank][i] - 3;
		int begin = id * length;
		int end = min(begin + length - 1, array_size - 1);
		int size = end - begin + 1;

		if (size > 0) {
			MPI_Send(ranges[i], size, MPI_INT, dest, 0, MPI_COMM_WORLD);
			proc.inform_communication(proc.rank, dest);
			delete[] ranges[i];
		}
	}
}
