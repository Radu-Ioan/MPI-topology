#include "process.h"

#include <cstdio>
#include <string>

using namespace std;

Process::Process(int rank) : rank(rank) {}

void Process::inform_communication(int src, int dest)
{
	printf("M(%d,%d)\n", src, dest);
}

void Process::confirm_topology()
{
	string message = to_string(rank) + " -> ";
	for (int i = 0; i < 3; i++) {
		message += to_string(i) + ':';
		for (int j = 0; j < cluster_size[i]; ++j) {
			message += to_string(cluster[i][j]);
			if (j < cluster_size[i] - 1) {
				message += ',';
			}
		}
		message += ' ';
	}
	printf("%s\n", message.c_str());
}
