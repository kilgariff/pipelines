all:
	gcc -g -fmax-errors=1 main.c pipelines.c util.c parser.c ctx.c -l yaml -o pipelines

run: all
	./pipelines

clean:
	rm -f *.o pipelines