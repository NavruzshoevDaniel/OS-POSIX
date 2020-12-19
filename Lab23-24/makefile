build_sem:
	mkdir -p out
	gcc ./*.c ./sem_implementation/*.c -pthread -o ./out/a.out -D SEM_IMPEMENTATION

build_cond:
	mkdir -p out
	gcc ./*.c .make/cond_implementation/*.c -pthread -o ./out/a.out

run:
	./out/a.out