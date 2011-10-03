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
    string in_filename("");
    string out_filename("");
    string namespace_nest("");
    string exclusions_to_tokenize("");
    vector<string> exclusions;
    
    //Create a map of strings to string references, so we can easily pass in arguments in any order
    map<string,string&> argument_to_string;
    argument_to_string.insert(pair<string,string&>("/IN:",in_filename));
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
        auto iter = argument_to_string.find(argument);
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

    cout << "Loading Library : " << in_filename << endl;
    
    cout << "Excluding symbols that substring match : " << endl;
    for_each(exclusions.begin(), exclusions.end(), [](string str) { cout << std::string(4, ' ') << str << endl; } );
    
    //open the input file as a binary input stream
    fstream lib_in_file;
    lib_in_file.open(in_filename, ios::binary | ios::in );
    if(lib_in_file.fail()) {
        cerr << "Failed to open input library " << in_filename << endl;
        return EXIT_FAILURE;
    }

    //open the output file as a binary output stream
    fstream lib_out_file;
    lib_out_file.open(out_filename, ios::binary | ios::out);
    if(lib_in_file.fail()) {
        cerr << "Failed to open output library " << out_filename << endl;
        return EXIT_FAILURE;
    }


    ArchiveFile archive;
    
    //parse the archive file
    archive.Read(lib_in_file);

    //Nest the all the symbols except those in 'exclusions' into namespace_nest
    archive.NestSymbols(namespace_nest, exclusions);

    //write the altered archive file out
    archive.Write(lib_out_file);
    
    lib_in_file.close();
    lib_out_file.close();

    return EXIT_SUCCESS;
}