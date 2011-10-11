// ArchiveStructure.cpp
// Copyright (c) 2011 iZotope Inc.

#include "stdafx.h"
#include "ArchiveStructure.h"
using namespace std;

namespace {
    const unsigned int IMAGE_HEADER_NAME_SIZE = 16u;
    const unsigned int IMAGE_HEADER_SIZE_SIZE = 10u;
    const streamsize MAX_SYMBOL_NAME_LENGTH = 512;

    //the inclusive string set consists of every string
    // in the string table of the first member, which
    // encompasses all symbols that the linker can find
    // in this library
    set<string> inclusive_string_set;

    // Pad the file out and return the delta of stop - start
    unsigned int PadFileStream(fstream * const file, unsigned int start_position = 0) {
        int stop_position = (file->flags() | ios::in) ? static_cast<int>(file->tellg()) : static_cast<int>(file->tellp());

        if( (stop_position - start_position) % 2 ){
            char padding(10);
            if(file->flags() | ios::in) {
                file->read(&padding, sizeof(padding));
            } else {
                file->write(&padding, sizeof(padding));
            }
        }

        return stop_position - start_position;
    }

    // converts a char array that is NOT null terminated into a integer
    int ConvertCStrToInt(const char* cstr, int size) {
        //create a string of the target size and copy the cstr into it
        string str;
        str.resize(size);
        copy(cstr, cstr+size, str.begin());

        //use stringstream to convert to integer
        int memsize(0);
        stringstream(str) >> memsize;
        return memsize;
    }

    // Iterate through a vector of functions applying each function to the edit_string
    void ApplyStringEditFunctions(string * const edit_string, vector<function<void(string * const)> > edit_funcs) {
        for_each(edit_funcs.begin(), edit_funcs.end(), 
            [edit_string](function<void(string * const)> func) {
                func(edit_string);
            }
        );
    }
}


// Base archive ReadEntry reads the header
bool ArchiveEntry::ReadEntry(fstream * const file) {
    TotalSize = 0;
    ZeroMemory(&header, sizeof(header));
    file->read(reinterpret_cast<char*>(&header), sizeof(header));
    TotalSize += sizeof(header);
    return true;
}

// Base archive WriteEntry writes the header
bool ArchiveEntry::WriteEntry(fstream * const file) {
    int memsize( ConvertCStrToInt(reinterpret_cast<char*>(header.Size), IMAGE_HEADER_SIZE_SIZE) );
    if(memsize == 0)
        return false;
    file->write(reinterpret_cast<char*>(&header), sizeof(header));
    return true;
}

// 1st linker member read entry
bool FirstLinkerMember::ReadEntry(fstream * const file)  {
    ArchiveEntry::ReadEntry(file);

    // 1st linker member has the name /
    if(strncmp(IMAGE_ARCHIVE_LINKER_MEMBER, reinterpret_cast<char*>(&header.Name[0]), IMAGE_HEADER_NAME_SIZE) != 0) {
        cerr << "Failed to properly read first linker member" << endl;
        return false;
    }

    int start_position = static_cast<int>(file->tellg());

    file->read(reinterpret_cast<char*>(&number_of_symbols), sizeof(number_of_symbols));
    // number of symbols field needs to be endian swapped
    number_of_symbols = _byteswap_ulong(number_of_symbols);
    if(number_of_symbols == 0) {
        cerr << "No symbols found";
        return false;
    }

    // after the number of symbols entry, there is a table containing offsets that point to
    //  the the location of each obj file that the corresponding symbol is contained in
    //  ie. offset 0 is the obj file that contains symbol name 0
    offsets_table.resize(number_of_symbols);
    file->read(reinterpret_cast<char*>(&offsets_table[0]), sizeof(unsigned long) * number_of_symbols);

    // after the offset table is the string table containing all symbol names for this library
    for(unsigned int i = 0; i < number_of_symbols; ++i) {
        string symbol_name;
        getline(*file, symbol_name, '\0');
        string_table.push_back(symbol_name);
        
        // when we fill the first linker member string table, also make a copy into our set for log(n) search of valid renamable symbols
        inclusive_string_set.insert(symbol_name);
    }

    TotalSize += PadFileStream(file, start_position);
    return true;
}

