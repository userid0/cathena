my personal kingdom

based on svn1104

incomplete list of modifications compared to the stable branch
///////////////////////////////////////////////////////////
c++ compilable with VC and g++
the makefiles are for solaris, 
so might need to edit linker options on linux
///////////////////////////////////////////////////////////
little/big endian support and memory allignment fixes
start to make types checking more tight
///////////////////////////////////////////////////////////
vararray support for non-gnu compilers
actually not really a support for that but a macro 
to create and free variable size buffers using 
the fastest supported method for the given compiler
///////////////////////////////////////////////////////////
a "real" ShowMessage 
includes a ansi sequence parser for windows and switchable 
calling conventions depending on the compiler
///////////////////////////////////////////////////////////
fast socket access
replacing the loop with IS_FDSET 
is much faster, still testing
adding different socket handling on WIN32
it is not exactly the same version I did for jAthena,
I'm still looking for a better implementation
///////////////////////////////////////////////////////////
npc duplication fix for multimap
duplication is not working on ,ultiple map servers 
when the npc to duplicate from is not on the same server, 
a fast fix; should generally reconsider structure of 
npc and scripts
///////////////////////////////////////////////////////////
input window
amount fixing on the buildin_input is useless 
to prevent trade hacks
///////////////////////////////////////////////////////////
sell/buy amount fix
check and fix the amount of sell/buy objects 
where it is possible, at clif_parse.
///////////////////////////////////////////////////////////
reading npcs recursively from folder
///////////////////////////////////////////////////////////
fix for moving npcs with ontouch event
a fast hack, should generally reconsider structure 
and access to map_data
///////////////////////////////////////////////////////////
changed all usages of ip numbers to host byte order 
for better consitency, 
still cleaning and simplifying code
///////////////////////////////////////////////////////////