#include "Transition.h"
#include "IAction.h"
#include "ICondition.h"
#include "IEvent.h"
#include "State.h"
#include <assert.h>

CTransition& CTransition::operator =(const CTransition& t)
{
    _s = t._s;
    _e = t._e;
    _a = t._a;
    _c = t._c;
	m_bIsNoTarget = t.m_bIsNoTarget;
    return *this;
}

IEvent* CTransition::getEvent()
{
    return _e;
}

const std::vector<CTransition::ActionPair>& CTransition::getAction()
{
    return _a;
}

CTransition::ConditionPair* CTransition::getCondition()
{
    return &_c;
}

CState* CTransition::getState()
{
    return _s;
}

CState* CTransition::get_Ancestor() const
{
    return Ancestor;
}

void CTransition::set_Ancestor(CState* left)
{
    Ancestor = left;
}

CTransition::CTransition(CState* s, IEvent* e, std::vector<ActionPair> const &a, ConditionPair* c, bool bisNoTarget)
{
    _s = s;
    _e = e;
    _a = a;
	if (c) _c = *c;
    Ancestor=nullptr;
	m_bIsNoTarget = bisNoTarget;
}