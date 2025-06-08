#include "LoopStatement.h"
#include "StateMachineImpl.h"

#define TAG "StateMachine"

LoopStatement::LoopStatement(BioSys::DNA* callback, std::vector<ActionPair> const& a, const char* name, int type)
	: IAction(name, type)
	, callback_(callback)
{
	a_ = a;
};

void LoopStatement::execute(const ParamPairList& params)
{
	String _index_model_name;
	int _index = 0;
	for (auto& elem : params)
	{
		if (elem.first == "index")
		{
			if (elem.second[0] == '-')
				_index_model_name = elem.second.substr(1);
			else
				_index_model_name = elem.second;
			callback_->Write(_index_model_name, -1);
			break;
		}
	}
	while (_index >= 0)
	{
		callback_->on_action(type(), name(), params);
		callback_->Read(_index_model_name, _index);
		if (_index < 0)
			break;
		for (const auto& elem : a_)
		{
			if (strcmp(elem.pAction->name(), CStateMachineImpl::TAG_NAMES[IF])==0
				|| strcmp(elem.pAction->name(), CStateMachineImpl::TAG_NAMES[FOREACH])==0)
			{
				elem.pAction->execute(elem.lstActionParam);
			}
			else
			{
				callback_->on_action(elem.pAction->type(), elem.pAction->name(), elem.lstActionParam);
			}
		}
		callback_->on_action(NEXT, name(), params);
		callback_->Read(_index_model_name, _index);
	}
}
