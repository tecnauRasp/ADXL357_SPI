FLAGS = -pthread

compile:
	gcc -o read_fifo read_fifo.c adxl357.c $(FLAGS)

compile_test:
	gcc -o thread_test thread_test.c adxl357.c $(FLAGS)

run:
	./read_fifo

c_and_r:
	make compile
	make run

clean:
	-rm *.o *~