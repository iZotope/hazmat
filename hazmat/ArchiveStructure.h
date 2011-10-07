#pragma once


// Microsoft Archive (Lib) format looks like this (more details at http://msdn.microsoft.com/en-us/windows/hardware/gg463119)
    // Signature :”!<arch>\n”
    // Header
    // 1st Linker Member
    // Header
    // 2nd Linker Member
    // Header
    // Longnames Member
    // Header
    // Contents of OBJ File 1 (COFF format)
    // Header
    // Contents of OBJ File 2 (COFF format)
    // ...
    // Header
    // Contents of OBJ File N (COFF format)


// Base struct for an entry in the archive file
struct ArchiveEntry {
    ArchiveEntry() : TotalSize(0) { ZeroMemory(&header, sizeof(header)); }
    virtual ~ArchiveEntry() {}

    // All members have a IMAGE_ARCHIVE_MEMBER_HEADER
    IMAGE_ARCHIVE_MEMBER_HEADER header;

    // Every archive member should be able to read from a filestream and write to a filestream
    virtual bool ReadEntry(std::fstream * const file);
    virtual bool WriteEntry(std::fstream * const file);

    // Total size of the entire entry
    unsigned long TotalSize;
};

// 1st linker member
struct FirstLinkerMember : public ArchiveEntry {
    FirstLinkerMember() : number_of_symbols(0) {}
    virtual ~FirstLinkerMember() {}

    unsigned long number_of_symbols;
    std::vector<unsigned long> offsets_table;
    std::vector<std::string> string_table;

    virtual bool ReadEntry(std::fstream * const file);
    virtual bool WriteEntry(std::fstream * const file);
};

// 2nd linker member
struct SecondLinkerMember : public ArchiveEntry  {
    SecondLinkerMember() : number_of_members(0), number_of_symbols(0) {}
    virtual ~SecondLinkerMember() {}

    unsigned long number_of_members;
    std::vector<unsigned long> offsets_table;
    unsigned long number_of_symbols;
    std::vector<unsigned short> indices;
    std::vector<std::string> string_table;

    virtual bool ReadEntry(std::fstream * const file);
    virtual bool WriteEntry(std::fstream * const file);
};

// Long names member
struct LongnamesMember  : public ArchiveEntry {
    LongnamesMember() {}
    virtual ~LongnamesMember() {}

    std::vector<std::string> longnames_table;

    virtual bool ReadEntry(std::fstream * const file);
    virtual bool WriteEntry(std::fstream * const file);
};

// Contents of obj file
struct ArchiveMember : public ArchiveEntry {
    ArchiveMember(std::fstream * const file) :
        string_table_size(0) 
        {
            ArchiveMember::ReadEntry(file);
        }
    virtual ~ArchiveMember() {}

    IMAGE_FILE_HEADER file_header;
    std::vector<byte> section_data;
    std::vector<IMAGE_SYMBOL> symbol_table;
    unsigned int string_table_size;
    std::vector<std::string> string_table;

    virtual bool ReadEntry(std::fstream * const file);
    virtual bool WriteEntry(std::fstream * const file);
};


// A struct to hold the entire parsed lib
struct ArchiveFile {
    char archstring[IMAGE_ARCHIVE_START_SIZE];
    FirstLinkerMember first_linker;
    SecondLinkerMember second_linker;
    LongnamesMember long_names;
    std::vector<ArchiveMember> members;

    bool Read(std::fstream * const libfile);
    bool Write(std::fstream * const libfile);
    bool NestSymbols(const std::string &namespace_nest, const std::vector<std::string> &exclusions);
};

