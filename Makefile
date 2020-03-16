Server:Mmysql.cpp  Server.cpp protobufs.proto RecvDataManager.cpp protobufs.pb.cc MySocket.cpp Timer.cpp Consts.h
	protoc -I=. --cpp_out=. ./protobufs.proto
	g++ -g Mmysql.cpp Server.cpp RecvDataManager.cpp protobufs.pb.cc MySocket.cpp Timer.cpp `mysql_config --cflags --libs`  -o Server -lprotobuf

.PHONY clean:
	rm -f *.o
