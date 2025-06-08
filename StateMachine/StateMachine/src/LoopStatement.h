#pragma once
#include <vector>
#include <string>
#include "IAction.h"
#include "ICondition.h"
#include "internal/DNA.h"

class LoopStatement : public IAction
{
public:
	struct ActionPair
	{
		ActionPair(IAction* action, IAction::ParamPairList& action_param) : pAction(action), lstActionParam(std::move(action_param)) {};
		ActionPair() : pAction(nullptr) {};
		IAction* pAction;
		IAction::ParamPairList lstActionParam;
	};
public:
	LoopStatement(BioSys::DNA* callback, std::vector<ActionPair> const& a, const char* name, int type);
	virtual ~LoopStatement() {};

public:
	virtual void execute(const ParamPairList& params);

	BioSys::DNA* callback_;

private:
	std::vector<ActionPair> a_;
};

