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

opengl:
	g++ -Iinclude -Iexamples/opengl/include -L. examples/opengl/sav3dplay.cpp -lglfw3 -lglew32 -lopengl32 -lgdi32 -luser32 -lkernel32 -lsav1 -o sav3dplay.exe

PLATFORM := x64-windows

dav4dplay_win:
	g++ -o dav4dplay.exe -std=c++11 -s \
	-DCPPTHROUGHANDTHROUGH \
	-Idependencies/libwebm/webm_parser/include -Idependencies/thread \
	-Idependencies/libyuv/include \
	-Idependencies/$(PLATFORM)/include \
	-Ldependencies/$(PLATFORM)/bin \
	-Ldependencies/$(PLATFORM)/lib \
	-ldav1d -lSDL2 -lopus \
	dependencies/libwebm/webm_parser/object_files/*.o \
	dependencies/libyuv/object_files/*.o \
	examples/dav4dplay.c src/*.c src/*.cpp -Iinclude -Isrc -lwinmm

dav4dplay_mac:
	g++ -o dav4dplay -std=c++11 -DCPPTHROUGHANDTHROUGH -Idependencies/libwebm/webm_parser/include -Idependencies/thread -Idependencies/libyuv/include -Idependencies/dav1d/include -Idependencies/opus/include -ldav1d -lSDL2 dependencies/libwebm/webm_parser/object_files/*.o dependencies/libyuv/object_files/*.o examples/dav4dplay.c src/*.c src/*.cpp -Iinclude -Isrc -lopus

clean:
	rm -f *.out *.o *.d *.exe dependencies/libwebm/webm_parser/object_files/*.o dependencies/libyuv/object_files/*.o
