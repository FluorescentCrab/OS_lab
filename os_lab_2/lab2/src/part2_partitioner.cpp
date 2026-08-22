#include <iostream>
#include <fstream>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>

using namespace std;

int main(int argc, char **argv)
{
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

	// addresses of the children of the parent process
	pid_t childrenPid[2];

	do {
		// pid of the current process
		pid_t my_pid = getpid();
		cout << "[" << my_pid << "] start position = " << search_start_position << " ; end position = " << search_end_position << "\n";
		int cur_chunk_size = search_end_position - search_start_position;
				
		if (cur_chunk_size > max_chunk_size) {
			cout << "DIVIDING THE CHUNK INTO 2 " << endl;

			// forking the left child
			childrenPid[0] = fork();
			if (childrenPid[0] != 0) cout << "[" << my_pid << "] leftChild forked = " << childrenPid[0] << endl;

			// now if we are the parent fork again
			if (childrenPid[0] != 0) {
				// forking the right child
				childrenPid[1] = fork();
				if (childrenPid[1] != 0) cout << "[" << my_pid << "] rightChild forked = " << childrenPid[1] << endl;
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

			cout << "[" << my_pid << "] chunk made " << endl;
			return 1;
		}
		
		// exit if we are the parent
	} while (childrenPid[0] == 0 || childrenPid[1] == 0);
	
	pid_t my_pid = getpid();

	// waiting for the left child
	int child0Status;
	waitpid(childrenPid[0],&child0Status,0);
	cout << "[" << my_pid << "] leftChild returned" << endl;
	if (WIFEXITED(child0Status) && WEXITSTATUS(child0Status) == 1) cout << "FOUND IN LEFT CHILD" << endl;

	int child1Status;
	waitpid(childrenPid[1],&child1Status,0);
	cout << "[" << my_pid << "] rightChild returned" << endl;
	if (WIFEXITED(child1Status) && WEXITSTATUS(child1Status) == 1) cout << "FOUND IN RIGHT CHILD" << endl;

	return 0;

	//cout << "[" << my_pid << "] forked left child " << my_children[0] << "\n";
	//cout << "[" << my_pid << "] forked right child " << my_children[1] << "\n";
	//cout << "[" << my_pid << "] left child returned\n";
	//cout << "[" << my_pid << "] right child returned\n";
	//cout << "[" << my_pid << "] left child returned\n";
	//cout << "[" << my_pid << "] right child returned\n";*/
	//cout << "[" << my_pid << "] forked searcher child " << searcher_pid << "\n";
	//cout << "[" << my_pid << "] searcher child returned \n";

}
