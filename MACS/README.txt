This project is currently in a 1st implementation stage. It may not compile yet, and needs some
checkup to be done before working. It's based on the same principle as the PLSE implementation;
it uses named pipes for communication between layers.

main.c      OS interface and named pipes management
macs.c      state machine and dependend routines
intf.c      passing the packets between MACS, PLSE and LLC through pipes
sbrout.c    routines left to user for implementation
gl_vars.c   global variables
aslist.c    association lists
macs.h      constants and structures
plse.h      constants and structures
