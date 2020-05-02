This implements a simulation of the power line PLSE from the specifications.
It uses pipes for communicating with the MACS and PL (see main.c). The PLSE
can be "stepped" by entering a value at the console. The implementation runs
on linux presently. The implementation is not optimized. It doesn't transmit
or receive CEBus chirp. It only implement the superior/inferior sup1/sup2 states.

2014/11/28
  So far, the PLSE transmit a packet successfully. The receive preamble doesn't
  seem to work yet.

Note: when transmission of EOP is complete (SYMTIMER = 100), enter 0 as increment
for jumping to the XMIT_CRC state.