// 1st linker member entry write
bool FirstLinkerMember::WriteEntry(fstream * const file)  {
    ArchiveEntry::WriteEntry(file);
    
    int start_position = static_cast<int>(file->tellg());

    // endian swap the number of symbols and write it out
    unsigned long number_of_symbols_flipped = _byteswap_ulong(number_of_symbols);
    file->write(reinterpret_cast<char*>(&number_of_symbols_flipped), sizeof(number_of_symbols));
    
    // write offset table
    file->write(reinterpret_cast<char*>(&offsets_table[0]), sizeof(unsigned long) * number_of_symbols);

    // write string table
    for_each(string_table.begin(), string_table.end(), [file](string str) {
        file->write(str.c_str(), str.length() + 1);
    });

    PadFileStream(file, start_position);

    return true;
}

// 2nd linker member entry read
bool SecondLinkerMember::ReadEntry(fstream * const file)  {
    ArchiveEntry::ReadEntry(file);
/*
    unsigned long number_of_members;
    std::vector<unsigned long> offsets_table;
    unsigned long number_of_symbols;
    std::vector<unsigned short> indices;
    std::vector<std::string> string_table;*/

    // 2nd linker member has the name /
    if(strncmp(IMAGE_ARCHIVE_LINKER_MEMBER, reinterpret_cast<char*>(&header.Name[0]), IMAGE_HEADER_NAME_SIZE) != 0) {
        cerr << "Failed to properly read second linker member" << endl;
        return false;
    }

    int start_position = static_cast<int>(file->tellg());

    // Read the number of members
    file->read(reinterpret_cast<char*>(&number_of_members), sizeof(number_of_members));
    if(number_of_members == 0)
        return false;

    // After the number of members is the offset table
    offsets_table.resize(number_of_members);
    file->read(reinterpret_cast<char*>(&offsets_table[0]), sizeof(unsigned long) * number_of_members);

    // Number of symbols in the 2nd linker member is not endian swapped
    file->read(reinterpret_cast<char*>(&number_of_symbols), sizeof(number_of_symbols));
    
    // The indices table maps the offset table to the string table
    indices.resize(number_of_symbols);
    file->read(reinterpret_cast<char*>(&indices[0]), sizeof(unsigned short) * number_of_symbols);

    // after the indices table is the symbol string table
    for(unsigned int i = 0; i < number_of_symbols; ++i) {
        string symbol_name;
        getline(*file, symbol_name, '\0');
        string_table.push_back(symbol_name);
    }

    TotalSize += PadFileStream(file, start_position);
    return true;
}

// 2nd linker member entry write
bool SecondLinkerMember::WriteEntry(fstream * const file)  {
    ArchiveEntry::WriteEntry(file);

    int start_position = static_cast<int>(file->tellg());

    file->write(reinterpret_cast<char*>(&number_of_members), sizeof(number_of_members));

    file->write(reinterpret_cast<char*>(&offsets_table[0]), sizeof(unsigned long) * number_of_members);

    file->write(reinterpret_cast<char*>(&number_of_symbols), sizeof(number_of_symbols));
    
    file->write(reinterpret_cast<char*>(&indices[0]), sizeof(unsigned short) * number_of_symbols);

    for_each(string_table.begin(), string_table.end(), [file](string str) {
        file->write(str.c_str(), str.length() + 1);
    });

    PadFileStream(file, start_position);

    return true;
}

// long name member entry read
bool LongnamesMember::ReadEntry(fstream * const file) {
    ArchiveEntry::ReadEntry(file);

    // long name member is called //
    if(strncmp(IMAGE_ARCHIVE_LONGNAMES_MEMBER, reinterpret_cast<char*>(&header.Name[0]), IMAGE_HEADER_NAME_SIZE) != 0) {
        cerr << "Failed to properly read first linker member" << endl;
        return false;
    }

    // get the total size of the data associated with the long name member
    int memsize( ConvertCStrToInt(reinterpret_cast<char*>(header.Size), IMAGE_HEADER_SIZE_SIZE) );

    int start_position = static_cast<int>(file->tellg());

    // grab each string in the long name table
    while(memsize > 0) {
        string longname;
        getline(*file, longname, '\0');
        longnames_table.push_back(longname);
        memsize -= longname.size() + 1; // +1 for the \0 at the end of the string
    }
    
    TotalSize += PadFileStream(file, start_position);

    return true;
}

