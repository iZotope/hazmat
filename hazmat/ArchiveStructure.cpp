#include <windows.h>
#include <iostream>
#include <string>
#include <sstream>
#include <algorithm>
#include <fstream>
#include <vector>
#include <map>
#include <set>
#include <intrin.h>
#include <DbgHelp.h>
#include "ArchiveStructure.h"
using namespace std;

namespace {
    const unsigned int IMAGE_HEADER_NAME_SIZE = 16;
    const unsigned int IMAGE_HEADER_SIZE_SIZE = 10;
    const streamsize MAX_SYMBOL_NAME_LENGTH = 512;
    set<string> inclusive_string_set;
}

//Pad the file out and return the delta of stop - start
unsigned int PadFileStream(fstream &file, unsigned int start_position = 0) {
    int stop_position = (file.flags() | ios::in) ? static_cast<int>(file.tellg()) : static_cast<int>(file.tellp());

    if( (stop_position - start_position) % 2 ){
        char padding;
        if(file.flags() | ios::in) {
            file.read(&padding, sizeof(padding));
        } else {
            file.write(&padding, sizeof(padding));
        }
    }

    return stop_position - start_position;
}

//converts a char array that is NOT null terminated into a integer
int ConvertCStrToInt(const char* cstr, int size) {
        string str;
        str.resize(10);
        copy(cstr, cstr+size, str.begin());
        int memsize(0);
        stringstream(str) >> memsize;
        return memsize;
}

bool ArchiveEntry::ReadEntry(fstream &file) {
    TotalSize = 0;
    ZeroMemory(&header, sizeof(header));
    file.read(reinterpret_cast<char*>(&header), sizeof(header));
    TotalSize += sizeof(header);
    return true;
}

bool ArchiveEntry::WriteEntry(fstream &file) {
    int memsize( ConvertCStrToInt(reinterpret_cast<char*>(header.Size), IMAGE_HEADER_SIZE_SIZE) );
    if(memsize == 0)
        return false;
    file.write(reinterpret_cast<char*>(&header), sizeof(header));
    return true;
}

bool FirstLinkerMember::ReadEntry(fstream &file)  {
    ArchiveEntry::ReadEntry(file);

    if(strncmp(IMAGE_ARCHIVE_LINKER_MEMBER, reinterpret_cast<char*>(&header.Name), IMAGE_HEADER_NAME_SIZE) != 0) {
        cerr << "Failed to properly read first linker member" << endl;
        return false;
    }

    int start_position = static_cast<int>(file.tellg());

    file.read(reinterpret_cast<char*>(&number_of_symbols), sizeof(number_of_symbols));
    number_of_symbols = _byteswap_ulong(number_of_symbols);
    if(number_of_symbols == 0) {
        cerr << "No symbols found";
        return false;
    }

    offsets_table.resize(number_of_symbols);
    file.read(reinterpret_cast<char*>(&offsets_table[0]), sizeof(unsigned long) * number_of_symbols);

    for(unsigned int i = 0; i < number_of_symbols; ++i) {
        string symbol_name;
        getline(file, symbol_name, '\0');
        string_table.push_back(symbol_name);
        
        //when we fill the first linker member string table, also make a copy into our set for log(n) search of valid renamable symbols
        inclusive_string_set.insert(symbol_name);
    }

    TotalSize += PadFileStream(file, start_position);
    return true;
}

bool FirstLinkerMember::WriteEntry(fstream &file)  {
    ArchiveEntry::WriteEntry(file);
    
    int start_position = static_cast<int>(file.tellg());


    unsigned long number_of_symbols_flipped = _byteswap_ulong(number_of_symbols);
    file.write(reinterpret_cast<char*>(&number_of_symbols_flipped), sizeof(number_of_symbols));
    

    file.write(reinterpret_cast<char*>(&offsets_table[0]), sizeof(unsigned long) * number_of_symbols);

    for(unsigned int i = 0; i < number_of_symbols; ++i) {
        file.write(string_table[i].c_str(), string_table[i].length() + 1);
    }

    PadFileStream(file, start_position);

    return true;
}


bool SecondLinkerMember::ReadEntry(fstream &file)  {
    ArchiveEntry::ReadEntry(file);
/*
    unsigned long number_of_members;
    std::vector<unsigned long> offsets_table;
    unsigned long number_of_symbols;
    std::vector<unsigned short> indices;
    std::vector<std::string> string_table;*/

    if(strncmp(IMAGE_ARCHIVE_LINKER_MEMBER, reinterpret_cast<char*>(&header.Name), IMAGE_HEADER_NAME_SIZE) != 0) {
        cerr << "Failed to properly read first linker member" << endl;
        return false;
    }

    int start_position = static_cast<int>(file.tellg());

    file.read(reinterpret_cast<char*>(&number_of_members), sizeof(number_of_members));
    if(number_of_members == 0)
        return false;

    offsets_table.resize(number_of_members);
    file.read(reinterpret_cast<char*>(&offsets_table[0]), sizeof(unsigned long) * number_of_members);

    file.read(reinterpret_cast<char*>(&number_of_symbols), sizeof(number_of_symbols));
    
    indices.resize(number_of_symbols);
    file.read(reinterpret_cast<char*>(&indices[0]), sizeof(unsigned short) * number_of_symbols);

    for(unsigned int i = 0; i < number_of_symbols; ++i) {
        string symbol_name;
        getline(file, symbol_name, '\0');
        string_table.push_back(symbol_name);
    }

    TotalSize += PadFileStream(file, start_position);
    return true;
}


