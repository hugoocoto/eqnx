# rterminal

'Simple' terminal emulator using eqnx written in rust by Claude et al.

To exit, you first need to exit the terminal and then close the window.
Otherwise the terminal keeps alive and the terminal can not be closed. In this
case, you might need to pkill eqnx or the executable that launch the process.
Consult ps for more details.
