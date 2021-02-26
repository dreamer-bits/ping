a.out: main.c ping.c
	gcc main.c ping.c -lm

clean:
	rm -rf a.out
