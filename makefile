sicasm: instructions.o symbols.o scoff.o main.o
	gcc -o sicasm -Wall -O0 instructions.o symbols.o scoff.o main.o

main.o: main.c
	gcc -c -Wall -O0 main.c

scoff.o: scoff.c
	gcc -c -Wall -O0 scoff.c

symbols.o: symbols.c
	gcc -c -Wall -O0 symbols.c

instructions.o: instructions.c
	gcc -c -Wall -O0 instructions.c

clean:
	rm -f *.o project2 *.obj