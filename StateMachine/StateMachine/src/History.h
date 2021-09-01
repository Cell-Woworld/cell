#ifndef __HISTORY_H__
#define __HISTORY_H__
#include "State.h"

class CHistory : public CState
{
public:
	CHistory(const char* szName = "");

private:
    virtual void Enter(bool bHistoryPath = false, bool isNoTarget = false);

	virtual CState* get_CurrentState() const;
};

#endif // __HISTORY_H__
