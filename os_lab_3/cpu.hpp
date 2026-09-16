#include <iostream>
#include <vector>

using namespace std;


class cpu {
public :
	bool isUsed;
	int cpu_id;
	int time_stamp;
	int time_started;
	int pid;
	cpu(int cpu_id);
};