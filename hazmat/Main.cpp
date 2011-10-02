#include <windows.h>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <algorithm>
#include <DbgHelp.h>
#include <iterator>
#include <map>
#include "ArchiveStructure.h"
using namespace std;


int main (int argc, char* argv[]) {
    if(argc < 4){
        cerr << "usage: hazmat /IN:LibraryName.lib /OUT:QuarantinedLib.lib /NEST:ABC /X:SymbolExcludeMatch,Symbol2ExcludeMatch,...,SymbolNExcludeMatch" << endl;
        return EXIT_FAILURE;
    }

	//strings for our argument inputs
	string lib_filename("");
	string out_filename("");
	string namespace_nest("");
	string exclusions_to_tokenize("");
	vector<string> exclusions;
	
	//Create a map of strings to string references, so we can easily pass in arguments in any order
	map<string,string&> argument_to_string;
	argument_to_string.insert(pair<string,string&>("/IN:",lib_filename));
	argument_to_string.insert(pair<string,string&>("/OUT:",out_filename));
	argument_to_string.insert(pair<string,string&>("/NEST:",namespace_nest));
	argument_to_string.insert(pair<string,string&>("/X:",exclusions_to_tokenize));

	//For each of the arguments
	for(int i = 0; i < argc; ++i) {
		string arg(argv[i]);

		//split the argument on the :
		size_t split = arg.find_first_of(":") + 1;
		string param_data = arg.substr(split);
		string argument = arg.substr(0,split);
		
		//lookup the argument command in our map
		map<string,string&>::iterator iter = argument_to_string.find(argument);
		if(iter != argument_to_string.end()) {
			//update the referenced string
			(*iter).second = param_data; 
		}
	}

	//turn the exclusions string into a vector of strings
	string temp;
	istringstream iss(exclusions_to_tokenize);
	while(getline(iss,temp,',')) {
		exclusions.push_back(temp);
	}

    cout << "Loading Library : " << lib_filename << endl;
	
	cout << "Excluding symbols that substring match : " << endl;
	for_each(exclusions.begin(), exclusions.end(), [](string str) { cout << std::string(4, ' ') << str << endl; } );
	
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