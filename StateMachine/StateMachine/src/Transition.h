#ifndef __TRANSITION_H__
#define __TRANSITION_H__

#include <vector>
#include <string>
#include "IAction.h"
#include "ICondition.h"

class CState;
class IEvent;

class CTransition
{
public:
	struct ActionPair
	{
		ActionPair(IAction* action, IAction::ParamPairList& action_param) : pAction(action), lstActionParam(std::move(action_param)) {};
		ActionPair() : pAction(nullptr) {};
		IAction* pAction;
		IAction::ParamPairList lstActionParam;
	};
	struct ConditionPair
	{
		ICondition* pCondition;
		ICondition::ParamPairList lstConditionParam;
		ConditionPair() :pCondition(nullptr) {};
		ConditionPair& operator=(const ConditionPair& src) { pCondition = src.pCondition; lstConditionParam = src.lstConditionParam; return *this; };
	};

public:
	CTransition(CState* s, IEvent* e, std::vector<CTransition::ActionPair> const &a = std::vector<CTransition::ActionPair>(), ConditionPair* c = nullptr, bool bisNoTarget = false);

	virtual ~CTransition()		{ /*if (m_pParam!=nullptr) delete [] m_pParam;*/ };

    CTransition& operator =(const CTransition& t);

    IEvent* getEvent();

	const std::vector<CTransition::ActionPair>& getAction();

    ConditionPair* getCondition();

    CState* getState();

    CState* get_Ancestor() const;

    void set_Ancestor(CState* left);

	bool isNoTarget() { return m_bIsNoTarget; };
private:
    CState* Ancestor;

    IEvent* _e;

    CState* _s;

    std::vector<ActionPair> _a;

	ConditionPair _c;

	bool m_bIsNoTarget;
};

#endif // __TRANSITION_H__
