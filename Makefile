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
	g++ -o integration.exe -std=c++11 -Idependencies/libwebm/webm_parser/include -Idependencies/libyuv/include -Idependencies/dav1d/include -L. -ldav1d dependencies/libwebm/webm_parser/object_files/*.o dependencies/libyuv/object_files/*.o src/integration.cpp

integration_static:
	g++ -o integration.exe -std=c++11 -Idependencies/libwebm/webm_parser/include -Idependencies/libwebm/webm_parser -Idependencies/libyuv/include -Idependencies/dav1d/include -L. -ldav1d src/integration.cpp dependencies/libwebm/webm_parser/src/*.cc dependencies/libyuv/source/*.cc

dav1d_test:
	gcc src/test.c -Idependencies/dav1d/include -L. -ldav1d 

clean:
	rm -f *.out *.o *.d *.exe dependencies/libwebm/webm_parser/object_files/*.o
 