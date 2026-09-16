#include <iostream>
#include <vector>
#include <fstream>
#include <queue>
#include "cpu.hpp"
#include <unordered_map>
#include "processes.hpp"

using namespace std;

// so make a cpu structure and store a process in it 
// instead of doing something over the top we can do this 
// modify the fifo_processs , by adding a new process called set_running where you pass in
// the time argument for which it should stay running 
// in update add a new if statment that updates its state according to the cur_time from running -> blocked 
// or done
// make a cpu struct in which it only contains a bool isUsed and time_stamp ( when it will be free )

void fifo(vector<fifo_process*> fifo_processList, int no_of_processors) {
	int cur_time = 0;

	int run_time = 0;

	unordered_map<int, fifo_process*> int_process_map;
	vector<cpu*> processor_list;
	vector<vector<vector<int>>> output(no_of_processors);

	for (int i = 0; i < no_of_processors; i++) {
		cpu* c = new cpu(i);
		processor_list.push_back(c);
	}

	for (fifo_process* p : fifo_processList) {
		int_process_map[p->getpid()] = p;
	}

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
			if (p->get_state() == READY && ( prevState == BLOCKED || prevState == NOT_ARRIVED) ) fifo_processQueue.push(p);

		}


		for (cpu* c : processor_list) {
			if (cur_time > c->time_stamp && c->isUsed) {
				c->isUsed = false;
				vector<int> t;
				t.push_back(c->pid);
				t.push_back(int_process_map[c->pid]->get_burst());
				t.push_back(c->time_started);
				t.push_back(c->time_stamp);
				output[c->cpu_id].push_back(t);
				//cout << "cpu id : [" << c->cpu_id << "] [" << c->pid << "] : " << c->time_started << " " << c->time_stamp << endl;
			}
		}

		//	if (inc_time) cur_time++;


		if (flag) break;
		/*if (!fifo_processQueue.empty()) {*/

		for (cpu* c : processor_list) {
			if (c->isUsed == false && !fifo_processQueue.empty()) {
				fifo_process* p = fifo_processQueue.front();
				fifo_processQueue.pop();	
				c->time_stamp = p->set_running(cur_time);
				c->isUsed = true;
				c->pid = p->getpid();
				c->time_started = cur_time;
			}
		}

		for (fifo_process* p : fifo_processList) {
			if (p->get_state() == RUNNING) {
				run_time++;
				break;
			}
		}

		cur_time++;

	}

	for (int i = 0; i < no_of_processors; i++) {
		cout << "CPU" << i << endl;
		for (vector<int> t : output[i]) {
			cout << "P" << t[0] << "," << t[1] << "\t" << t[2] << "\t" << t[3] << endl;
		}
		cout << endl;
	}
	

	cout << "===========================================================" << endl;
	cout << "RUN TIME : " << run_time << endl;

	// average turn around time 
	long long total_turn_around_time = 0;
	int max_turn_around_time = 0;
	for (fifo_process* p : fifo_processList) {
		int cur_turn_around_time = p->get_turn_around_time();
		//cout << "turn around time " << cur_turn_around_time << endl;
		max_turn_around_time = cur_turn_around_time > max_turn_around_time ? cur_turn_around_time : max_turn_around_time;
		total_turn_around_time += cur_turn_around_time;
	}

	double average_turn_around_time = static_cast<double>(total_turn_around_time) / fifo_processList.size();
	cout << "AVERAGE TURN AROUND TIME:" << average_turn_around_time << endl;
	cout << "MAX TURN AROUND TIME:" << max_turn_around_time << endl;
	return;
}

int main(int argc, char** argv) {

	// argu init,file loading and taking input
	// ====================================================================================
	if (argc != 3) {
		cout << "error invalid parameters :: usage : ./fifo_mp.out <path-to-workload-description-file> <no_of_processors> \ncurrent arguments : " << endl;
		for (int i = 1; i < argc; i++)
			cout << argv[i] << "\t";

		return -1;
	}

	char* path_to_workload_descriptor = argv[1];
	char* no_of_processors_char = argv[2];
	int no_of_processors = atoi(no_of_processors_char);
	fstream workLoadFile(path_to_workload_descriptor);

	if (!workLoadFile.is_open()) {
		cout << "error file not opened" << endl;
		return -2;
	}

	//cout << "file loaded successfully" << endl;
	cout << "===========================================================" << endl;
	cout << "INPUTS : " << endl;
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

	cout << "===========================================================" << endl;
	cout << "STARTING FIFO.. " << endl;
	fifo(fifo_process_list,no_of_processors);

	for (int i = 0; i < fifo_process_list.size(); i++) delete fifo_process_list[i];

	return 0;
}