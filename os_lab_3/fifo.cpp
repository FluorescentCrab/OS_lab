#include <iostream>
#include <vector>
#include <fstream>
#include <queue>
#include "processes.hpp"

using namespace std;

void fifo(vector<fifo_process*> fifo_processList) {
	int cur_time = 0;

	int run_time = 0;
	// so make somekind of queue
	// make a while true loop that exits when all the fifo_processes are completed
	
	// at the start of the while loop check all the fifo_processes that are blocked or not_arrived and update if necessary and make them ready
	// run the fifo_process at the start of the queue 
	// update the fifo_process and mark it as done if necessary
	
	queue<fifo_process*> fifo_processQueue;

	while (true) {
		// updating all the fifo_processes
		bool flag = true;
		for (fifo_process* p : fifo_processList) {
			process_state prevState = p->get_state();
			if (prevState == DONE) continue;
			else flag = false;
			p->update(cur_time);
			// if the prev state and cur state are diff then its ready to push into the queue
			// now if it becomes ready push it in the queue
			if(p->get_state() != prevState) fifo_processQueue.push(p);

		}

		if (flag) break;
		if (!fifo_processQueue.empty()) {
			int prev_time = cur_time;
			fifo_process* t = fifo_processQueue.front();
			fifo_processQueue.pop();
			cur_time = t->cpu_done(cur_time);

			cout <<"[" << t->getpid() << "] : " << prev_time << " " << cur_time << endl;
			run_time += cur_time - prev_time + 1;
			cur_time++;
		}
		else cur_time++;
	}

	cout << "run time of the program : " << run_time << endl;

	// average turn around time 
	long long total_turn_around_time = 0;
	int max_turn_around_time = 0;
	for (fifo_process* p : fifo_processList) {
		int cur_turn_around_time = p->get_turn_around_time();
		max_turn_around_time = cur_turn_around_time > max_turn_around_time ? cur_turn_around_time : max_turn_around_time;
		total_turn_around_time += cur_turn_around_time;
	}

	double average_turn_around_time = static_cast<double>(total_turn_around_time) /fifo_processList.size();
	cout << "average turn around time : " << average_turn_around_time << endl;
	cout << "max turn around time : " << max_turn_around_time << endl;
	return;
}

int main(int argc , char ** argv) {
	
	// argu init,file loading and taking input
	// ====================================================================================
	if (argc != 2) {
		cout << "error invalid parameters :: usage : ./fifo.out <path-to-workload-description-file> \ncurrent arguments : " << endl;
		for (int i = 1; i < argc ; i++)
			cout << argv[i] << "\t";

		return -1;
	}

	char* path_to_workload_descriptor = argv[1];

	fstream workLoadFile(path_to_workload_descriptor);

	if (!workLoadFile.is_open()) {
		cout << "error file not opened" << endl;
		return -2;
	}

	cout << "file loaded successfully" << endl;

	// ====================================================================================

	vector<fifo_process*> fifo_process_list;

	vector<int> input;
	int i;
	int id = 0;
	while (workLoadFile >> i) {
		if (i == -1) {
			fifo_process* new_fifo_process = new fifo_process(input, id++);
			fifo_process_list.push_back(new_fifo_process);
			input.clear();
			cout << endl;
			continue;
		}
		cout << i << "\t";
		input.push_back(i);
	}
	cout << endl;

	// ====================================================================================

	cout << "STARTING FIFO.." << endl;
	fifo(fifo_process_list);

	for (int i = 0; i < fifo_process_list.size(); i++) delete fifo_process_list[i];

	return 0;
}