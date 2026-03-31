default : servudp cliudp servbeuip clibeuip creme.o biceps

cliudp : cliudp.c
	cc -Wall -o cliudp cliudp.c

servudp : servudp.c
	cc -Wall -o servudp servudp.c

servbeuip : servbeuip.c
	cc -Wall -o servbeuip servbeuip.c

clibeuip : clibeuip.c
	cc -Wall -o clibeuip clibeuip.c

creme.o : creme.c creme.h
	cc -Wall -c creme.c

biceps : biceps.c gescom.c gescom.h creme.o
	cc -Wall -o biceps biceps.c gescom.c creme.o -lreadline

clean :
	rm -f cliudp servudp servbeuip clibeuip biceps *.o