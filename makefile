

compile:
	gcc -o bin/read_acceleration read_acceleration.c adxl357.c
	gcc -o bin/read_temperature read_temperature.c adxl357.c

run:
	./bin/read_acceleration

run_t:
	make run &
	./bin/read_temperature &
	

plot:
	python3 ./PlotData.py

clean:
	-rm *.o ./bin/*

delete_csv:
	-rm ./RecordsAcc/*
	-rm ./RecordsTemp/*
