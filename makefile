

compile:
	gcc -o bin/process_test process_test.c adxl357.c

run:
	./bin/process_test

plot:
	python3 ./PlotData.py

clean:
	-rm *.o ./bin/*

delete_csv:
	-rm ./CsvRecords/*

