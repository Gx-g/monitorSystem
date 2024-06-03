Target = Client
Object = client.c 

$(Target):$(Object)
	gcc $(Object) -o $(Target) -lpthread -lmysqlclient -lrt -lcrypto
.PHONY:clean
clean:
	-rm -rf $(Target)
