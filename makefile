

compile:
	gcc -o bin/read_acceleration read_acceleration.c adxl357.c

run:
	./bin/read_acceleration

plot:
	python3 ./PlotData.py

clean:
	-rm *.o ./bin/*

delete_csv:
	-rm ./CsvRecords/*

