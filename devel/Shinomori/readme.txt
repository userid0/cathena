my personal kingdom

based on svn1140

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
fixed a stupid error in windows socket list handling
maybe I just take my other socket class in here 
but would need to rewrite the interface then
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
simplified and tighted timers a bit
finalizing still causes problems, but will be solved 
when swapping the whole core to my other c++ lib
///////////////////////////////////////////////////////////
changes packet processing interaction scheme between 
core socket and parse_func's, 
changed eof to a couple of flags that are used to hold 
the status of the connection, also added access function
///////////////////////////////////////////////////////////
added forcefull disconnect detection and let the pc online 
for 10 seconds so Alt-F4 cannot be used to flee a fight
moved the clif_waitclose to the session,
might reconsider the wide usage of waitclose in clif.c, 
it is not useful on every existing position
///////////////////////////////////////////////////////////
(not online)
preparing a new script machine that pre-compiles the text 
to some intermediate code which can be interpreted 
in a pc or map instance
this would enable memory reduction ie. by using  
script sharing or building string tables
intermediate code might be also smaller and faster to use
///////////////////////////////////////////////////////////
(not online)
support of multimap servers it should be generally 
reconsidered, the current problems come from map servers
not knowing from each other and no sync unit is used
the char server could do this but it would be necessary
to add much code for syncronisation and checking 
which might be not easy and error prone.
Changing the general structure could get better resutls
ie. the char could hold maps, players, variables etc. 
(it does this even now but is not used much)
and takes care of all global map-player interaction things
the mapserver is initialized with data from the char server
ans only does stuff related to single maps and gives any 
other requests directly back to the char server to handle them
///////////////////////////////////////////////////////////