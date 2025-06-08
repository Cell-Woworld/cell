#pragma once

#include <string.h>
#include <vector>
#include <map>

#ifdef USE_STATEMACHINE_LIB
#  define STATEMACHINE_API
#else
#if defined(USE_STATEMACHINE_EXPORT)
#  define STATEMACHINE_API __declspec(dllexport)
#else
#  define STATEMACHINE_API __declspec(dllimport)
#endif
#endif

class STATEMACHINE_API IAction
{
public:
	typedef std::vector<std::pair<std::string, std::string>> ParamPairList;

	IAction(const char* szName, int type) {
		name_ = szName;
		type_ = type;
	};
    virtual ~IAction() {};
	char* name() { return (char*)name_.data(); };
	int type() { return type_; };

public:
    virtual void execute(const ParamPairList& params) = 0;

private:
	std::string name_;
	int type_;

#pragma region friend
    friend class CState;
#pragma endregion friend
};

