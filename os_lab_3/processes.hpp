#include <iostream>
#include <vector>

using namespace std;

// defines different states of the process
enum process_state { BLOCKED, RUNNING, READY, NOT_ARRIVED, DONE };

//======================================================================================================================
// base class for other kind of processes
class process {
protected:
	// process id
	int id;
	// current state of the process
	process_state cur_state;
	// cpu bursts times
	vector<int> cpu_bursts;
	// ios bursts times
	vector<int> ios_bursts;
	// process arrival time
	int arrival_time;
	// process completion time
	int completion_time;
public :
	process();	
	process(vector<int> input, int id);
	int getpid() const;
	int get_turn_around_time();
	virtual ~process() = default;
	// returns the current process state
	process_state get_state();

};

//======================================================================================================================

class fifo_process : public process {
private :
	// which cpu/io burst are we on
	int i;
	// when next time will we come back from ios
	int time_stamp;
public:
	fifo_process(vector<int> input, int id);

	// update the state
	void update(int cur_time);

	// does cpu work and returns the elapsed time when cpu will be done with the process
	int cpu_done(int cur_time);

	int set_running(int cur_time);

	int get_burst();
};

//======================================================================================================================

class rr_process : public process {
private:
	// refer fifo def.s
	int i;
	int time_stamp;
public :
	rr_process(vector<int> ipnut, int id);

	// update the state
	void update(int cur_time);

	// cpu work
	int cpu_done(int cur_time,int time_quanta);


	int set_running(int cur_time, int time_quanta);

	int get_burst();
};

//======================================================================================================================

// so here are the mlfq rules 
//	Rule1: If Priority(A) > Priority(B), A runs(B doesn’t).
//  Rule2 : If Priority(A) = Priority(B), A& B run in RR.
//  Rule 3 : When a job enters the system, it is placed at the highest priority(the topmost queue).
//  Rule 4 : Once a job uses up its time allotment at a given level(re
//	gardless of how many times it has given up the CPU), its priority is
//	reduced(i.e., it moves down one queue).
//  Rule 5 : After some time period S, move all the jobs in the system
//	to the topmost queue.

// so one more thing i wanna note there will be two ways : 1. when everything is reset after time S ; 2. when we dont 
// also we have to somehow store time_used <- i think to store it in the upper level function not here

class mlfq_process : public process {
private:
	// refer fifo def.s
	int i;
	int time_stamp;
public:
	mlfq_process(vector<int> input, int id);

	// update the state
	void update(int cur_time);

	// cpu work
	int cpu_done(int cur_time, int time_quanta);


	int set_running(int cur_time, int time_quanta);

	int get_burst();
};