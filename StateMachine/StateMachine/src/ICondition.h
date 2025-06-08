#pragma once

#include <string.h>

#ifdef USE_STATEMACHINE_LIB
#  define STATEMACHINE_API
#else
#if defined(USE_STATEMACHINE_EXPORT)
#  define STATEMACHINE_API __declspec(dllexport)
#else
#  define STATEMACHINE_API __declspec(dllimport)
#endif
#endif

class STATEMACHINE_API ICondition
{
public:
	typedef std::vector<std::pair<std::string, std::string>> ParamPairList;

	ICondition(const char* szName) { name_ = szName; };
    virtual ~ICondition() {};
    char* name() { return (char*)name_.data(); };

public:
    virtual bool eval(const ParamPairList& params) = 0;

private:
    std::string name_;
};

