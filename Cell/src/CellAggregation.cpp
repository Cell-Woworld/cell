#include "CellAggregation.h"
#include "../proto/Cell.pb.h"
#include "uuid.h"

#define TAG "Cell"

#ifndef VERSION_MAJOR
#define VERSION_MAJOR 0
#endif

#ifndef VERSION_MINOR
#define VERSION_MINOR 0
#endif

#ifndef VERSION_BUILD
#define VERSION_BUILD 1
#endif

#ifndef VERSION_REVISION
#define VERSION_REVISION ""
#endif

#ifdef _WIN32
#include <windows.h>
#define DLL_EXTNAME ".dll"

#elif defined __linux__
#include <dlfcn.h>
#define LoadLibrary(a) dlopen(a, RTLD_NOW)
#define FreeLibrary(a) dlclose(a)
#define GetProcAddress(a, b) dlsym(a, b)
#define DLL_EXTNAME ".so"

#elif defined __APPLE__
#include <dlfcn.h>
#include <CoreFoundation/CFBundle.h>
#define LoadLibrary(a)	dlopen(a, RTLD_NOW)
#define FreeLibrary(a)	dlclose(a)
#define GetProcAddress(a,b)	dlsym(a,b)
#define HINSTANCE void*
#define DLL_EXTNAME		".dylib"
#endif

USING_BIO_NAMESPACE

void CellAggregation::add_event(const DynaArray& msg_name, const DynaArray& payload, IBiomolecule* src)
{
	if (msg_name.str() == Bio::Cell::Aggregation::descriptor()->full_name())
	{
		aggregation_list_[payload] = src;
	}
	else if (msg_name.str() == Bio::Cell::Deaggregation::descriptor()->full_name())
	{
		if (aggregation_list_.count(payload) > 0)
			aggregation_list_.erase(payload);
	}
	else if(msg_name == "Bio.Cell.Destroyed")
		return;
	else
	{
		for (const auto& cell : aggregation_list_)
		{
			if (cell.second != src)
				cell.second->add_event(msg_name, payload, src);
		}
	}
}


void CellAggregation::do_event(const DynaArray& msg_name)
{
	assert(false);
};

void CellAggregation::on_event(const DynaArray& name_space, const DynaArray& msg_name, const DynaArray& payload)
{
}

bool CellAggregation::has(const DynaArray& uuid)
{
	return aggregation_list_.count(uuid) > 0;
}
