#include <iostream>
#include <vector>
#include <fstream>
#include <queue>
#include <algorithm>
#include <unordered_map>
#include "processes.hpp"

#define queue_time 10

using namespace std;

struct req_comp {
	bool operator()(const mlfq_process* a, const mlfq_process* b) const {
		// Return true if 'a' should be ordered before 'b'
		return a->getpid() < b->getpid();
	}
};

void mlfq(vector<mlfq_process*> mlfq_process_list, int time_quanta, int no_of_queues, int reset_time) {
	int cur_time = 0;
	int run_time = 0;
	int cur_reset_time = reset_time - 1;

	vector<queue<mlfq_process*>> queues(no_of_queues);
	unordered_map<mlfq_process*, int> mp;
	unordered_map<mlfq_process*, int> time_remaining;

	vector<mlfq_process*> requeue;

	for (mlfq_process* p : mlfq_process_list) {
		mp[p] = 0;
		time_remaining[p] = queue_time;
	}

	while (true) {
		// updating all the fifo_processes
		bool flag = true;
		for (mlfq_process* p : mlfq_process_list) {
			process_state prevState = p->get_state();
			if (prevState == DONE) continue;
			else flag = false;
			p->update(cur_time);
			// if the prev state and cur state are diff then its ready to push into the queue
			// now if it becomes ready push it in the queue
			if (p->get_state() != prevState) queues[mp[p]].push(p);
		}


		for (mlfq_process* p : requeue) queues[mp[p]].push(p);
		requeue.clear();

		if (flag) break;

		bool atleast_one_executed = false;

		for (int i = 0; i < no_of_queues; i++) {
			if (!queues[i].empty()) {
				atleast_one_executed = true;
				int prev_time = cur_time;
				mlfq_process* t = queues[i].front();
				queues[i].pop();
				// if we are at the reset time
				// so we have 3 things ->
				// reset time , time_remaining and time_quanta

				bool not_normal_execution = (cur_reset_time - cur_time <= time_quanta) || (time_remaining[t] <= time_quanta);
				bool hard_reset_first = cur_reset_time <= cur_time + time_remaining[t];
				if (not_normal_execution && hard_reset_first) {
					if (cur_reset_time - cur_time > 0) {
						cur_time = t->cpu_done(cur_time, cur_reset_time - cur_time);
						cout << "1queue:[" << mp[t] << "] " << "pid:[" << t->getpid() << "] : " << prev_time << " " << cur_time << endl;
						run_time += cur_time - prev_time + 1;
						cur_time++;
					}

					cur_reset_time = cur_time + reset_time;

					vector<mlfq_process*> req;
					for (int j = 0; j < no_of_queues; j++) {
						while (!queues[j].empty()) {
							req.push_back(queues[j].front());
							queues[j].pop();
						}
					}

					if(t->get_state() == READY)req.push_back(t);

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
					break;
				}
				else if( not_normal_execution && !hard_reset_first) {

					// if remaining time is over
					if (mp[t] < no_of_queues - 1) {
						if (time_remaining[t] > 0) {
							cur_time = t->cpu_done(cur_time, time_remaining[t]);
							cout << "2queue:[" << mp[t] << "] " << "pid:[" << t->getpid() << "] : " << prev_time << " " << cur_time << endl;
							run_time += cur_time - prev_time + 1;
							cur_time++;
						}

						time_remaining[t] = queue_time;
						
						int cur_q = mp[t];
						mp[t] = cur_q + 1;
						if (t->get_state() == READY) requeue.push_back(t);
						
					}
					else {
						cur_time = t->cpu_done(cur_time, time_quanta);
						cout << "3queue:[" << mp[t] << "] " << "pid:[" << t->getpid() << "] : " << prev_time << " " << cur_time << endl;
						run_time += cur_time - prev_time + 1;
						cur_time++;
						if (t->get_state() == READY) requeue.push_back(t);
					}
					break;
				}

				cur_time = t->cpu_done(cur_time, time_quanta);
				// reducing the amount of time remaining
				time_remaining[t] -= cur_time - prev_time + 1;
				cout << "4queue:[" << mp[t] << "] " << "pid:[" << t->getpid() << "] : " << prev_time << " " << cur_time << endl;
				run_time += cur_time - prev_time + 1;
				cur_time++;
				if (t->get_state() == READY) requeue.push_back(t);
				break;
			}

		}

		if(!atleast_one_executed)cur_time++;
	}


	cout << "run time of the program : " << run_time << endl;

	// average turn around time 
	long long total_turn_around_time = 0;
	int max_turn_around_time = 0;
	for (mlfq_process* p : mlfq_process_list) {
		int cur_turn_around_time = p->get_turn_around_time();
		max_turn_around_time = cur_turn_around_time > max_turn_around_time ? cur_turn_around_time : max_turn_around_time;
		total_turn_around_time += cur_turn_around_time;
	}


	double average_turn_around_time = static_cast<double>(total_turn_around_time) / mlfq_process_list.size();
	cout << "average turn around time : " << average_turn_around_time << endl;
	cout << "max turn around time : " << max_turn_around_time << endl;

	return;
	
}

int main(int argc, char** argv) {

	// argu init,file loading and taking input
	// ====================================================================================
	if (argc != 2) {
		cout << "error invalid parameters :: usage : ./mlfq.out <path-to-workload-description-file> \ncurrent arguments : " << endl;
		for (int i = 1; i < argc; i++)
			cout << argv[i] << "\t";

		return -1;
	}

	char* path_to_workload_descriptor = argv[1];

	
	const int time_quanta = 2;
	const int no_of_queues = 3;
	const int reset_time = 20; 
	vector<int> queue_times = { 10,10,10 };

	fstream workLoadFile(path_to_workload_descriptor);

	if (!workLoadFile.is_open()) {
		cout << "error file not opened" << endl;
		return -2;
	}

	cout << "file loaded successfully" << endl;

	// ====================================================================================

	vector<mlfq_process*> mlfq_process_list;

	vector<int> input;
	int i;
	int id = 0;
	while (workLoadFile >> i) {
		if (i == -1) {
			mlfq_process* new_mlfq_process = new mlfq_process(input,id++);
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

	cout << "STARTING MLFQ.." << endl;
	mlfq(mlfq_process_list, time_quanta, no_of_queues, reset_time);

}
