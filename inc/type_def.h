#pragma once

#if !defined(API_PUBLIC) || !defined(STATIC_API_PROVIDER)

#if defined _WIN32 || defined __CYGWIN__
	#ifdef STATIC_API
		#ifdef __GNUC__
			#undef __FUNCTION__
			#define __FUNCTION__ __PRETTY_FUNCTION__
			#define PUBLIC_API __attribute__ ((dllexport))
		#else
			#define PUBLIC_API
		#endif
	#elif defined API_PROVIDER
		#ifdef __GNUC__
			#undef __FUNCTION__
			#define __FUNCTION__ __PRETTY_FUNCTION__
			#define PUBLIC_API __attribute__ ((dllexport))
		#else
			#define PUBLIC_API __declspec(dllexport) // Note: actually gcc seems to also supports this syntax.
		#endif
	#else
		#ifdef __GNUC__
			#undef __FUNCTION__
			#define __FUNCTION__ __PRETTY_FUNCTION__
			#define PUBLIC_API __attribute__ ((dllimport))
		#else
			#define PUBLIC_API __declspec(dllimport) // Note: actually gcc seems to also supports this syntax.
		#endif
	#endif
	#define PRIVATE_API
#ifndef _CRT_SECURE_NO_WARNINGS
	#define _CRT_SECURE_NO_WARNINGS
#endif
#else
	#if __GNUC__ >= 4
		#define PUBLIC_API __attribute__ ((visibility ("default")))
		#define PRIVATE_API  __attribute__ ((visibility ("hidden")))
		#undef __FUNCTION__
		#define __FUNCTION__ __PRETTY_FUNCTION__
	#else
		#define PUBLIC_API
		#define PRIVATE_API
	#endif
#endif

#endif

#ifdef __ANDROID__
#include <android/log.h>
#ifdef _DEBUG
#define LOG_D(TAG, ...) __android_log_print(ANDROID_LOG_DEBUG,TAG ,__VA_ARGS__)
#else
#define LOG_D(TAG,...)
#endif
#define LOG_I(TAG, ...) __android_log_print(ANDROID_LOG_INFO,TAG ,__VA_ARGS__)
#define LOG_W(TAG, ...) __android_log_print(ANDROID_LOG_WARN,TAG ,__VA_ARGS__)
#define LOG_E(TAG, ...) __android_log_print(ANDROID_LOG_ERROR,TAG ,__VA_ARGS__)
#define LOG_F(TAG, ...) __android_log_print(ANDROID_LOG_FATAL,TAG ,__VA_ARGS__)
#else
#ifdef _DEBUG
#ifdef _TRACE
#define LOG_T(TAG,...) {time_t t;time(&t);printf("%s %#x>>> TRACE:<%s> ", ctime(&t), (unsigned int)std::hash<std::thread::id>{}(std::this_thread::get_id()), TAG);printf(__VA_ARGS__); printf("\n");}
#else
#define LOG_T(TAG,...) {}
#endif
#define LOG_D(TAG,...) {time_t t;time(&t);printf("%s %#x>>> DEBUG:<%s> ", ctime(&t), (unsigned int)std::hash<std::thread::id>{}(std::this_thread::get_id()), TAG);printf(__VA_ARGS__); printf("\n");}
#else
#define LOG_T(TAG,...) {}
#define LOG_D(TAG,...) {}
#endif
#define LOG_I(TAG,...) {time_t t;time(&t);printf("%s %#x>>> INFO:<%s> ", strtok(ctime(&t), "\n"), (unsigned int)std::hash<std::thread::id>{}(std::this_thread::get_id()), TAG);printf(__VA_ARGS__); printf("\n");}
#define LOG_W(TAG,...) {time_t t;time(&t);printf("%s %#x>>> WARN:<%s> ", strtok(ctime(&t), "\n"), (unsigned int)std::hash<std::thread::id>{}(std::this_thread::get_id()), TAG);printf(__VA_ARGS__); printf("\n");}
#define LOG_E(TAG,...) {time_t t;time(&t);printf("%s %#x>>> ERROR:<%s> ", strtok(ctime(&t), "\n"), (unsigned int)std::hash<std::thread::id>{}(std::this_thread::get_id()), TAG);printf(__VA_ARGS__); printf("\n");}
#define LOG_F(TAG,...) {time_t t;time(&t);printf("%s %#x>>> FATAL:<%s> ", strtok(ctime(&t), "\n"), (unsigned int)std::hash<std::thread::id>{}(std::this_thread::get_id()), TAG);printf(__VA_ARGS__); printf("\n");}
#endif

