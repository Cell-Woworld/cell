// IState.h: interface for the IState class.
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include <assert.h>

#ifdef USE_STATEMACHINE_LIB
#  define STATEMACHINE_API
#else
#if defined(USE_STATEMACHINE_EXPORT)
#  define STATEMACHINE_API __declspec(dllexport)
#else
#  define STATEMACHINE_API __declspec(dllimport)
#endif
#endif

class STATEMACHINE_API IState
{
    virtual IState* createInstance(const char* szName, void* pCallback) = 0;

public:
    IState(const char* cszName)
    {
        assert( (cszName != nullptr) && (cszName[0] != '\0') );
        name_ = cszName;
    };

    virtual ~IState() {};

    virtual const char* name() { return name_.data(); };

protected:
    virtual void enter() {};
    virtual void exit() {};

private:
    std::string name_;

#pragma region friend
    friend class CState;
#pragma endregion friend
};

