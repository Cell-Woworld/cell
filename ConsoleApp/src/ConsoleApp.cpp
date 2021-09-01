// ConsoleApp.cpp : 此檔案包含 'main' 函式。程式會於該處開始執行及結束執行。
//

#include "IBiomolecule.h"
#include "ICell.h"
#include <iostream>
#include <filesystem>

#define TAG "ConsoleApp"

#ifdef __ANDROID__
#include "../ndk-unzip/src/miniunz.h"
#include <android/asset_manager_jni.h>
#include <android/log.h>
#include <jni.h>
#include <unistd.h>
#endif

#ifdef _WIN32
#include <windows.h>
#include <direct.h>
#define DLL_EXTNAME ".dll"
#define chdir _chdir
#define strncpy strncpy_s
#define strcpy strcpy_s

#elif defined __linux__
#include <dlfcn.h>
#include <windows.h>
#define LoadLibrary(a) dlopen(a, RTLD_NOW)
#define FreeLibrary(a) dlclose(a)
#define GetProcAddress(a, b) dlsym(a, b)
#define DLL_EXTNAME ".so"
#define PREFIX "lib"

#elif defined __APPLE__
#include <dlfcn.h>
#define LoadLibrary(a) dlopen(a, RTLD_NOW)
#define FreeLibrary(a) dlclose(a)
#define GetProcAddress(a, b) dlsym(a, b)
#define HINSTANCE void *
#define DLL_EXTNAME ".dylib"
#endif

USING_BIO_NAMESPACE
namespace fs = std::filesystem;

bool Evolution(const String& source_path, const String& target_path)
{
	bool _ret = true;
	std::error_code _err_code;
	if (fs::exists(target_path) == false)
		return false;
	// replace target_folder with repo
	auto _file_list = [](const String& path) -> Array<String> {
		Array<String> _list;
		for (auto& dirEntry : std::filesystem::directory_iterator(path)) {
			if (!dirEntry.is_regular_file()) {
				//std::cout << "Directory: " << dirEntry.path() << std::endl;
				if (dirEntry.path().string().find(".git") == String::npos)
					_list.push_back(dirEntry.path().string());
				//continue;
			}
			//std::filesystem::path file = dirEntry.path();
			//std::cout << "Filename: " << file.filename() << " extension: " << file.extension() << std::endl;
			//_list.push_back(file.string());
		}
		return _list;
	};
	if (fs::exists("backup") == true)
	{
		LOG_I(TAG, "removing folder backup");
		fs::remove_all("backup", _err_code);
		if (_err_code.value() != 0)
			LOG_E(TAG, "error (code,message) of remove_all: (%d,%s)", _err_code.value(), _err_code.message().c_str());
	};
	LOG_I(TAG, "creating folder backup");
	fs::create_directories("backup");
	for (auto dir_name : _file_list(target_path))
	{
		LOG_I(TAG, "target: %s\n", dir_name.c_str());
		if (dir_name != (target_path + "update") && dir_name != (target_path + "backup") && dir_name != (target_path + "_Tools") && dir_name != (target_path + "_tools"))
		{
			String _relative_path = dir_name.substr(target_path.size());
			LOG_I(TAG, "moving folder %s to %s", dir_name.c_str(), ((String)"backup/" + _relative_path).c_str());
			fs::rename(dir_name, (String)"backup/" + _relative_path, _err_code);
			if (_err_code.value() != 0)
				LOG_E(TAG, "error (code,message) of renaming %s to %s: (%d,%s)", dir_name.c_str(), ((String)"backup/" + _relative_path).c_str(), _err_code.value(), _err_code.message().c_str())
		}
	}
	for (auto dir_name : _file_list(source_path))
	{
		String _target_full_path = target_path + dir_name.substr(source_path.size());
		//if (fs::exists(_target_full_path) == true)
		//{
		//	fs::remove_all(_target_full_path + ".backup", _err_code);
		//	if (_err_code.value() != 0)
		//		LOG_E(TAG, "error message of remove_all: %s", _err_code.message().c_str());
		//	fs::rename(_target_full_path, _target_full_path + ".backup", _err_code);
		//	if (_err_code.value() != 0)
		//		LOG_E(TAG, "error message of rename: %s", _err_code.message().c_str())
		//}
		LOG_I(TAG, "creating folder %s", _target_full_path.c_str());
		fs::create_directories(_target_full_path, _err_code);
		LOG_I(TAG, "copying folder %s to %s", dir_name.c_str(), _target_full_path.c_str());
		fs::copy(dir_name, _target_full_path, fs::copy_options::overwrite_existing | fs::copy_options::recursive, _err_code);
		if (_err_code.value() != 0)
		{
			LOG_E(TAG, "error (code,message) of copying %s to %s: (%d,%s)", dir_name.c_str(), _target_full_path.c_str(), _err_code.value(), _err_code.message().c_str());
		}
	}
	if (fs::exists("backup/www/.data/") == true)
	{
		LOG_I(TAG, "copying folder %s to %s", "backup/www/.data/", (target_path + "www/.data/").c_str());
		fs::copy("backup/www/.data/", target_path + "www/.data/", fs::copy_options::overwrite_existing | fs::copy_options::recursive, _err_code);
		if (_err_code.value() != 0)
		{
			LOG_E(TAG, "error (code,message) of copying %s to %s: (%d,%s)", "backup/www/.data/", (target_path + "www/.data/").c_str(), _err_code.value(), _err_code.message().c_str());
			//_ret = false;
			_ret = true;
		}
	}
	return _ret;
};

