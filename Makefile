all: dbms_query

dbms_query: dbms_query.o

clean:
	rm dbms_query dbms_query.o output.txt
