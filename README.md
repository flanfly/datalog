# Intro
This is a basic deductive database that can be queried with Datalog.
The rules are written in a DSEL that mimics the Datalog syntax found in other implementations like DES (http://des.sourceforge.net).
Deduction is done bottom-up by delta/semi-naive iteration.

# Build

A simple 'make' should do the trick. Dependencies are a working, C++11 compliant, compiler (modify the CXX variable if you don't want clang), cppunit and GNU Make.

# Why?

This prototype will be integrated into a [larger program analysis framework](https://github.com/das-labor/panopticon).