#ifdef STATIC_API
extern "C" RNA * UnitTest_CreateInstance(IBiomolecule * owner);
extern "C" RNA * Timer_CreateInstance(IBiomolecule * owner);
extern "C" RNA * iOSPathway_CreateInstance(IBiomolecule * owner);
extern "C" RNA * Websocket_CreateInstance(IBiomolecule * owner);
extern "C" RNA * UIKits_CreateInstance(IBiomolecule * owner);
extern "C" RNA * FileIO_CreateInstance(IBiomolecule * owner);
#define BUILD_CREATOR_MAP(NAME, FUNC) RNA_Map_[NAME] = FUNC
#endif

#if defined(__ANDROID__) && !defined(__ANDROID_APP__)
void get_lib_dir(char path[]) {
	static const char* SELF_NAME = "/libAndroidUnitTest.so";
	static const size_t SELF_NAME_LEN = strlen(SELF_NAME);
	FILE* fmap = fopen("/proc/self/maps", "r");
	if (!fmap) {
		LOG_E(TAG, "failed to open maps");
		return;
	}
	std::unique_ptr<FILE, int (*)(FILE*)> fmap_close{ fmap, ::fclose };
	char linebuf[512];
	while (fgets(linebuf, sizeof(linebuf), fmap)) {
		uintptr_t begin, end;
		char perm[10], offset[20], dev[10], inode[20], path_mem[256], * _path;
		int nr = sscanf(linebuf, "%zx-%zx %s %s %s %s %s", &begin, &end, perm,
			offset, dev, inode, path_mem);
		if (nr == 6) {
			_path = nullptr;
		}
		else {
			if (nr != 7) {
				LOG_E(TAG, "failed to parse map line: %s", linebuf);
				return;
			}
			_path = path_mem;
		}
		if (_path) {
			auto len = strlen(_path);
			auto last_dir_end = _path + len - SELF_NAME_LEN;
			if (!strcmp(last_dir_end, SELF_NAME)) {
				last_dir_end[1] = 0;
				strncpy(path, _path, std::min<int>(259, len));
				return;
			}
		}
	}
	LOG_E(TAG, "can not find path of %s", SELF_NAME + 1);
	return;
};

void get_user_dir(char path[]) {
	FILE* fmap = fopen("/proc/self/cmdline", "r");
	if (!fmap) {
		LOG_E(TAG, "failed to open maps");
		return;
	}
	std::unique_ptr<FILE, int (*)(FILE*)> fmap_close{ fmap, ::fclose };
	char linebuf[260];
	while (fgets(linebuf, sizeof(linebuf), fmap)) {
		char* _path;
		_path = linebuf;
		auto len = strlen(_path);
		auto last_dir_end = _path + len;
		last_dir_end[1] = 0;
		strcpy(path, "/data/data/");
		strncat(path, _path, std::min<int>(259, sizeof("/data/data/") + len));
		return;
	}
	LOG_E(TAG, "no such path");
	return;
};
extern "C" int Java_com_woworld_cell_Cell_NativeMain(JNIEnv * env, jobject thiz,
	jobject assetManager,
	jstring input)

#else

int main(int argc, char* argv[])

#endif

