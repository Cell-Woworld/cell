#ifndef __TRANSIENTSTATE_H__
#define __TRANSIENTSTATE_H__
#include "State.h"

class CTransientState : public CState
{
public:
    CTransientState(const char* szName ="", IState* pCallback = nullptr);

public:
	virtual CTransition* AddTrans(CState* s, IEvent* e, std::vector<CTransition::ActionPair> const &a = std::vector<CTransition::ActionPair>(), CTransition::ConditionPair* c = nullptr, bool isNoTarget = false);

private:
    virtual void Enter(bool bHistoryPath = false, bool isNoTarget = false);
};

#endif // __TRANSIENTSTATE_H__
