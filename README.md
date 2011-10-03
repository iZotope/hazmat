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