// long name member entry write
bool LongnamesMember::WriteEntry(fstream * const file) {
    ArchiveEntry::WriteEntry(file);

    int start_position = static_cast<int>(file->tellg());

    // write the table of long names
    for_each(longnames_table.begin(), longnames_table.end(), [file](string str) {
            file->write(str.c_str(), str.length() + 1); 
    });

    PadFileStream(file, start_position);

    return true;
}


// obj member entry read
bool ArchiveMember::ReadEntry(fstream * const file) {
    ArchiveEntry::ReadEntry(file);

    // check to see if the data size for this obj file is 0
    int memsize( ConvertCStrToInt(reinterpret_cast<char*>(&header.Size[0]), IMAGE_HEADER_SIZE_SIZE) );
    if(memsize == 0)
        return true;
    int start_position = static_cast<int>(file->tellg());

    // there is an IMAGE_FILE_HEADER at the beginning of the obj data
    file->read(reinterpret_cast<char*>(&file_header), sizeof(file_header));

    // from the end of the file header to the beginning of the symbol table is section data
    section_data.resize(file_header.PointerToSymbolTable-sizeof(file_header));
    file->read(reinterpret_cast<char*>(&section_data[0]), section_data.size());

    // after the section data is a symbol table
    symbol_table.resize(file_header.NumberOfSymbols);
    file->read(reinterpret_cast<char*>(&symbol_table[0]), sizeof(IMAGE_SYMBOL) * file_header.NumberOfSymbols);

    // the first 4 bytes of the string table represents the size of the string table
    file->read(reinterpret_cast<char*>(&string_table_size), sizeof(string_table_size));

    // go through each entry in the symbol table
    for(unsigned int i = 0; i<file_header.NumberOfSymbols; ++i) {
        // if the short symbol name is non-zero, there is no entry in the string table
        if(symbol_table[i].N.Name.Short)
            continue;
        string string_table_entry;
        getline(*file, string_table_entry, '\0');
        string_table.push_back(string_table_entry);
    }

    TotalSize += PadFileStream(file, start_position);

    return true;
}

// obj member entry write
bool ArchiveMember::WriteEntry(fstream * const file) {
    if(!ArchiveEntry::WriteEntry(file))
        return false;

    int start_position = static_cast<int>(file->tellg());

    file->write(reinterpret_cast<char*>(&file_header), sizeof(file_header));
    
    file->write(reinterpret_cast<char*>(&section_data[0]), section_data.size());

    file->write(reinterpret_cast<char*>(&symbol_table[0]), sizeof(IMAGE_SYMBOL) * file_header.NumberOfSymbols);

    file->write(reinterpret_cast<char*>(&string_table_size), sizeof(string_table_size));

    for_each(string_table.begin(), string_table.end(), [&file](string str) { file->write(str.c_str(), str.length() + 1); } );

    PadFileStream(file, start_position);

    return true;
}

// Archive file read 
bool ArchiveFile::Read(fstream * const libfile) {
    // read the arch string
    libfile->read(reinterpret_cast<char*>(&archstring[0]), sizeof(char)*IMAGE_ARCHIVE_START_SIZE);

    // test the validity of the lib file by looking at the null terminated string at the front of the file
    // IMAGE_ARCHIVE_START = "!<arch>\n" in WinNT.h
    if(strncmp(IMAGE_ARCHIVE_START, archstring, IMAGE_ARCHIVE_START_SIZE) != 0) {
        std::cerr << "Failed to read the Image Archive Start string from COFF library" << std::endl;
        return false;
    }

    // read the 1st, 2nd and long name members
    first_linker.ReadEntry(libfile);
    second_linker.ReadEntry(libfile);
    long_names.ReadEntry(libfile);
    
    // read obj members until there is no more file to read
    while(!libfile->eof()) {
        members.push_back(ArchiveMember(libfile));
    }

    return true;
}

