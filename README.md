Hazmat
======

About
-----
Hazmat is a namespace injection tool for Microsoft library (.lib) files. It also has an obfuscation mode that can change
symbol names without destroying the functionality of the library.

It's primary purpose is to resolve name conflicts between static libraries that share a common base that may change over time.
It also can be used to hide sensitive IP that may be revealed by symbol names.

For every internally defined symbol it can obfuscate the symbol or "nest" it into a namespace. Hazmat also has a method for excluding functions which will be accessed externally.
This can be used to obfuscate a libraries symbol names but still expose a public interface.


Compiling
---------
A Visual Studio 2010 sln file "hazmat.sln" is included for building the executable. It has been tested in Debug and Release
builds for Win32.


Usage
-----
To nest all symbols in original.lib into the namespace ABC
`hazmat /IN:original.lib /OUT:output.lib /NEST:ABC /X:none`

To nest all symbols into ABC, but exclude two symbols by manged name
`hazmat /IN:original.lib /OUT:output.lib /NEST:ABC /XM:?mangledcplus@name@@abcdefg,?anothermangled@name@@abcdefg`

To nest all symbols into ABC, but exclude two based on substring match
`hazmat /IN:original.lib /OUT:output.lib /NEST:ABC /X:MyEngineClass,MyPublicAllocatorNamespace`

To obfuscate all the symbols in original lib
`hazmat /IN:original.lib /OUT:output.lib /OBFS`

To obfuscate all the symbols, and nest the obfuscated symbols into a namespace but still expose public namespaces or classes
`hazmat /IN:original.lib /OUT:output.lib /OBFS /NEST:ABC /X:PublicInterfaceClass,PublicFactoryNamespace`

Notes
-----
Hazmat may not work properly on libraries built with whole program optimization enabled. There are no guarantees that
even the Microsoft library tools will work on libs built with /GL. For more information on this limitation and how
/GL effects static libraries visit [MSDN /GL (Whole Program Optimization)][1]


License
-------
Hazmat is distributed under the BSD License. See the file LICENSE.md for full license information.
Copyright (c) 2012 iZotope Inc.


[1]: http://msdn.microsoft.com/en-us/library/0zza0de8(v=VS.100).aspx