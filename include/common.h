#ifndef SAV1_COMMON
#define SAV1_COMMON

#ifndef SAV1_API
    #ifdef _WIN32
        #define SAV1_API __declspec(dllexport)
    #else
      #if __GNUC__ >= 4
        #define SAV1_API __attribute__ ((visibility ("default")))
      #else
        #define SAV1_API
      #endif
    #endif
#endif

#endif