#include <iostream>
#include <fstream>
#include <cstring>
#include <unistd.h>
#include <signal.h>

using namespace std;

int main(int argc, char **argv)
{
	if(argc != 5)
	{
		cout <<"usage: ./partitioner.out <path-to-file> <pattern> <search-start-position> <search-end-position>\nprovided arguments:\n";
		for(int i = 0; i < argc; i++)
			cout << argv[i] << "\n";
		return -1;
	}
	
	char *file_to_search_in = argv[1];
	char *pattern_to_search_for = argv[2];
	int search_start_position = atoi(argv[3]);
	int search_end_position = atoi(argv[4]);

	// opening file 
	fstream filein(file_to_search_in, ios::in);

	if (!filein.is_open()) {
		cout << "ERROR : FILE DID NOT OPEN" << endl;
		return 2;
	}
	cout << "FILE OPENED SUCCESSFULLY" << endl;
	
	// okay so loading entire thing in a string in the memory 
	int size = search_end_position - search_start_position;
	string content(size+1, '\0');
	filein.seekg(0);
	int k;

	// setting up the flag
	bool found = false;

	// checking if the conetent is properly copied in the string
	if (!filein.read(&content[0], size)) {
		cout << "ERROR : COULD NOT LOAD THE FILE CONTENT INTO THE STRING" << endl;
		return 3;
	}
	else {
		cout << "CONTENT LOADED SUCCESSFULLY" << endl;

		// checking if there exists a substsring that contains the pattern
		k = content.find(pattern_to_search_for);

		if (k != content.npos) found = true;
	}

	// closing the file
	filein.close();
	cout << "FILE CLOSED" << endl;
	
	if (!found) {
		cout << "[-1] didn't find\n";
		return 0;
	}
	else {
		cout << "PATTERN :" << pattern_to_search_for << " :: FOUND @ : " << search_start_position + k << endl;
		return 1;
	}
}
