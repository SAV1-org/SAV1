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

integration:
	g++ -o integration.exe -std=c++11 -Idependencies/libwebm/webm_parser/include -Idependencies/libyuv/include -Idependencies/dav1d/include -L. -ldav1d -Idependencies/SDL2/include -lSDL2 dependencies/libwebm/webm_parser/object_files/*.o dependencies/libyuv/object_files/*.o src/integration.cpp

integration_static:
	g++ -o integration.exe -std=c++11 -Idependencies/libwebm/webm_parser/include -Idependencies/libwebm/webm_parser -Idependencies/libyuv/include -Idependencies/dav1d/include -L. -ldav1d -Idependencies/SDL2/include -lSDL2 src/integration.cpp dependencies/libwebm/webm_parser/src/*.cc dependencies/libyuv/source/*.cc

dav2dplay:
	g++ -o dav2dplay.exe -std=c++11 -s -Wall -Wextra -pedantic -g -Idependencies/libwebm/webm_parser/include -Idependencies/libyuv/include -Idependencies/dav1d/include -L. -ldav1d -Idependencies/SDL2/include -lSDL2 dependencies/libwebm/webm_parser/object_files/*.o dependencies/libyuv/object_files/*.o src/dav2dplay/main.c src/dav2dplay/parse.cpp src/dav2dplay/decode.c src/dav2dplay/convert.cpp 

dav3dplay_win:
	g++ -o dav3dplay.exe -std=c++11 -s -Idependencies/libwebm/webm_parser/include -Idependencies/thread -Idependencies/libyuv/include -Idependencies/dav1d/include -L. -ldav1d -Idependencies/SDL2/include -lSDL2 dependencies/libwebm/webm_parser/object_files/*.o dependencies/libyuv/object_files/*.o src/dav3dplay/*.c src/dav3dplay/*.cpp -lwinmm

dav1d_test:
	gcc src/test.c -Idependencies/dav1d/include -L. -ldav1d 

clean:
	rm -f *.out *.o *.d *.exe dependencies/libwebm/webm_parser/object_files/*.o

opus:
	g++ -o integration.exe -std=c++11 -Idependencies/libwebm/webm_parser/include -Idependencies/libyuv/include -Idependencies/dav1d/include -L. -ldav1d -Idependencies/SDL2/include -lSDL2 dependencies/libwebm/webm_parser/object_files/*.o dependencies/libyuv/object_files/*.o src/integration.cpp -Idependencies/opus/include -lopus

audio_test:
	g++ -o test.haha -std=c++11 -Idependencies/libwebm/webm_parser/include -Idependencies/libyuv/include -Idependencies/dav1d/include -L. -ldav1d -Idependencies/SDL2/include -lSDL2 dependencies/libwebm/webm_parser/object_files/*.o dependencies/libyuv/object_files/*.o src/dav2dplay/audio.c -Idependencies/opus/include -lopus
 
