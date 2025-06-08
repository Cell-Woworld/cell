#include "Transition.h"
#include "TransientState.h"
#include "ICondition.h"
#include "IEvent.h"
#include "internal/IStateMachine.h"
#include <assert.h>

CTransientState::CTransientState(const char* szName, IState* pCallback)
    : CState(szName, STATE, pCallback)
{
}

void CTransientState::Enter(bool bHistoryPath, bool isNoTarget)
{
	if (!isNoTarget)
		EnterNotify();

	TransVector::iterator pos = m_vectTrans.begin();
    for(; pos != m_vectTrans.end(); ++pos )
    {
		//assert((*pos)->getEvent() == nullptr || (*pos)->getEvent()->GetName()[0] == '\0');
		if ((*pos)->getEvent() != nullptr && (*pos)->getEvent()->GetName()[0] != '\0')
			continue;
		CTransition::ConditionPair *c = (*pos)->getCondition();
		if ( eval(c) )
		{
			if (ChangeState(*pos, bHistoryPath) == IStateMachine::TO_NEXT_STATE)
				break;
		}
    }
}

CTransition* CTransientState::AddTrans(CState* s, IEvent* e, std::vector<CTransition::ActionPair> const &a, CTransition::ConditionPair* c, bool isNoTarget)
{
	//assert(e == nullptr);
	CTransition* newTransition = nullptr;
	if (s != this)
	{
		return CState::AddTrans(s, e, a, c, isNoTarget);
	}
	else
	{
		if (c != nullptr)
		{
			if ((m_vectTrans.size() > 0) && (m_vectTrans.back()->getCondition()->pCondition == nullptr))
			{
				TransVector::iterator itTrans = m_vectTrans.end();
				itTrans--;
				while ((itTrans != m_vectTrans.begin()) && (((*itTrans)->getState() != this) || ((*itTrans)->getCondition()->pCondition == nullptr)))
				{
					itTrans--;
				}

				if ((*itTrans)->getCondition()->pCondition != nullptr)
				{
					itTrans++;
				}
				newTransition = new CTransition(s, e, a, c, isNoTarget);
				m_vectTrans.insert(itTrans, newTransition);
			}
			else
			{
				newTransition = new CTransition(s, e, a, c, isNoTarget);
				m_vectTrans.push_front(newTransition);
			}
		}
		else
		{
			newTransition = new CTransition(s, e, a, c, isNoTarget);
			m_vectTrans.push_front(newTransition);
		}
		return newTransition;
	}
}