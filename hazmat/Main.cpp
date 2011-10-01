#include <windows.h>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <algorithm>
#include <DbgHelp.h>
#include <iterator>
#include "ArchiveStructure.h"
using namespace std;


int main (int argc, char* argv[]) {
    if(argc < 4){
        cerr << "usage: hazmat /IN:LibraryName.lib /OUT:QuarantinedLib.lib /NEST:ABC /X:SymbolExcludeMatch,Symbol2ExcludeMatch,...,SymbolNExcludeMatch" << endl;
        return EXIT_FAILURE;
    }

	string lib_filename("");
	string out_filename("");
	string namespace_nest("");
	vector<string> exclusions;

	for(int i = 0; i < argc; ++i) {
		string arg(argv[i]);

		size_t command = arg.find("/IN:");
		if(command != string::npos) {
			size_t split = arg.find_first_of(":") + 1;
			lib_filename = arg.substr(split);
			continue;
		} 
		//refactor city, population you
		command = arg.find("/OUT:");
		if(command != string::npos) {
			size_t split = arg.find_first_of(":") + 1;
			out_filename = arg.substr(split);
			continue;
		} 

		command = arg.find("/NEST:");
		if(command != string::npos) {
			size_t split = arg.find_first_of(":") + 1;
			namespace_nest = arg.substr(split);
			continue;
		} 

		command = arg.find("/X:");
		if(command != string::npos) {
			size_t split = arg.find_first_of(":") + 1;
			string temp;
			istringstream iss(arg.substr(split));
			while(getline(iss,temp,',')) {
				exclusions.push_back(temp);
			}
			continue;
		} 
	}


    cout << "Loading Library : " << lib_filename << endl;
	
	cout << "Excluding symbols that substring match : " << endl;
	copy(exclusions.begin(), exclusions.end(), ostream_iterator<string>(cout, "\n"));

    //open the file as a binary input stream
    fstream libfile;
    libfile.open(lib_filename, ios::binary | ios::in );
    if(libfile.fail()) {
        cerr << "Failed to open library" << endl;
        cin.get();
        return EXIT_FAILURE;
    }

	ArchiveFile archive;
	
	archive.Read(libfile);
	libfile.close();

	fstream out_file;
	out_file.open(out_filename, ios::binary | ios::out);
	archive.Write(namespace_nest, out_file, exclusions);
	out_file.close();

    return EXIT_SUCCESS;
}