dependencies:
	rm -f libwebm/webm_parser/object_files/*.o
	g++ -c -std=c++11 -Ilibwebm/webm_parser/include -Ilibwebm/webm_parser libwebm/webm_parser/src/*.cc
	mv *.o libwebm/webm_parser/object_files/

full:
	g++ -o bytes.exe -std=c++11 -Ilibwebm/webm_parser/include -Ilibwebm/webm_parser bytes_extractor.cpp libwebm/webm_parser/src/*.cc

bytes:
	g++ -o bytes.exe -std=c++11 -Ilibwebm/webm_parser/include -Ilibwebm/webm_parser libwebm/webm_parser/object_files/*.o bytes_extractor.cpp

demo:
	g++ -o demo.exe -std=c++11 -Ilibwebm/webm_parser/include -Ilibwebm/webm_parser libwebm/webm_parser/object_files/*.o demo.cpp

clean:
	rm -f *.out *.o *.d demo.exe bytes.exe libwebm/webm_parser/object_files/*.o
