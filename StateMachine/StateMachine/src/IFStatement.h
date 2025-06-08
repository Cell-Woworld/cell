#pragma once
#include <vector>
#include <string>
#include "IAction.h"
#include "ICondition.h"
#include "internal/DNA.h"

class IFStatement : public IAction
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
	IFStatement(BioSys::DNA* callback, std::vector<ActionPair> const& a, ConditionPair* c, const char* name, int type);
	virtual ~IFStatement() {};

public:
	virtual void execute(const ParamPairList& params);

	BioSys::DNA* callback_;

private:
	std::vector<ActionPair> a_;
	ConditionPair c_;
};

