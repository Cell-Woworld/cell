#ifndef __IEVENT_H__
#define __IEVENT_H__

#include <string.h>

class IEvent
{
public:
    IEvent(const char* szName ="")  { name_ = szName; };
    virtual ~IEvent() {};
    const char* GetName()	{ return (char*)name_.data(); };
protected:
    std::string name_;
};

#endif // __IEVENT_H__
