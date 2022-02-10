#ifndef __PROCESS_H_
#define __PROCESS_H_

/* every process must know the topology after the first stage */
struct Process
{
	int rank;
	int *cluster[3];
	int cluster_size[3];
	Process(int rank);
	void inform_communication(int src, int dest);
	void confirm_topology();
};

#endif