bool SecondLinkerMember::WriteEntry(fstream &file)  {
    ArchiveEntry::WriteEntry(file);

    int start_position = static_cast<int>(file.tellg());

    file.write(reinterpret_cast<char*>(&number_of_members), sizeof(number_of_members));

    file.write(reinterpret_cast<char*>(&offsets_table[0]), sizeof(unsigned long) * number_of_members);

    file.write(reinterpret_cast<char*>(&number_of_symbols), sizeof(number_of_symbols));
    
    file.write(reinterpret_cast<char*>(&indices[0]), sizeof(unsigned short) * number_of_symbols);

    for(unsigned int i = 0; i < number_of_symbols; ++i) {
        file.write(string_table[i].c_str(), string_table[i].length() + 1);
    }

    PadFileStream(file, start_position);

    return true;
}


bool LongnamesMember::ReadEntry(std::fstream &file) {
    ArchiveEntry::ReadEntry(file);

    if(strncmp(IMAGE_ARCHIVE_LONGNAMES_MEMBER, reinterpret_cast<char*>(&header.Name), IMAGE_HEADER_NAME_SIZE) != 0) {
        cerr << "Failed to properly read first linker member" << endl;
        return false;
    }

    int memsize( ConvertCStrToInt(reinterpret_cast<char*>(header.Size), IMAGE_HEADER_SIZE_SIZE) );

    int start_position = static_cast<int>(file.tellg());

    while(memsize > 0) {
        string longname;
        getline(file, longname, '\0');
        longnames_table.push_back(longname);
        memsize -= longname.size() + 1; //+1 for the \0 at the end of the string
    }
    
    TotalSize += PadFileStream(file, start_position);

    return true;
}

bool LongnamesMember::WriteEntry(std::fstream &file) {
    ArchiveEntry::WriteEntry(file);

    int start_position = static_cast<int>(file.tellg());

    for(unsigned int i = 0; i< longnames_table.size(); ++i) {
        file.write(longnames_table[i].c_str(), longnames_table[i].length() + 1);
    }

    PadFileStream(file, start_position);

    return true;
}


bool ArchiveMember::ReadEntry(std::fstream &file) {
    ArchiveEntry::ReadEntry(file);

    int memsize( ConvertCStrToInt(reinterpret_cast<char*>(header.Size), IMAGE_HEADER_SIZE_SIZE) );
    if(memsize == 0)
        return true;
    int start_position = static_cast<int>(file.tellg());

    file.read(reinterpret_cast<char*>(&file_header), sizeof(file_header));

    //from the end of the file header to the beginning of the symbol table is section data
    section_data.resize(file_header.PointerToSymbolTable-sizeof(file_header));
    file.read(reinterpret_cast<char*>(&section_data[0]), section_data.size());

    symbol_table.resize(file_header.NumberOfSymbols);
    file.read(reinterpret_cast<char*>(&symbol_table[0]), sizeof(IMAGE_SYMBOL) * file_header.NumberOfSymbols);

    file.read(reinterpret_cast<char*>(&string_table_size), sizeof(string_table_size));

    for(unsigned int i = 0; i<file_header.NumberOfSymbols; ++i) {
        if(symbol_table[i].N.Name.Short)
            continue;
        string string_table_entry;
        getline(file, string_table_entry, '\0');
        string_table.push_back(string_table_entry);
    }

    TotalSize += PadFileStream(file, start_position);

    return true;
}

bool ArchiveMember::WriteEntry(std::fstream &file) {
    if(!ArchiveEntry::WriteEntry(file))
        return false;

    int start_position = static_cast<int>(file.tellg());

    file.write(reinterpret_cast<char*>(&file_header), sizeof(file_header));
    
    file.write(reinterpret_cast<char*>(&section_data[0]), section_data.size());

    file.write(reinterpret_cast<char*>(&symbol_table[0]), sizeof(IMAGE_SYMBOL) * file_header.NumberOfSymbols);

    file.write(reinterpret_cast<char*>(&string_table_size), sizeof(string_table_size));

    for_each(string_table.begin(), string_table.end(), [&file](string str) { file.write(str.c_str(), str.length() + 1); } );

    PadFileStream(file, start_position);

    return true;
}

