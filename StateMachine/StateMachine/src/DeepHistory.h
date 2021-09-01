#ifndef __DEEPHISTORY_H__
#define __DEEPHISTORY_H__
#include "State.h"

class CDeepHistory : public CState
{
public:
	CDeepHistory(const char* szName = "");

private:
    virtual void Enter(bool bHistoryPath = false, bool isNoTarget = false);

	virtual CState* get_CurrentState() const;
};

#endif // __DEEPHISTORY_H__
