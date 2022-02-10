#include "worker.h"

#include <mpi.h>
#include <cmath>

using namespace std;


Worker::Worker(int rank) : Process(rank) {}

void Worker::get_topology_from_leader()
{
	MPI_Status status;
	MPI_Recv(cluster_size, 1, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD,
		&status);
	leader = status.MPI_SOURCE;
	cluster[0] = new int[cluster_size[0]];
	MPI_Recv(cluster[0], cluster_size[0], MPI_INT, leader, 0, MPI_COMM_WORLD,
		&status);

	for (int i = 1; i <= 2; i++) {	
		MPI_Recv(cluster_size + i, 1, MPI_INT, leader, 0, MPI_COMM_WORLD,
			&status);
		cluster[i] = new int[cluster_size[i]];
		MPI_Recv(cluster[i], cluster_size[i], MPI_INT, leader, 0,
			MPI_COMM_WORLD, &status);
	}
}

void Worker::receive_calculate_and_send_results_to_leader()
{
	int array_size;
	MPI_Status status;
	MPI_Recv(&array_size, 1, MPI_INT, leader, 0, MPI_COMM_WORLD, &status);

	int no_workers = 0;
	for (int i = 0; i < 3; i++)
		no_workers += cluster_size[i];

	int length = (int) ceil((double) array_size / (double) no_workers);
	int id = rank - 3;
	int begin = id * length;
	int end = min(begin + length - 1, array_size - 1);
	int size = end - begin + 1;

	if (size <= 0)
		return;

	int *v = new int[size];
	MPI_Recv(v, size, MPI_INT, leader, 0, MPI_COMM_WORLD, &status);
	process_range_of_array(v, size);
	MPI_Send(v, size, MPI_INT, leader, 0, MPI_COMM_WORLD);
	inform_communication(rank, leader);
	delete[] v;
}

void Worker::process_range_of_array(int *v, int dim)
{
	for (int i = 0; i < dim; i++)
		v[i] <<= 1;	
}

void run_as_worker(int rank, int no_processes)
{
	Worker self(rank);
	self.get_topology_from_leader();
	self.confirm_topology();
	self.receive_calculate_and_send_results_to_leader();
}
