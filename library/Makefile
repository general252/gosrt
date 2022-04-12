
all:
	g++ -fPIC -shared gosrt.cpp  \
		-L./srt/lib -lsrt \
		-I./srt/include \
		-lpthread -std=c++11 \
		-o ../bin/gosrt.dll
