#include <mpi.h>
#include <iostream>
#include <sstream>

#include "processing/coordinator.h"
#include "processing/worker.h"

using namespace std;

int main(int argc, char *argv[])
{
	int array_dim, rank, no_processes, error;

	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &no_processes);

	if (argc < 2) {
		cout << "Usage: mpirun -np <no_processes> ./tema3 "
			<< "<array_dim> <communication_error>";
		return 0;
	}

	stringstream convert(argv[1]);
	convert >> array_dim;
	convert = stringstream(argv[2]);
	convert >> error;

	if (rank > 2) {
		run_as_worker(rank, no_processes);
	} else {
		if (rank == 0)
			run_as_coordinator(rank, no_processes, error, array_dim);
		else
			run_as_coordinator(rank, no_processes, error);
	}

	MPI_Finalize();
	return 0;
}
