#include <iostream>
#include <fstream>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>

using namespace std;

int main(int argc, char **argv)
{
	// so -1 denotes improper values
	if(argc != 6)
	{
		cout <<"usage: ./partitioner.out <path-to-file> <pattern> <search-start-position> <search-end-position> <max-chunk-size>\nprovided arguments:\n";
		for(int i = 0; i < argc; i++)
			cout << argv[i] << "\n";
		return -1;
	}

	char *file_to_search_in = argv[1];
	char *pattern_to_search_for = argv[2];
	int search_start_position = atoi(argv[3]);
	int search_end_position = atoi(argv[4]);
	int max_chunk_size = atoi(argv[5]);

	cout << "Partitioner Starting ..." << '\n' << "Searching For : " << pattern_to_search_for << " From File : " << file_to_search_in << endl;

	// pids of the children of the parent process
	pid_t childrenPid[2];

	do {
		// pid of the current process
		pid_t my_pid = getpid();
		cout << "[" << my_pid << "] Part. :: Interval : start {" << search_start_position << "} end {" << search_end_position << "}" << endl;
		
		// current chunk size of data
		int cur_chunk_size = search_end_position - search_start_position;
				
		// if its less than max_chunk_size we divide it into 2
		if (cur_chunk_size > max_chunk_size) {

			// forking the left child
			childrenPid[0] = fork();
			if (childrenPid[0] != 0) cout << "[" << my_pid << "] Part. :: Forked : left_child {" << childrenPid[0] << "}" << endl;

			// now if we are the parent fork again
			if (childrenPid[0] != 0) {
				// forking the right child
				childrenPid[1] = fork();
				if (childrenPid[1] != 0) cout << "[" << my_pid << "] Part. :: Forked : right_child {" << childrenPid[1] << "}" << endl;
			}

			// now we would have 3 process 
			
			// 1.parent who has both the children address
			// 2.left child now would have only ca[0] = 0 ca[1] we cant comment
			// 3.and right child would have ca[1] = 0
			
			// now updating the limits according to the childrens
			if (childrenPid[0] == 0) {
				search_end_position = (search_start_position + search_end_position) / 2;
			}
			else if (childrenPid[1] == 0) {
				search_start_position = (search_start_position + search_end_position) / 2;
			}
		}
		else {
			// if the chunk size is less than or equal to the max_chunk_size we spawn a searcher child
			pid_t searcherChildPid = fork();
			// now if its the parent process
			if (searcherChildPid != 0) {

				cout << ">[" << my_pid << "] Part. :: Forked : searcher_child {" << searcherChildPid << "}" << endl;
				waitpid(searcherChildPid, NULL, 0);
				cout << "[" << my_pid << "] Part. :: Returned : searcher_child" << endl;
				return 1;
			}
			else {
				// convert the child process into searcher
				string startPos = to_string(search_start_position);
				string endPos = to_string(search_end_position);

				execl("./part2_searcher.out","part2_searcher.out", file_to_search_in, pattern_to_search_for, &startPos[0], &endPos[0], (char*)NULL);
				return 1;

			}
		}
		
		// exit if we are the parent
	} while (childrenPid[0] == 0 || childrenPid[1] == 0);
	
	pid_t my_pid = getpid();

	// waiting for the left child
	waitpid(childrenPid[0],NULL,0);
	cout << "[" << my_pid << "] Part. :: Returned : left_child" << endl;

	// waiting for the right child
	waitpid(childrenPid[1],NULL,0);
	cout << "[" << my_pid << "] Part. :: Returned : right_child" << endl;
	
	return 0;

}
