#include <iostream>
#include <vector>
#include <fstream>
#include <queue>
#include "cpu.hpp"
#include <unordered_map>
#include "processes.hpp"

using namespace std;

void round_robin(vector<rr_process*> rr_processList, int time_quanta,  int no_of_processors) {
	int cur_time = 0;

	int run_time = 0;

	unordered_map<int, rr_process*> int_process_map;
	vector<vector<vector<int>>> output(no_of_processors);

	vector<cpu*> processor_list;

	for (int i = 0; i < no_of_processors; i++) {
		cpu* c = new cpu(i);
		processor_list.push_back(c);
	}

	for (rr_process* p : rr_processList) {
		int_process_map[p->getpid()] = p;
	}
	// go through the list and if the process is ready just execute it 
	// do this untill all the processes are done
	queue<rr_process*> rr_processQueue;

	vector<rr_process*> requeue;

	while (true) {
		bool flag = true;
		for (rr_process* p : rr_processList) {
			process_state prevState = p->get_state();
			if (prevState == DONE) continue;
			else flag = false;
			p->update(cur_time);
			// if the prev state and cur state are diff then its ready to push into the queue
			// now if it becomes ready push it in the queue
			if (p->get_state() == READY && (prevState == BLOCKED || prevState == NOT_ARRIVED || prevState == RUNNING)) rr_processQueue.push(p);
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

		if (flag) break;
		/*for (rr_process* p : requeue) rr_processQueue.push(p);
		requeue.clear();*/

		/*if (!rr_processQueue.empty()) {
			int prev_time = cur_time;
			rr_process* t = rr_processQueue.front();
			rr_processQueue.pop();
			cur_time = t->cpu_done(cur_time, time_quanta);

			cout << "[" << t->getpid() << "] : " << prev_time << " " << cur_time << endl;
			run_time += cur_time - prev_time + 1;
			cur_time++;
			if (t->get_state() == READY) requeue.push_back(t);
		}
		else cur_time++;*/


		for (cpu* c : processor_list) {
			if (c->isUsed == false && !rr_processQueue.empty()) {
				rr_process* p = rr_processQueue.front();
				rr_processQueue.pop();
				c->time_stamp = p->set_running(cur_time,time_quanta);
				c->isUsed = true;
				c->pid = p->getpid();
				c->time_started = cur_time;
			}
		}

		for (rr_process* p : rr_processList) {
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
	for (rr_process* p : rr_processList) {
		int cur_turn_around_time = p->get_turn_around_time();
		//cout << "turn around time " << cur_turn_around_time << endl;
		max_turn_around_time = cur_turn_around_time > max_turn_around_time ? cur_turn_around_time : max_turn_around_time;
		total_turn_around_time += cur_turn_around_time;
	}

	double average_turn_around_time = static_cast<double>(total_turn_around_time) / rr_processList.size();
	cout << "AVERAGE TURN AROUND TIME:" << average_turn_around_time << endl;
	cout << "MAX TURN AROUND TIME:" << max_turn_around_time << endl;
	return;
}


int main(int argc, char** argv) {

	// argu init,file loading and taking input
	// ====================================================================================
	if (argc != 4) {
		cout << "error invalid parameters :: usage : ./round_robin_mp.out <path-to-workload-description-file> <time_quanta> <no_of_processors>\ncurrent arguments : " << endl;
		for (int i = 1; i < argc; i++)
			cout << argv[i] << "\t";

		return -1;
	}

	char* path_to_workload_descriptor = argv[1];
	char* time_quanta_char = argv[2];
	char* no_of_processors_char = argv[3];

	int time_quanta = atoi(time_quanta_char);
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

	vector<rr_process*> rr_process_list;

	vector<int> input;
	int i;
	int id = 0;
	while (workLoadFile >> i) {
		if (i == -1) {
			rr_process* new_rr_process = new rr_process(input, id++);
			rr_process_list.push_back(new_rr_process);
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
	cout << "STARTING ROUND ROBIN.." << endl;

	round_robin(rr_process_list, time_quanta,no_of_processors);

	for (int i = 0; i < rr_process_list.size(); i++) delete rr_process_list[i];

	return 0;
}