compile:
	gcc -o read_fifo read_fifo.c adxl357.c

run:
	./read_fifo

c_and_r:
	make compile
	make run

clean:
	-rm *.o *~