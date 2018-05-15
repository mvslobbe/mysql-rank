all: clean build

build:
	gcc -O3 -fPIC -Wall `mysql_config --include` -I /usr/local/include -c udf_rank.cc -o udf_rank.o
	ld -shared -o udf_rank.so udf_rank.o

clean:
	rm -f *.o *.so

install:
	cp udf_rank.so /usr/lib/mysql/plugin/udf_rank.so