// Nest the symbols in the parse archive file into namespace_nest
bool ArchiveFile::EditSymbolNames(const vector<string> &exclusions, vector<function<void (string * const edit_string)> > update_symbol_name_funcs) {

    // Start by removing explicitly excluded symbols from our inclusive set of re nameable symbols
    for_each(exclusions.begin(), exclusions.end(), [](string str) { inclusive_string_set.erase(str); });

    // Update tables

    // start by updating the members
    for(unsigned int i = 0; i< members.size(); ++i) {

        // update the strings in the string tables

        // start the offset at 4 bytes, since the first 4 bytes of the string table is actually the string table size
        unsigned int offset = 4;

        // update the symbol long pointers
        unsigned int k = 0;
        for(unsigned int j = 0; j< members[i].symbol_table.size(); ++j) {
            // If the short name is non-zero, it doesn't have a string entry
            if(members[i].symbol_table[j].N.Name.Short) {
                continue;
            } else {
                // update the symbol name
                if(inclusive_string_set.find(members[i].string_table[k]) != inclusive_string_set.end()) {
                    ApplyStringEditFunctions(&members[i].string_table[k], update_symbol_name_funcs);
                }

                // update the offset to the entry in the string table for this symbol
                members[i].symbol_table[j].N.Name.Long = offset;
                offset += members[i].string_table[k].length() + 1;
                ++k;
            }
        }

        unsigned int size_delta = offset - members[i].string_table_size;

        // update the total size for string table 
        members[i].string_table_size += size_delta;

        // update member header data size value
        members[i].TotalSize += size_delta;
        stringstream datasize;
        datasize << members[i].TotalSize - sizeof(members[i].header); // The datasize field in the header does not include the size of the header itself
        memcpy(&members[i].header.Size,datasize.str().c_str(), datasize.str().length());
    }

    // update second linker symbol names and data size value
    {
        unsigned int offset = 0;
        for(unsigned int i = 0; i < second_linker.string_table.size(); ++i) {
            second_linker.TotalSize -= (second_linker.string_table[i].length() + 1);
            if(inclusive_string_set.find(second_linker.string_table[i]) != inclusive_string_set.end()) {
                ApplyStringEditFunctions(&second_linker.string_table[i], update_symbol_name_funcs);
            }
            offset += second_linker.string_table[i].length() + 1;
        }

        second_linker.TotalSize += offset;

        stringstream datasize;
        datasize << second_linker.TotalSize - sizeof(second_linker.header); // The datasize field in the header does not include the size of the header itself
        memcpy(&second_linker.header.Size,datasize.str().c_str(), datasize.str().length());
    }


    // update first linker symbol names and data size value
    {
        unsigned int offset = 0;
        for(unsigned int i = 0; i< first_linker.string_table.size(); ++i) {
            first_linker.TotalSize -= (first_linker.string_table[i].length() + 1);
            if(inclusive_string_set.find(first_linker.string_table[i]) != inclusive_string_set.end()) {
                ApplyStringEditFunctions(&first_linker.string_table[i], update_symbol_name_funcs);
            }
            offset += first_linker.string_table[i].length() + 1;
        }

        first_linker.TotalSize += offset;

        stringstream datasize;
        datasize << first_linker.TotalSize - sizeof(first_linker.header); // The datasize field in the header does not include the size of the header itself
        memcpy(&first_linker.header.Size, datasize.str().c_str(), datasize.str().length());
    }

    return true;
}

// archive file write
bool ArchiveFile::Write(fstream * const libfile) {
    // Write out everything, and fill a temp offset table for writing at the end
    vector<unsigned long> temp_offset_table(second_linker.number_of_members);

    // Write Image Archive start string
    libfile->write(reinterpret_cast<char*>(&archstring), sizeof(char)*IMAGE_ARCHIVE_START_SIZE);
    
    // Write first linker member entry
    first_linker.WriteEntry(libfile);

    // Grab the offset to the second linker table offset table to update at the end
    unsigned int second_linker_offset_table_offset = static_cast<unsigned int>(libfile->tellp()) + sizeof(IMAGE_ARCHIVE_MEMBER_HEADER) + sizeof(unsigned long);
    
    // Write the second linker member entry
    second_linker.WriteEntry(libfile);

    // Write the longnames member
    long_names.WriteEntry(libfile);
    
    // Write all obj members and headers
    for(unsigned int i = 0; i< members.size()-1; ++i) {
        temp_offset_table[i] = static_cast<unsigned long>(libfile->tellp());
        members[i].WriteEntry(libfile);
    }

    // seek back and write the offset table, only in to the second linker member since the first member is unused
    libfile->seekp(second_linker_offset_table_offset);
    libfile->write(reinterpret_cast<char*>(&temp_offset_table[0]), temp_offset_table.size() * sizeof(unsigned long));

    return true;
}
