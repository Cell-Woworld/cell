#pragma once

#include "ICondition.h"

class NullCondition : public ICondition
{
	virtual ICondition* createInstance(const char* szName, void* pCallback) { return nullptr; };
public:
	NullCondition(const char* szName):ICondition(szName) {};
	virtual ~NullCondition(void) {};
protected:
	virtual bool eval(const char*) { return false; };
};

class Enabled : public ICondition
{
	virtual ICondition* createInstance(const char* szName, void* pCallback) { return nullptr; };
public:
	Enabled(const char* szName) :ICondition(szName) {};
	virtual ~Enabled(void) {};
protected:
	virtual bool eval(const char*) { return true; };
};

