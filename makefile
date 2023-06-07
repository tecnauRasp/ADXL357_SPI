

compile:
	gcc -o process_test process_test.c adxl357.c

run:
	./process_test

c_and_r:
	make compile
	make run

compile_original:
	gcc -o read_fifo read_fifo.c adxl357.c

run_original:
	./read_fifo

c_and_r_original:
	make compile_original
	make run_original

clean:
	-rm *.o *~