#ifdef _WIN32
#define PREFIX ""
#elif defined __linux__
#define PREFIX "lib"
#elif defined __APPLE__
#define PREFIX "lib"
#endif

#include <cassert>
#include <functional>
#include <algorithm>
#include <iostream>
#include <list>
#include <map>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <queue>
#include <deque>
#include <set>
#include <sstream>
#include <stack>
#include <string>
#include <stdexcept>
#include <thread>
#include <vector>
#include <utility>
#include <condition_variable>
#include <time.h>
#include "dynarray.h"
#include "nlohmann/json.hpp"

//#define null nullptr

#define assertEquals(x, y)	assert((x) == (y))
#define assertTrue(x)	assert(x)
#define assertFalse(x)	assert(!x)

#define BIO_NAMESPACE BioSys
#define BIO_BEGIN_NAMESPACE namespace BIO_NAMESPACE {
#define BIO_END_NAMESPACE }
#define USING_BIO_NAMESPACE using namespace BIO_NAMESPACE;


//typedef bool boolean;
typedef int Integer;
typedef unsigned char Byte;

typedef std::string						String;
typedef std::wstring					WString;
typedef std::invalid_argument			IllegalArgumentException;
//typedef std::shared_ptr<bool>			Boolean;
typedef std::shared_ptr<void>			Object;
typedef std::mutex						Mutex;
typedef std::lock_guard<Mutex>			MutexLocker;
typedef std::unique_lock<Mutex>			CondLocker;
typedef std::stringstream				StringBuilder;
typedef std::thread						Thread;
typedef std::thread::id					ThreadID;
typedef std::runtime_error				StdException;
typedef std::condition_variable			Cond_Var;
typedef std::vector<unsigned char> 		ByteArray;


#define ToBind							std::bind
#define GetCurrentThreadID				std::this_thread::get_id
#define Func(x)							std::bind(&x, this, std::placeholders::_1, std::placeholders::_2)
//#define Task(x)							std::bind(&x, this)

//typedef std::static_pointer_cast<void> ToObj;

#define ToObject(x)		std::static_pointer_cast<void>(x)

template <typename T>
		using Array = std::vector<T>;

//template <typename T>
//		using Bind = std::bind<T>;

template <typename T>
		using Function = std::function<T>;

template <typename T>	
		using List = std::list<T>;

template <typename _Kty, typename _Ty, class _Pr = std::less<_Kty>>
		using Map = std::map<_Kty, _Ty, _Pr, std::allocator<std::pair<const _Kty, _Ty>>>;

template <typename _Kty, typename _Ty, class _Hasher = std::hash<_Kty>, class _Keyeq = std::equal_to<_Kty>>
		using UoMap = std::unordered_map<_Kty, _Ty, _Hasher, _Keyeq, std::allocator<std::pair<const _Kty, _Ty>>>;

template <typename T> 
		using Queue = std::queue<T>;

template< typename T1,
			typename T2 >
			using Pair = std::pair<T1, T2>;
		
template <typename T>
		using DEQueue = std::deque<T>;

template <typename T> 
		using Set = std::set<T>;


template <typename T>
		using SPtr = std::shared_ptr<T>;

template <typename T>
		using Obj = std::shared_ptr<T>;

//template <typename T> 
//		using ObjTo = std::static_pointer_cast<T>;


template <typename T>
		using Stack = std::stack<T>;
		
#define CREATE_INSTANCE "CreateInstance"
const char TAG_PACKET[] = "@payload";
const char TAG_COLUMNS_SOURCE_MODEL_LIST[] = "@columns_src";
