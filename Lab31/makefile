build:
	gcc -lpthread -o main main.c services/* argschecker/argschecker.c
run:
	./main 5 18205
sanitizer:
	gcc -lpthread -o main main.c services/* argschecker/argschecker.c -fsanitize=address
clean:
	rm main
