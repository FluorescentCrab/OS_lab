#include <iostream>
#include <vector>
#include <fstream>
#include <queue>
#include "cpu.hpp"
#include <algorithm>
#include <unordered_map>
#include "processes.hpp"


using namespace std;

struct req_comp {
	bool operator()(const mlfq_process* a, const mlfq_process* b) const {
		// Return true if 'a' should be ordered before 'b'
		return a->getpid() < b->getpid();
	}
};

void mlfq(vector<mlfq_process*> mlfq_process_list, int time_quanta, int no_of_queues, int reset_time, int queue_time,int no_of_processors) {
	int cur_time = 0;
	int run_time = 0;
	int cur_reset_time = reset_time - 1;

	vector<queue<mlfq_process*>> queues(no_of_queues);
	unordered_map<mlfq_process*, int> mp;
	unordered_map<mlfq_process*, int> time_remaining;
	unordered_map<int, mlfq_process*> int_process_map;
	vector<vector<vector<int>>> output(no_of_processors);

	vector<mlfq_process*> requeue;

	vector<cpu*> processor_list;

	for (int i = 0; i < no_of_processors; i++) {
		cpu* c = new cpu(i);
		processor_list.push_back(c);
	}

	for (mlfq_process* p : mlfq_process_list) {
		mp[p] = 0;
		time_remaining[p] = queue_time;
		int_process_map[p->getpid()] = p;
	}

	while (true) {
		// updating all the fifo_processes
		bool flag = true;
		
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
				mlfq_process* temp = int_process_map[c->pid];
				
				// so here we make sure that time remaining always reamins 0 if there is demotion
				if (time_remaining[temp] == 0) {
					// if time remainig is zero we demote
					time_remaining[temp] = queue_time;
					if (mp[temp] < no_of_queues - 1) {
						int cur_q = mp[temp];
						mp[temp] = cur_q + 1;
					}
				}
			}
		}

		for (mlfq_process* p : mlfq_process_list) {
			process_state prevState = p->get_state();
			if (prevState == DONE) continue;
			else flag = false;
			p->update(cur_time);
			// if the prev state and cur state are diff then its ready to push into the queue
			// now if it becomes ready push it in the queue
			if (p->get_state() == READY && (prevState == BLOCKED || prevState == NOT_ARRIVED || prevState == RUNNING)) queues[mp[p]].push(p);
		}

		// if it is reset time we reset all the processes
		if (cur_time >= cur_reset_time) {
			cur_reset_time += reset_time;
			vector<mlfq_process*> req;
			for (int j = 0; j < no_of_queues; j++) {
				while (!queues[j].empty()) {
					req.push_back(queues[j].front());
					queues[j].pop();
				}
			}

			for (mlfq_process* p : mlfq_process_list) {
				if (p->get_state() != DONE) {
					mp[p] = 0;
					time_remaining[p] = queue_time;
				}
			}

			sort(req.begin(), req.end(), req_comp());

			for (mlfq_process* p : req) {
				queues[0].push(p);
			}
		}

		/*for (mlfq_process* p : requeue) queues[mp[p]].push(p);
		requeue.clear();*/

		if (flag) break;

		bool atleast_one_executed = false;

		for (cpu* c : processor_list) {
			if (c->isUsed == false ) {
				for (int i = 0; i < no_of_queues; i++) {
					if (!queues[i].empty()) {
						mlfq_process* t = queues[i].front();
						queues[i].pop();
						c->isUsed = true;
						c->pid = t->getpid();
						c->time_started = cur_time;
						bool not_normal_execution = (cur_reset_time - cur_time <= time_quanta) || (time_remaining[t] <= time_quanta);
						bool hard_reset_first = cur_reset_time <= cur_time + time_remaining[t];
						if (not_normal_execution && hard_reset_first) {
							c->time_stamp = t->set_running(cur_time, cur_reset_time - cur_time);
							time_remaining[t] -= cur_reset_time - cur_time;
							break;
						}
						else if (not_normal_execution && !hard_reset_first) {

							c->time_stamp = t->set_running(cur_time, time_remaining[t]);
							time_remaining[t] = 0;
							break;
						}

						c->time_stamp = t->set_running(cur_time, time_quanta);
						// reducing the amount of time remaining
						time_remaining[t] -= time_quanta;
						break;
					}

				}
			}
		}

		for (mlfq_process* p : mlfq_process_list) {
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
	for (mlfq_process* p : mlfq_process_list) {
		int cur_turn_around_time = p->get_turn_around_time();
		//cout << "turn around time " << cur_turn_around_time << endl;
		max_turn_around_time = cur_turn_around_time > max_turn_around_time ? cur_turn_around_time : max_turn_around_time;
		total_turn_around_time += cur_turn_around_time;
	}

	double average_turn_around_time = static_cast<double>(total_turn_around_time) / mlfq_process_list.size();
	cout << "AVERAGE TURN AROUND TIME:" << average_turn_around_time << endl;
	cout << "MAX TURN AROUND TIME:" << max_turn_around_time << endl;
	return;
}

int main(int argc, char** argv) {

	// argu init,file loading and taking input
	// ====================================================================================
	if (argc != 7) {
		cout << "error invalid parameters :: usage : ./mlfq_mp.out <path-to-workload-description-file> <time_quanta> <no_of_queues> <reset_time> <queue_time> <no_of_processors>\ncurrent arguments : " << endl;
		for (int i = 1; i < argc; i++)
			cout << argv[i] << "\t";

		return -1;
	}

	char* path_to_workload_descriptor = argv[1];
	char* time_qunata_char = argv[2];
	char* no_of_queues_char = argv[3];
	char* reset_time_char = argv[4];
	char* queue_time_char = argv[5];
	char* no_of_processors_char = argv[6];

	const int time_quanta = atoi(time_qunata_char);
	const int no_of_queues = atoi(no_of_queues_char);
	const int reset_time = atoi(reset_time_char);
	const int queue_time = atoi(queue_time_char);
	const int no_of_processors = atoi(no_of_processors_char);

	fstream workLoadFile(path_to_workload_descriptor);

	if (!workLoadFile.is_open()) {
		cout << "error file not opened" << endl;
		return -2;
	}

	//cout << "file loaded successfully" << endl;
	cout << "===========================================================" << endl;
	cout << "INPUTS : " << endl;
	// ====================================================================================

	vector<mlfq_process*> mlfq_process_list;

	vector<int> input;
	int i;
	int id = 0;
	while (workLoadFile >> i) {
		if (i == -1) {
			mlfq_process* new_mlfq_process = new mlfq_process(input, id++);
			mlfq_process_list.push_back(new_mlfq_process);
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
	cout << "STARTING MLFQ.." << endl;
	mlfq(mlfq_process_list, time_quanta, no_of_queues, reset_time,queue_time,no_of_processors);

}
