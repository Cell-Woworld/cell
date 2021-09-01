#ifndef _WIN32

#include <google/protobuf/stubs/common.h>

#ifdef __ANDROID__
#include <android/log.h>
#define TAG "FileIO"
#define LOG_D(...) __android_log_print(ANDROID_LOG_DEBUG,TAG ,__VA_ARGS__)
#define LOG_I(...) __android_log_print(ANDROID_LOG_INFO,TAG ,__VA_ARGS__)
#define LOG_W(...) __android_log_print(ANDROID_LOG_WARN,TAG ,__VA_ARGS__)
#define LOG_E(...) __android_log_print(ANDROID_LOG_ERROR,TAG ,__VA_ARGS__)
#define LOG_F(...) __android_log_print(ANDROID_LOG_FATAL,TAG ,__VA_ARGS__)
#else
#define LOG_D(...) printf("DEBUG: ");printf(__VA_ARGS__); printf("\n")
#define LOG_I(...) printf("INFO: ");printf(__VA_ARGS__); printf("\n")
#define LOG_W(...) printf("WARNING: ");printf(__VA_ARGS__); printf("\n")
#define LOG_E(...) printf("ERROR: ");printf(__VA_ARGS__); printf("\n")
#define LOG_F(...) printf("FATAL: ");printf(__VA_ARGS__); printf("\n")
#endif

void so_init() __attribute__((constructor));
void so_fini() __attribute__((destructor));

void so_init()
{
	LOG_I("so_init()");
}

void so_fini()
{
	LOG_I("so_fini()");
	google::protobuf::ShutdownProtobufLibrary();
}

#endif