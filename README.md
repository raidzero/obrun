obrun
=====

Run dialog desinged for *box WM's. Will happily run anywhere glibc and gtk 2 live though.

Has tab PATH autocompletion and records a history of successfully run commands :)

This very likely has memory leaks. It works for me though.

In order to build, just run make, make install (optional)

To use: enter a command, all possible matches can be cycled through using TAB
pass -a as an argument to switch autocomplete sort mode to alphabetical, like bash