bool ArchiveFile::Read(fstream &libfile) {
    libfile.read(reinterpret_cast<char*>(&archstring), sizeof(char)*IMAGE_ARCHIVE_START_SIZE);

    //test the validity of the lib file by looking at the null terminated string at the front of the file
    //IMAGE_ARCHIVE_START = "!<arch>\n" in WinNT.h
    if(strncmp(IMAGE_ARCHIVE_START, archstring, IMAGE_ARCHIVE_START_SIZE) != 0) {
        std::cerr << "Failed to read the Image Archive Start string from COFF library" << std::endl;
        return false;
    }

    first_linker.ReadEntry(libfile);
    second_linker.ReadEntry(libfile);
    longnames.ReadEntry(libfile);
    
    while(!libfile.eof()) {
        members.push_back(ArchiveMember(libfile));
    }

    return true;
}

void UpdateSymbolName(string &input, const string &nest) {
    //only update symbols in the inclusive table
    if(inclusive_string_set.find(input) == inclusive_string_set.end())
        return;

    size_t find_at_at = input.find("@@");
    if(find_at_at != string::npos)
        input.replace(find_at_at, 2, "@" + nest + "@@");
    else
        input = nest + input;
}

bool ArchiveFile::Write(const string namespace_nest, fstream &libfile, vector<string> &exclusions) {
    
    //Start by removing explicetly excluded symbols from our inclusive set of renameable symbols
    for_each(exclusions.begin(), exclusions.end(), [](string str) { inclusive_string_set.erase(str); });

    //Update tables

    //start by updating the members
    for(unsigned int i = 0; i< members.size(); ++i) {
        
        //update the strings in the string tables

        //start the offset at 4 bytes, since the first 4 bytes of the string table is actually the string table size
        unsigned int offset = 4;

        //update the symbol long pointers
        unsigned int k = 0;
        for(unsigned int j = 0; j< members[i].symbol_table.size(); ++j) {
            //If the short name is non-zero, it doesn't have a string entry
            if(members[i].symbol_table[j].N.Name.Short) {
                //do i need to update the short names???
                continue;
            } else {
                UpdateSymbolName(members[i].string_table[k], namespace_nest);

                members[i].symbol_table[j].N.Name.Long = offset;
                offset += members[i].string_table[k].length() + 1;
                ++k;
            }
        }

        unsigned int size_delta = offset - members[i].string_table_size;

        //update the total size for string table 
        members[i].string_table_size += size_delta;

        //update member header data size value
        members[i].TotalSize += size_delta;
        stringstream datasize;
        datasize << members[i].TotalSize - sizeof(members[i].header); //The datasize field in the header does not include the size of the header itself
        memcpy(&members[i].header.Size,datasize.str().c_str(), datasize.str().length());
    }

    //update second linker symbol names and data size value
    {
        unsigned int offset = 0;
        for(unsigned int i = 0; i < second_linker.string_table.size(); ++i) {
            second_linker.TotalSize -= (second_linker.string_table[i].length() + 1);

            UpdateSymbolName(second_linker.string_table[i], namespace_nest);
            offset += second_linker.string_table[i].length() + 1;
        }

        second_linker.TotalSize += offset;

        stringstream datasize;
        datasize << second_linker.TotalSize - sizeof(second_linker.header); //The datasize field in the header does not include the size of the header itself
        memcpy(&second_linker.header.Size,datasize.str().c_str(), datasize.str().length());
    }


    //update first linker symbol names and data size value
    {
        unsigned int offset = 0;
        for(unsigned int i = 0; i< first_linker.string_table.size(); ++i) {
            first_linker.TotalSize -= (first_linker.string_table[i].length() + 1);

            UpdateSymbolName(first_linker.string_table[i], namespace_nest);
            offset += first_linker.string_table[i].length() + 1;
        }

        first_linker.TotalSize += offset;

        stringstream datasize;
        datasize << first_linker.TotalSize - sizeof(first_linker.header); //The datasize field in the header does not include the size of the header itself
        memcpy(&first_linker.header.Size,datasize.str().c_str(), datasize.str().length());
    }

    //Write out everything, and fill a temp offset table for writing at the end
    vector<unsigned long> temp_offset_table(second_linker.number_of_members);

    //Write Image Archive start string
    libfile.write(reinterpret_cast<char*>(&archstring), sizeof(char)*IMAGE_ARCHIVE_START_SIZE);
    
    //Write first linker member entry
    first_linker.WriteEntry(libfile);

    //Grab the offset to the second linker table offset table to update at the end
    unsigned int second_linker_offset_table_offset = static_cast<unsigned int>(libfile.tellp()) + sizeof(IMAGE_ARCHIVE_MEMBER_HEADER) + sizeof(unsigned long);
    
    //Write the second linker member etry
    second_linker.WriteEntry(libfile);

    //Write the longnames member
    longnames.WriteEntry(libfile);
    
    //Write all obj members and headers
    for(unsigned int i = 0; i< members.size()-1; ++i) {
        temp_offset_table[i] = static_cast<unsigned long>(libfile.tellp());
        members[i].WriteEntry(libfile);
    }

    //seek back and write the offset table, only in to the second linker member since the first member is unused
    libfile.seekp(second_linker_offset_table_offset);
    libfile.write(reinterpret_cast<char*>(&temp_offset_table[0]), temp_offset_table.size() * sizeof(unsigned long));

    return true;
}
