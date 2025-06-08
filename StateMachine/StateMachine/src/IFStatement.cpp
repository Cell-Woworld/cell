#include "IFStatement.h"
#include "StateMachineImpl.h"

#define TAG "StateMachine"

IFStatement::IFStatement(BioSys::DNA* callback, std::vector<ActionPair> const& a, ConditionPair* c, const char* name, int type)
	: IAction(name, type)
	, callback_(callback)
{
	a_ = a;
	c_ = *c;
};

void IFStatement::execute(const ParamPairList& params)
{
	if (c_.pCondition==nullptr || c_.pCondition->eval(c_.lstConditionParam))
	{
		for (const auto& elem : a_)
		{
			if (strcmp(elem.pAction->name(), CStateMachineImpl::TAG_NAMES[ELSEIF])==0
				|| strcmp(elem.pAction->name(), CStateMachineImpl::TAG_NAMES[ELSE])==0)
			{
				break;
			}
			else if (strcmp(elem.pAction->name(), CStateMachineImpl::TAG_NAMES[IF]) == 0
				|| strcmp(elem.pAction->name(), CStateMachineImpl::TAG_NAMES[FOREACH]) == 0)
			{
				elem.pAction->execute(elem.lstActionParam);
			}
			else
			{
				callback_->on_action(elem.pAction->type(), elem.pAction->name(), elem.lstActionParam);
			}
		}
	}
	else
	{
		bool _found = false;
		for (const auto& elem : a_)
		{
			if (!_found && strcmp(elem.pAction->name(), CStateMachineImpl::TAG_NAMES[ELSEIF]) == 0 && callback_->on_condition(elem.lstActionParam[0].second.c_str(), elem.lstActionParam))
			{
				_found = true;
			}
			else if (_found)
			{
				if (strcmp(elem.pAction->name(), CStateMachineImpl::TAG_NAMES[ELSEIF]) == 0 || strcmp(elem.pAction->name(), CStateMachineImpl::TAG_NAMES[ELSE]) == 0)
				{
					break;
				}
				else if (strcmp(elem.pAction->name(), CStateMachineImpl::TAG_NAMES[IF]) == 0
						|| strcmp(elem.pAction->name(), CStateMachineImpl::TAG_NAMES[FOREACH]) == 0)
				{
					elem.pAction->execute(elem.lstActionParam);
				}
				else
				{
					callback_->on_action(elem.pAction->type(), elem.pAction->name(), elem.lstActionParam);
				}
			}
		}
		if (_found)
			return;
		for (const auto& elem : a_)
		{
			if (strcmp(elem.pAction->name(), CStateMachineImpl::TAG_NAMES[ELSE]) == 0)
			{
				_found = true;
			}
			else if (_found)
			{
				if (strcmp(elem.pAction->name(), CStateMachineImpl::TAG_NAMES[ELSEIF]) == 0 || strcmp(elem.pAction->name(), CStateMachineImpl::TAG_NAMES[ELSE]) == 0)
				{
					break;
				}
				else if (strcmp(elem.pAction->name(), CStateMachineImpl::TAG_NAMES[IF]) == 0
					|| strcmp(elem.pAction->name(), CStateMachineImpl::TAG_NAMES[FOREACH]) == 0)
				{
					elem.pAction->execute(elem.lstActionParam);
				}
				else
				{
					callback_->on_action(elem.pAction->type(), elem.pAction->name(), elem.lstActionParam);
				}
			}
		}
	}
}