{
	while (true)
	{
		Obj<IBiomolecule> _cell = nullptr;

#if defined(__ANDROID__) && !defined(__ANDROID_APP__)
		const char* _input = env->GetStringUTFChars(input, 0);
#else
		char _input[MAX_PATH] = { 0 };
		if (argc > 1)
			strncpy(_input, argv[1], MAX_PATH - 1);
		else
			strncpy(_input, "scxml/Root.scxml", MAX_PATH - 1);
#endif

#ifdef STATIC_API
		BUILD_CREATOR_MAP("UnitTest", UnitTest_CreateInstance);
		BUILD_CREATOR_MAP("Timer", Timer_CreateInstance);
		BUILD_CREATOR_MAP("iOSPathway", iOSPathway_CreateInstance);
		BUILD_CREATOR_MAP("Websocket", Websocket_CreateInstance);
		BUILD_CREATOR_MAP("Websocket/Websocket", Websocket_CreateInstance);
		BUILD_CREATOR_MAP("UIKits/UIKits", UIKits_CreateInstance);
		BUILD_CREATOR_MAP("FileIO/FileIO", FileIO_CreateInstance);

		_cell = Obj<IBiomolecule>((IBiomolecule*)Cell_CreateInstance(_input));
		_cell->do_event("Bio.Cell.WaitApoptosis");
		if (_cell != nullptr) {
			_cell.reset();
		}
#else
		char _user_path[260];
		memset(_user_path, 0, sizeof(_user_path));
		strcpy(_user_path, "../");
		char _lib_path[260];
		memset(_lib_path, 0, sizeof(_lib_path));
		strcpy(_lib_path, "bin");
#if defined(__ANDROID__) && !defined(__ANDROID_APP__)
		const int MAXFILENAME = 260;
		// use asset manager to open asset by filename
		AAssetManager* _mgr = AAssetManager_fromJava(env, assetManager);
		assert(_mgr != nullptr);
		AAssetDir* _current_dir = AAssetManager_openDir(_mgr, "");
		const char* _fileName = AAssetDir_getNextFileName(_current_dir);
		while (_fileName != nullptr) {
			if (strcmp(_fileName, "scxml.zip") != 0) {
				_fileName = AAssetDir_getNextFileName(_current_dir);
				continue;
			}

			AAsset* _aasset = AAssetManager_open(_mgr, _fileName, AASSET_MODE_UNKNOWN);
			if (_aasset) {
				// const char writeablePath[] = "/data/data/com.example.hellojni";
				get_user_dir(_user_path);

				size_t _asset_length = AAsset_getLength(_aasset);

				String _content;
				_content.assign(_asset_length, '\0');
				AAsset_read(_aasset, (char*)_content.data(), _content.size());
				LOG_I(TAG, "Asset file size: %zu\n", _content.size());

				char tofname[MAXFILENAME] = "";
				FILE* file = NULL;
				snprintf(tofname, MAXFILENAME, "%s/%s", _user_path, _fileName);
				file = fopen(tofname, "wb");
				if (file) {
					fwrite(_content.data(), sizeof(char), _asset_length, file);
					fclose(file);
				}
				else {
					LOG_I(TAG, "open file error : %s", tofname);
				}

				AAsset_close(_aasset);
				unzip(tofname, _user_path);
				break;
			}

			_fileName = AAssetDir_getNextFileName(_current_dir);
		}
		char _lib_path[MAXFILENAME];
		memset(_lib_path, 0, sizeof(_lib_path));
		get_lib_dir(_lib_path);
#endif
		chdir(_lib_path);
		HINSTANCE _instance = NULL;
		CREATE_CELL_INSTANCE_FUNCTION CreateInstanceF = NULL;
		if ((_instance = LoadLibrary(
			(String("./") + PREFIX + "Cell" + DLL_EXTNAME).c_str())) == NULL)
			return -1;
		CreateInstanceF =
			(CREATE_CELL_INSTANCE_FUNCTION)GetProcAddress(_instance, CREATE_INSTANCE);
		if (CreateInstanceF)
			_cell = Obj<IBiomolecule>((IBiomolecule*)CreateInstanceF(
			(String(_user_path) + "/" + _input).c_str()));

		_cell->do_event("Bio.Cell.WaitApoptosis");

		if (_cell != nullptr) {
			_cell.reset();
		}
#if defined(__ANDROID__) && !defined(__ANDROID_APP__)
		env->ReleaseStringUTFChars(input, _input);
#endif

		FreeLibrary(_instance);
#endif
		//std::this_thread::sleep_for(std::chrono::milliseconds(20000));
		chdir("..");
		if (Evolution("update/", "./") == false)
		{
			break;
		}
	}
	return 0;
}
