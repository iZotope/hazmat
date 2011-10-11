// Hazmat
//   A tool for putting all the symbols in a static library into a namespace,
//   or rename namespaces and classes to protect you from hazardous name conflicts
//
// Written by Jasper Duba
// Copyright (c) 2011 iZotope Inc.

#include "stdafx.h"
#include "ArchiveStructure.h"
using namespace std;

namespace {
    const string VALID_COMMANDS[] = {   "/IN",
                                        "/OUT",
                                        "/NEST",
                                        "/X"};

    const string TAB = string(4, ' ');
}


// parse the command line arguments and build a map of valid
//  command/argument pairs
bool HandleCommandLineArgs(const int argc, const char* const argv[], map<string, string> *argument_to_string) {
    //create a set of valid commands
    set<string> valid_commands;
    valid_commands.insert(VALID_COMMANDS, VALID_COMMANDS + (sizeof(VALID_COMMANDS)/sizeof(VALID_COMMANDS[0])));
    
    // For each of the arguments
    for (int i = 1; i < argc; ++i) {
        string arg(argv[i]);

        // split the argument on the :
        size_t split = arg.find_first_of(":") + 1;
        string param_data = arg.substr(split);
        string argument = arg.substr(0, split-1);

        if(valid_commands.find(argument) == valid_commands.end()) {
            cerr << "Unrecognized option " << argument << endl;
            return false;
        }

        argument_to_string->insert(pair<string, string>(argument,param_data));
    }

    //Check for required arguments
    if(argument_to_string->find("/IN") == argument_to_string->end()) {
        cerr << "Missing input library command \"/IN:input.lib\"" << endl;
        return false;
    }
    if(argument_to_string->find("/OUT") == argument_to_string->end() ) {
        cerr << "Missing output library command \"/OUT:output.lib\"" << endl;
        return false;
    }

    return true;
}

// Print basic usage, plus all options in VALID_COMMANDS
void PrintUsage() {
    cerr << "usage: hazmat /IN:input.lib /OUT:output.lib [options] " << endl;
    cerr << endl << "Options:" << endl;
    for_each(VALID_COMMANDS, VALID_COMMANDS + (sizeof(VALID_COMMANDS)/sizeof(VALID_COMMANDS[0])),
        [](string command_str) {
            cerr << TAB  << command_str << endl;
        }
    );
}

int main(int argc, char* argv[]) {
    map<string, string> valid_arguments;

    // if there are invalid arguments, valid arguments size is zero
    if (!HandleCommandLineArgs(argc, argv, &valid_arguments)) {
        PrintUsage();
        return EXIT_FAILURE;
    }

    string in_filename = valid_arguments["/IN"];
    string out_filename = valid_arguments["/OUT"];
    string namespace_nest = valid_arguments["/NEST"];
    string hierarchy_replace = valid_arguments["/REPLACE"];
    string exclusions_to_tokenize = valid_arguments["/X"];

    
    cout << "Loading Library : " << in_filename << endl;

    // turn the exclusions string into a vector of strings
    vector<string> exclusions;
    if (exclusions_to_tokenize.length() > 0) {
        string temp;
        istringstream iss(exclusions_to_tokenize);
        while (getline(iss, temp, ',')) {
            exclusions.push_back(temp);
        }
    }

    // create a vector of lambdas to apply in order to the symbol names
    vector<function<void (string * const edit_string)> > symbol_name_edit_functions;

    // Namespace nest command
    if(namespace_nest.length() > 0) {
        cout << "Nesting symbols into namespace " << namespace_nest << endl;

        // add the lambda for nesting a namespace to the edit list
        symbol_name_edit_functions.push_back(

            [namespace_nest](string * const str) {
                //find the first @@ which indicates the end of the mangled name
                size_t find_at_at = str->find("@@");
                if(find_at_at != string::npos) {
                    // replace @@ with @nest@@
                    str->replace(find_at_at, 2, "@" + namespace_nest + "@@");
                } else {
                    // if there is no @@, prepend the nest name
                    *str = namespace_nest + *str;
                }
            }

        );
    }


    if(exclusions.size() > 0) {
        cout << "Excluding symbols that match : " << endl;
        for_each(exclusions.begin(), exclusions.end(), [](string str) { cout << TAB << str << endl; });
    }

    // open the input file as a binary input stream
    fstream lib_in_file;
    lib_in_file.open(in_filename, ios::binary | ios::in);
    if (lib_in_file.fail()) {
        cerr << "Failed to open input library " << in_filename << endl;
        return EXIT_FAILURE;
    }

    // open the output file as a binary output stream
    fstream lib_out_file;
    lib_out_file.open(out_filename, ios::binary | ios::out);
    if (lib_in_file.fail()) {
        cerr << "Failed to open output library " << out_filename << endl;
        return EXIT_FAILURE;
    }


    ArchiveFile archive;

    // parse the archive file
    archive.Read(&lib_in_file);

    // Nest the all the symbols except those in 'exclusions' into namespace_nest
    archive.EditSymbolNames(exclusions, symbol_name_edit_functions);

    cout << "Writing Library : " << out_filename << endl;
    // write the altered archive file out
    archive.Write(&lib_out_file);

    lib_in_file.close();
    lib_out_file.close();

    return EXIT_SUCCESS;
}
