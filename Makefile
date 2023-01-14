libwebm:
	rm -f dependencies/libwebm/webm_parser/object_files/*.o
	g++ -c -std=c++11 -Idependencies/libwebm/webm_parser/include -Idependencies/libwebm/webm_parser dependencies/libwebm/webm_parser/src/*.cc
	mv *.o dependencies/libwebm/webm_parser/object_files/

libyuv:
	rm -f dependencies/libyuv/object_files/*.o
	g++ -c -std=c++11 -Idependencies/libyuv/include dependencies/libyuv/source/*.cc
	mv *.o dependencies/libyuv/object_files/

all_dependencies:
	$(MAKE) libwebm
	$(MAKE) libyuv

PLATFORM := x64-windows

dav3dplay_win:
	g++ -o dav3dplay.exe -std=c++11 -s									\
	-Idependencies/libwebm/webm_parser/include -Idependencies/thread	\
	-Idependencies/libyuv/include										\
	-Idependencies/$(PLATFORM)/include									\
	-Ldependencies/$(PLATFORM)/bin										\
	-Ldependencies/$(PLATFORM)/lib										\
	-ldav1d -lSDL2 -lopus												\
	dependencies/libwebm/webm_parser/object_files/*.o					\
	dependencies/libyuv/object_files/*.o								\
	examples/dav3dplay.c src/*.c src/*.cpp -Iinclude -Isrc -lwinmm

dav3dplay_mac:
	g++ -o dav3dplay -std=c++11 -Idependencies/libwebm/webm_parser/include -Idependencies/thread -Idependencies/libyuv/include -Idependencies/dav1d/include -Idependencies/opus/include -ldav1d -lSDL2 dependencies/libwebm/webm_parser/object_files/*.o dependencies/libyuv/object_files/*.o examples/dav3dplay.c src/*.c src/*.cpp -Iinclude -Isrc -lopus

clean:
	rm -f *.out *.o *.d *.exe dependencies/libwebm/webm_parser/object_files/*.o

opus:
	g++ -o integration.exe -std=c++11 -Idependencies/libwebm/webm_parser/include -Idependencies/libyuv/include -Idependencies/dav1d/include -L. -ldav1d -Idependencies/SDL2/include -lSDL2 dependencies/libwebm/webm_parser/object_files/*.o dependencies/libyuv/object_files/*.o src/integration.cpp -Idependencies/opus/include -lopus 
