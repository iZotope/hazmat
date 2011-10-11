Hazmat
======

About
-----
Hazmat is a tool for nesting symbols names in Microsoft library (.lib) files into a deeper namespace. It's primary 
purpose is to resolve name conflicts between static libraries that share a common base that may change over time.
It can nest every internally defined symbol in a library into a namespace, and has a method for excluding functions
which will be accessed externally.


Compiling
---------
A Visual Studio 2010 sln file "hazmat.sln" is included for building the executable. It has been tested in Debug and Release
builds for Win32.


Usage
-----
To nest all symbols in original.lib into the namespace ABC
`hazmat /IN:original.lib /OUT:output.lib /NEST:ABC /X:none`

To nest all symbols into ABC, but exclude two symbols
`hazmat /IN:original.lib /OUT:output.lib /NEST:ABC /X:?mangledcplus@name@@abcdefg,?anothermangled@name@@abcdefg`

Notes
-----
Hazmat may not work properly on libraries built with whole program optimization enabled. There are no garuntees that
even the microsoft library tools will work on libs built with /GL. For more information on this limitation and how
/GL effects static libraries visit [MSDN /GL (Whole Program Optimization)][1]


License
-------
Hazmat is distributed under the BSD License. See the file LICENSE for full license information.
Copyright (c) 2011 iZotope Inc.


[1]: http://msdn.microsoft.com/en-us/library/0zza0de8(v=VS.100).aspx