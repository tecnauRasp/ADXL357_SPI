

compile:
	gcc -o bin/process_test process_test.c adxl357.c

run:
	./bin/process_test

c_and_r:
	make compile
	make run

compile_original:
	gcc -o bin/read_fifo read_fifo.c adxl357.c

run_original:
	./bin/read_fifo

c_and_r_original:
	make compile_original
	make run_original

clean:
	-rm *.o *~