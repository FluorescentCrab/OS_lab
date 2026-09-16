#include "processes.hpp"

process::process()
{
	id = -1;
	arrival_time = -1;
	cur_state = NOT_ARRIVED;
}

process::process(vector<int> input, int id) : id(id)
{
	cur_state = NOT_ARRIVED;
	
	arrival_time = input[0];
	for (int i = 1; i < input.size(); i++) {
		
		if (i % 2 == 0) cpu_bursts.push_back(input[i]);
		else ios_bursts.push_back(input[i]);
		
	}
}

int process::getpid() const
{
	return this->id;
}

int process::get_turn_around_time()
{
	return this->completion_time - this->arrival_time + 1;
}

process_state process::get_state()
{
	return cur_state;      
}

fifo_process::fifo_process(vector<int> input, int id)
{
	this->i = 0;
	this->id = id;
	cur_state = NOT_ARRIVED;

	arrival_time = input[0];
	for (int i = 1; i < input.size(); i++) {

		if (i % 2 == 1) cpu_bursts.push_back(input[i]);
		else ios_bursts.push_back(input[i]);

	}
		
}

void fifo_process::update(int cur_time)
{
	if (cur_state == NOT_ARRIVED) {
		if (cur_time >= arrival_time) cur_state = READY;
		return;
	}

	if (cur_state == BLOCKED) {
		if (cur_time >= time_stamp) {
			cur_state = READY;
			i++;
		}
		return;
	}

	if (cur_state == RUNNING) {
		if (cur_time > time_stamp) {
			if (i < ios_bursts.size()) {
				cur_state = BLOCKED;
				time_stamp = ios_bursts[i] + cur_time;
			}
			else {
				cur_state = DONE;
				completion_time = cur_time;
			}
		}
	}
}


int fifo_process::cpu_done(int cur_time)
{
	if (i < ios_bursts.size()) {
		cur_state = BLOCKED;
		time_stamp = ios_bursts[i] + cur_time + cpu_bursts[i];
	}
	else {
		cur_state = DONE;
		completion_time = cur_time + cpu_bursts[i] - 1;
	}
	
	return cur_time + cpu_bursts[i] - 1;
}

int fifo_process::set_running(int cur_time)
{
	cur_state = RUNNING;
	time_stamp = cur_time + cpu_bursts[i] - 1;
	return cur_time + cpu_bursts[i] - 1;
}

rr_process::rr_process(vector<int> input, int id)
{
	this->i = 0;
	this->id = id;
	cur_state = NOT_ARRIVED;

	arrival_time = input[0];
	for (int i = 1; i < input.size(); i++) {

		if (i % 2 == 1) cpu_bursts.push_back(input[i]);
		else ios_bursts.push_back(input[i]);

	}
}

void rr_process::update(int cur_time)
{
	if (cur_state == NOT_ARRIVED) {
		if (cur_time >= arrival_time) cur_state = READY;
		return;
	}

	if (cur_state == BLOCKED) {
		if (cur_time >= time_stamp) {
			cur_state = READY;
			i++;
		}
		return;
	}

	if (cur_state == RUNNING) {
		if (cur_time > time_stamp) {
			if (cpu_bursts[i] > 0) {
				cur_state = READY;
			}
			else {
				if (i < ios_bursts.size()) {
					cur_state = BLOCKED;
					time_stamp = ios_bursts[i] + cur_time;
				}
				else {
					cur_state = DONE;
					completion_time = cur_time;
				}
			}
		}
	}
}

int rr_process::cpu_done(int cur_time,int time_quanta)
{
	if (cpu_bursts[i] - time_quanta > 0) {
		cpu_bursts[i] -= time_quanta;
		return cur_time + time_quanta - 1;
	}
	else {
		if (i < ios_bursts.size()) {
			time_stamp = cur_time + cpu_bursts[i] + ios_bursts[i];
			cur_state = BLOCKED;
		}
		else {
			cur_state = DONE;
			completion_time = cur_time + cpu_bursts[i] - 1;		
		}
		return cur_time + cpu_bursts[i] - 1;
	}
}

int rr_process::set_running(int cur_time, int time_quanta)
{
	cur_state = RUNNING;
	if (cpu_bursts[i] - time_quanta > 0) {
		cpu_bursts[i] -= time_quanta;
		time_stamp = cur_time + time_quanta - 1;
		return cur_time + time_quanta - 1;
	}
	int temp = cpu_bursts[i];
	cpu_bursts[i] = 0;
	time_stamp = cur_time + temp - 1;
	return cur_time + temp - 1;
}

mlfq_process::mlfq_process(vector<int> input, int id)
{
	this->i = 0;
	this->id = id;
	this->cur_state = NOT_ARRIVED;

	arrival_time = input[0];
	for (int i = 1; i < input.size(); i++) {

		if (i % 2 == 1) cpu_bursts.push_back(input[i]);
		else ios_bursts.push_back(input[i]);

	}
}

void mlfq_process::update(int cur_time)
{
	if (cur_state == NOT_ARRIVED) {
		if (cur_time >= arrival_time) cur_state = READY;
		return;
	}

	if (cur_state == BLOCKED) {
		if (cur_time >= time_stamp) {
			cur_state = READY;
			i++;
		}
		return;
	}

	if (cur_state == RUNNING) {
		if (cur_time > time_stamp) {
			if (cpu_bursts[i] > 0) {
				cur_state = READY;
			}
			else {
				if (i < ios_bursts.size()) {
					cur_state = BLOCKED;
					time_stamp = ios_bursts[i] + cur_time;
				}
				else {
					cur_state = DONE;
					completion_time = cur_time;
				}
			}
		}
	}
}

int mlfq_process::cpu_done(int cur_time, int time_quanta)
{
	if (cpu_bursts[i] - time_quanta > 0) {
		cpu_bursts[i] -= time_quanta;
		return cur_time + time_quanta - 1;
	}
	else {
		if (i < ios_bursts.size()) {
			time_stamp = cur_time + cpu_bursts[i] + ios_bursts[i];
			cur_state = BLOCKED;
		}
		else {
			cur_state = DONE;
			completion_time = cur_time + cpu_bursts[i] - 1;
		}
		return cur_time + cpu_bursts[i] - 1;
	}
}


int mlfq_process::set_running(int cur_time, int time_quanta)
{
	cur_state = RUNNING;
	if (cpu_bursts[i] - time_quanta > 0) {
		cpu_bursts[i] -= time_quanta;
		time_stamp = cur_time + time_quanta - 1;
		return cur_time + time_quanta - 1;
	}
	int temp = cpu_bursts[i];
	cpu_bursts[i] = 0;
	time_stamp = cur_time + temp - 1;
	return cur_time + temp - 1;
}