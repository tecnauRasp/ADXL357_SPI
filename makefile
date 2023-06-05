compile:
	gcc -o read_fifo read_fifo.c adxl357.c

compile_test:
	gcc -o process_test process_test.c adxl357.c

run_test:
	./process_test

c_and_r_test:
	make compile_test
	make run_test

run:
	./read_fifo

c_and_r:
	make compile
	make run

clean:
	-rm *.o *~