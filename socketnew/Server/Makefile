Target = Server
Object = server.c condition.c threadpool.c

$(Target):$(Object)
	gcc $(Object) -o $(Target) -lpthread -lmysqlclient -lrt
.PHONY:clean
clean:
	-rm -rf $(Target)
