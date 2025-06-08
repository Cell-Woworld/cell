// StateMachineImpl.cpp: implementation of the CStateMachineImpl class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#include "StateMachineImpl.h"

#include <assert.h>
#include <sstream>
#include <memory>

#include "IState.h"
#include "State.h"
#include "IEvent.h"
#include "ICondition.h"
#include "IAction.h"
#include "Transition.h"
#include "NullCondition.h"
#include "TransientState.h"
#include "IFStatement.h"
#include "LoopStatement.h"

#include "History.h"
#include "DeepHistory.h"

#include "internal/DNA.h"
#include "internal/IModel.h"
#include "internal/utils/serializer.h"
#include <fstream>

#pragma warning (disable:4996)

#define TAG "StateMachine"

using namespace rapidxml;
using namespace std;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
const char* CStateMachineImpl::TAG_NAMES[] =
{
	"raise",
	"send",
	"script",
	"assign",
	"log",
	"if",
	"elseif",
	"else",
	"foreach",
	"foreach",
	"invoke",
	"invoke",
	"final",

	"state",
	"history",
    "shallow",
    "deep",
    "parallel",

	"onentry",
	"onexit",

	"param",

    "transition",

	"datamodel",
	"data",
};

enum ENUM_ATTR
{
	ID,
	INITIAL_STATE,
	TYPE,

	EVENT,
	CONDITION,

	SRC,
	SRCEXPR,
	TARGET,
	NAME,
	LOCATION,
	LABEL,
	EXPR,
	AUTOFORWARD,

	ARRAY,
	INDEX,
	ITEM,
};

const char* CStateMachineImpl::ATTR_NAMES[] =
{
	"id",
	"initial",
	"type",

	"event",
	"cond",

	"src",
	"srcexpr",
	"target",
	"name",
	"location",
	"label",
	"expr",
	"autoforward",

	"array",
	"index",
	"item",
};

class Condition : public ICondition
{
public:
	Condition(BioSys::DNA* callback, const char* name)
		: ICondition(name) 
		, callback_(callback)
	{
	};
	virtual ~Condition() {};

protected:
	virtual bool eval(const ParamPairList& params)
	{
		return callback_->on_condition(name(), params);
	}

	BioSys::DNA* callback_;
};

class Action : public IAction
{
public:
	Action(BioSys::DNA* callback, const char* name, int type) 
		: IAction(name, type) 
		, callback_(callback)
	{
	};
	virtual ~Action() {};

public:
	virtual void execute(const ParamPairList& params)
	{
		callback_->on_action(type(), name(), params);
	}

	BioSys::DNA* callback_;
};
CStateMachineImpl::CStateMachineImpl()
{
	m_strEntryStateID = "RootState";
}

CStateMachineImpl::~CStateMachineImpl()
{
    ClearAll();
}

bool CStateMachineImpl::Load(const char* szLogicMap, BioSys::DNA* callback, bool isFile)
{
	owner_ = callback;
    ClearAll();

	// this open and parse the XML file:
	xml_document<> doc;
	vector<char> buffer;
	if (isFile)
	{
		// Read the xml file into a vector
		ifstream theFile(szLogicMap);
		buffer = vector<char>((istreambuf_iterator<char>(theFile)), istreambuf_iterator<char>());
	}
	else
	{
		buffer.insert(buffer.end(), szLogicMap, szLogicMap + strlen(szLogicMap));
	}
	buffer.push_back('\0');
	// Parse the buffer using the xml file parsing library into doc
	doc.parse<0>(&buffer[0]);
	xml_node<>* xMainNode = doc.first_node("scxml");

    if (xMainNode == nullptr)
    {
		LOG_E(TAG, "CStateMachineImpl::Load() - oXMLParser.Load(%s) - false.", szLogicMap);
		assert(false);
		return false;
    }

    // Create all states according to XML
	m_mapStates[m_strEntryStateID] = new CState(m_strEntryStateID.c_str());
	if (false == CreateMap(callback, *xMainNode, m_mapStates[m_strEntryStateID]))
    {
        LOG_E(TAG, "CStateMachineImpl::Load() - CreateStateMap() - false");
		assert(false);
		return false;
    }

	LOG_D(TAG, "Adding Transitions");
    // Link states with transitions
    if (LinkTransitions(callback, *xMainNode) == false)
    {
		LOG_E(TAG, "CStateMachineImpl::Load() - LinkTransitions(oXMLParser) - false");
		assert(false);
		return false;
    }

	LOG_D(TAG, "Entering entry point");

    // power on the engine
    const char* szRootEntryStateID = nullptr;
	szRootEntryStateID = xMainNode->first_attribute(ATTR_NAMES[INITIAL_STATE]) == nullptr ? nullptr : xMainNode->first_attribute(ATTR_NAMES[INITIAL_STATE])->value();

    if (szRootEntryStateID == nullptr)
    {
		LOG_D(TAG, "CStateMachineImpl::Load() - no entry assigned, apply the first state");
		assert(xMainNode->first_node(TAG_NAMES[STATE]) != nullptr);
		const xml_node<>& _first_child = *xMainNode->first_node(TAG_NAMES[STATE]);
		szRootEntryStateID = _first_child.first_attribute(ATTR_NAMES[ID]) == nullptr ? nullptr : _first_child.first_attribute(ATTR_NAMES[ID])->value();
		//assert(false);
		//return false;
    }

    map<string, CState*>::const_iterator iterItem = m_mapStates.find(m_strEntryStateID);
    if ( iterItem == m_mapStates.end() )
    {
		LOG_E(TAG, "CStateMachineImpl::Load() - entry not found in map!!!");
		assert(false);
		return false;
    }
    else
    {
		m_mapStates[m_strEntryStateID]->InitialState(m_mapStates[szRootEntryStateID]);
        m_mapStates[m_strEntryStateID]->Enter();
    }

    return true;
}

int CStateMachineImpl::EventFired(const char* szEventID)
{
	EventMap::iterator iterItem = m_mapEvents.find(szEventID);
	if (iterItem != m_mapEvents.end())
		return m_mapStates[m_strEntryStateID]->EventFired(iterItem->second);
	else
	{
		std::string _event_id = szEventID;
		size_t _pos = std::string::npos;
		while ((_pos = _event_id.rfind('.')) != std::string::npos)
		{
			_event_id = _event_id.substr(0, _pos);
			EventMap::iterator iterItem = m_mapEvents.find(_event_id + ".*");
			if (iterItem != m_mapEvents.end())
				return m_mapStates[m_strEntryStateID]->EventFired(iterItem->second);
		}
		EventMap::iterator iterItem = m_mapEvents.find("*");
		if (iterItem != m_mapEvents.end())
			return m_mapStates[m_strEntryStateID]->EventFired(iterItem->second);
		return -1;	// IStateMachine::ENUM_FIRE_EVENT_RESULT::NOTHING_TO_DO
	}
}

void CStateMachineImpl::GetActiveStates(const char* lstActiveState[], const int iMaxListSize)
{
    assert(lstActiveState != nullptr);
    int iTotalCount = 0;

    for (int i=0; i<iMaxListSize; i++)
        lstActiveState[i] = nullptr;

    m_mapStates[m_strEntryStateID]->get_ActiveStates(lstActiveState, iMaxListSize, iTotalCount);
}

void CStateMachineImpl::SetActiveStates(const char* lstActiveState[], const int iMaxListSize)
{
    for (int i=0;  (i<iMaxListSize)&&(lstActiveState[i]!=nullptr); i++)
    {
        if (lstActiveState[i] != nullptr)
        {
			//vector<string> _state_name_list;
			//_Split(_state_name_list, lstActiveState[i], "/");
			//for (auto _state : _state_name_list)
			string _state = string(lstActiveState[i]);
			if (_state.front() == '\"' && _state.back() == '\"')
				_state = _state.substr(1, _state.size() - 2);
			size_t _pos = _state.find_last_of('/');
			if (_pos != string::npos && _pos < _state.size() - 1)
				_state = _state.substr(_pos + 1);
			StateMap::iterator itStateMap = m_mapStates.find(_state);
			if (itStateMap != m_mapStates.end())
			{
				itStateMap->second->set_CurrentStateLink();
			}
        }
    }
}

void CStateMachineImpl::ClearAll()
{
    DeleteValuePtrAndClearMap<StateMap>(m_mapStates);
	DeletePtrInArrayAndClearMap<EventMap>(m_mapEvents);
    DeleteValuePtrAndClearMap<ConditionMap>(m_mapConditions);
    DeleteValuePtrAndClearMap<ActionMap>(m_mapActions);
}

template< typename _Map >
void CStateMachineImpl::DeleteValuePtrAndClearMap( _Map &Map )
{
    if (Map.size() > 0)
    {
        for(auto aPair : Map)
        {
			delete aPair.second;
        }

        Map.clear();
    }
}

template< typename _Map >
void CStateMachineImpl::DeletePtrInArrayAndClearMap(_Map& Map)
{
	if (Map.size() > 0)
	{
		for (auto aPair : Map)
		{
			if (aPair.second.size() > 0)
				delete aPair.second[0];		// only need to delete 1st, others will be deleted later
		}

		Map.clear();
	}
}

bool CStateMachineImpl::CreateMap(BioSys::DNA* callback, const xml_node<>& xNode, CState* pParentNode)
{
	vector<int> vectStateID = { STATE, FINAL_STATE, PARALLEL};
	for (auto const &nStateID : vectStateID)
	{
		// Iterate over the states
		for (xml_node<>* itStateNode = xNode.first_node(TAG_NAMES[nStateID]); itStateNode; itStateNode = itStateNode->next_sibling(TAG_NAMES[nStateID]))
		{
			const char* _state_name = itStateNode->first_attribute(ATTR_NAMES[ID]) == nullptr ? nullptr : itStateNode->first_attribute(ATTR_NAMES[ID])->value();

			if (m_mapStates.count(_state_name) > 0)
			{
				LOG_E(TAG, "!!! CStateMachineImpl::CreateStateMap() - state name:%s has already in m_mapStates", _state_name);
				assert(false);
				return false;
			}

			CreateDataModel(*itStateNode, m_mapDataModel);

			CState *pNewState = nullptr;
			if (CheckIfTransientState(*itStateNode) == true)
				pNewState = new CTransientState(_state_name);
			else
				pNewState = new CState(_state_name, nStateID);
			m_mapStates[_state_name] = pNewState;

			LOG_D(TAG, "%s[%s] attached", TAG_NAMES[nStateID], _state_name);
			if (pParentNode != nullptr)
			{
				pParentNode->AddState(m_mapStates[_state_name]);
			}

			if (CreateMap(callback, *itStateNode, m_mapStates[_state_name]) == false)
			{
				assert(false);
				return false;
			}

			const char* cszInitStateName = nullptr;
			if (nStateID == STATE)
			{
				if (itStateNode->first_attribute(ATTR_NAMES[INITIAL_STATE]) != nullptr)
					cszInitStateName = itStateNode->first_attribute(ATTR_NAMES[INITIAL_STATE]) == nullptr ? nullptr : itStateNode->first_attribute(ATTR_NAMES[INITIAL_STATE])->value();
				else
				{
					const xml_node<>* _first_child = itStateNode->first_node(TAG_NAMES[STATE]);
					if (_first_child != nullptr && _first_child->first_attribute(ATTR_NAMES[ID]) != nullptr)
						cszInitStateName = _first_child->first_attribute(ATTR_NAMES[ID]) == nullptr ? nullptr : _first_child->first_attribute(ATTR_NAMES[ID])->value();
				}
			}
			if (cszInitStateName != nullptr)
			{
				if (m_mapStates.count(cszInitStateName) == 1)
				{
					m_mapStates[_state_name]->InitialState(m_mapStates[cszInitStateName]);
				}
				else
				{
					LOG_E(TAG, "!!! CStateMachineImpl::CreateStateMap() - The state[%s] is ths StartState, but the pParentNode is nullptr.", cszInitStateName);
					assert(false);
					return false;
				}
			}

			vector<CTransition::ActionPair> _invoke_action_pair_list;
			getInvokeList(callback, *itStateNode, INVOKE, _invoke_action_pair_list);
			vector<CTransition::ActionPair> _uninvoke_action_pair_list;
			getInvokeList(callback, *itStateNode, UNINVOKE, _uninvoke_action_pair_list);
			m_mapStates[_state_name]->add_invoke_action(_invoke_action_pair_list, _uninvoke_action_pair_list);

			bool _onentry = itStateNode->first_node(TAG_NAMES[ON_ENTRY]) != nullptr;
			if (_onentry)
			{
				vector<CTransition::ActionPair> _action_pair_list;
				getActionList(callback, *itStateNode->first_node(TAG_NAMES[ON_ENTRY]), _action_pair_list);
				m_mapStates[_state_name]->add_entry_action(_action_pair_list);
			}

			bool _onexit = itStateNode->first_node(TAG_NAMES[ON_EXIT]) != nullptr;
			if (_onexit)
			{
				vector<CTransition::ActionPair> _action_pair_list;
				getActionList(callback, *itStateNode->first_node(TAG_NAMES[ON_EXIT]), _action_pair_list);
				m_mapStates[_state_name]->add_exit_action(_action_pair_list);
			}

			if (nStateID == FINAL_STATE)
			{
				vector<CTransition::ActionPair> _action_pair_list;
				_action_pair_list.push_back(CTransition::ActionPair());
				CTransition::ActionPair& _action_pair = _action_pair_list.back();
				const char* _action_name = itStateNode->first_attribute(ATTR_NAMES[ID]) == nullptr ? nullptr : itStateNode->first_attribute(ATTR_NAMES[ID])->value();
				pair<string, int> _action_key = make_pair(_action_name, FINAL_STATE);
				_action_pair.pAction = new Action(callback, _action_name, FINAL_STATE);
				m_mapActions[_action_key] = _action_pair.pAction;
				m_mapStates[_state_name]->add_entry_action(_action_pair_list);
			}
			else if (nStateID == PARALLEL)
			{
				m_mapStates[_state_name]->set_ConcurrentState(true);
			}

			for (xml_node<>* itTransitionNode = itStateNode->first_node(TAG_NAMES[TRANSITION]); itTransitionNode; itTransitionNode = itTransitionNode->next_sibling(TAG_NAMES[TRANSITION]))
			{
				// event
				std::string _event_name = itTransitionNode->first_attribute(ATTR_NAMES[EVENT]) == nullptr ? "" : itTransitionNode->first_attribute(ATTR_NAMES[EVENT])->value();
				ReplaceDataModel(_event_name);
				if (m_mapEvents.count(_event_name) == 0)
				{
					m_mapEvents[_event_name].push_back(new IEvent(_event_name.c_str()));
				}

				LOG_D(TAG, "%s[%s] attached", ATTR_NAMES[EVENT], _event_name.c_str());
			}
		}	//	for (int i = 0; i < nStateCount; ++i)
	}
	for (auto& event : m_mapEvents)
	{
		if (event.first.find('*') == string::npos)
		{
			SetPriorityEventList(event.first, event.second);
		}
	}

	{	// History
		const int THE_SAME_STRING = 0;
		for (xml_node<>* xHistoryNode = xNode.first_node(TAG_NAMES[HISTORY]); xHistoryNode; xHistoryNode = xHistoryNode->next_sibling(TAG_NAMES[HISTORY]))
		{
			const char* szType = xHistoryNode->first_attribute(ATTR_NAMES[TYPE]) == nullptr ? nullptr : xHistoryNode->first_attribute(ATTR_NAMES[TYPE])->value();
			const char* szName = xHistoryNode->first_attribute(ATTR_NAMES[ID]) == nullptr ? nullptr : xHistoryNode->first_attribute(ATTR_NAMES[ID])->value();
			//if (stricmp(szType, TAG_NAMES[DEEP_HISTORY]) == THE_SAME_STRING)			// "undeclared identifire 'stricmp'" reported by ndk
			if (strcmp(szType, TAG_NAMES[DEEP_HISTORY]) == THE_SAME_STRING)
			{
				CState *pNewState = pParentNode->DeepHistoryEntry();
				m_mapStates[pNewState->GetName()] = pNewState;
				m_mapHistory[szName] = pNewState;
				LOG_D(TAG, "%s[%s] attached", TAG_NAMES[DEEP_HISTORY], szName);
			}
			//else if (stricmp(szType, TAG_NAMES[SHALLOW_HISTORY]) == THE_SAME_STRING)
			else if (strcmp(szType, TAG_NAMES[SHALLOW_HISTORY]) == THE_SAME_STRING)		// "undeclared identifire 'stricmp'" reported by ndk
			{
				CState *pNewState = pParentNode->HistoryEntry();
				m_mapStates[pNewState->GetName()] = pNewState;
				m_mapHistory[szName] = pNewState;
				LOG_D(TAG, "%s[%s] attached", TAG_NAMES[HISTORY], szName);
			}
		}
	}
	return true;
}

bool CStateMachineImpl::LinkTransitions(BioSys::DNA* callback, const xml_node<>& xNode)
{
	vector<int> vectStateID = { STATE, FINAL_STATE, PARALLEL };
	for (auto const &nStateID : vectStateID)
	{
		for (xml_node<>* itStateNode = xNode.first_node(TAG_NAMES[nStateID]); itStateNode; itStateNode = itStateNode->next_sibling(TAG_NAMES[nStateID]))
		{
			if (LinkTransitions(callback, *itStateNode) == false)
			{
				assert(false);
				return false;
			}

			for (xml_node<>* itTransitionNode = itStateNode->first_node(TAG_NAMES[TRANSITION]); itTransitionNode; itTransitionNode = itTransitionNode->next_sibling(TAG_NAMES[TRANSITION]))
			{
				CState* pFromState = m_mapStates[itStateNode->first_attribute(ATTR_NAMES[ID])->value()];
				CState* pToState = nullptr;
				bool bIsNoTargetTransition = false;
				if (itTransitionNode->first_attribute(ATTR_NAMES[TARGET]) != nullptr)
				{
					const char* _state_name = itTransitionNode->first_attribute(ATTR_NAMES[TARGET]) == nullptr ? nullptr : itTransitionNode->first_attribute(ATTR_NAMES[TARGET])->value();
					const auto& _itr = m_mapStates.find(_state_name);
					if (_itr != m_mapStates.end())
					{
						pToState = _itr->second;
					}
					else
					{ 
						const auto& _itr = m_mapHistory.find(_state_name);
						if (_itr != m_mapHistory.end())
							pToState = _itr->second;
					}
				}
				else
				{
					bIsNoTargetTransition = true;
					if (pFromState->isCompositeState() && !pFromState->get_ConcurrentState())
					{
						pToState = pFromState->DeepHistoryEntry();
						m_mapStates[pToState->GetName().c_str()] = pToState;
						LOG_D(TAG, "%s[%s] attached", TAG_NAMES[DEEP_HISTORY], pToState->GetName().c_str());
					}
					else
					{
						pToState = pFromState;
					}
				}

				IEvent* pEvent = nullptr;
				if (itTransitionNode->first_attribute(ATTR_NAMES[EVENT]) != nullptr)
				{
					string strEventName = itTransitionNode->first_attribute(ATTR_NAMES[EVENT]) == nullptr ? "" : itTransitionNode->first_attribute(ATTR_NAMES[EVENT])->value();
					ReplaceDataModel(strEventName);
					pEvent = m_mapEvents[strEventName][0];
				}
				else if (pFromState->GetName().find('*') != string::npos)
				{
					LOG_E(TAG, "!!! Invalid transition: [%s]->[%s] without %s", pFromState->GetName().c_str(), pToState->GetName().c_str(), TAG_NAMES[EVENT]);
					assert(false);
				}

				unique_ptr<CTransition::ConditionPair> pCondition;
				const char* cszConditionName = itTransitionNode->first_attribute(ATTR_NAMES[CONDITION]) == nullptr ? nullptr : itTransitionNode->first_attribute(ATTR_NAMES[CONDITION])->value();
				if (cszConditionName != nullptr)
				{
					string strFullConditionName = cszConditionName;
					string strConditionName = "";

					ReplaceDataModel(strFullConditionName);

					std::size_t found = strFullConditionName.find_first_of("(");
					if (found != string::npos)
					{
						std::size_t _invalid_function_name = strFullConditionName.find_last_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_");
						if (_invalid_function_name == string::npos)
						{
							strConditionName = strFullConditionName.substr(0, found);
							if (strConditionName.length() == 0)
								strConditionName = strFullConditionName;
						}
						else
						{
							strConditionName = strFullConditionName;
						}
					}
					else
					{
						strConditionName = strFullConditionName;
					}

					if (strConditionName.length() > 0)
					{
						pCondition = unique_ptr<CTransition::ConditionPair>(new CTransition::ConditionPair());
						if (m_mapConditions.count(strConditionName) > 0)
							pCondition->pCondition = m_mapConditions[strConditionName];
						else
						{
							pCondition->pCondition = new Condition(callback, strConditionName.c_str());
							m_mapConditions[strConditionName] = pCondition->pCondition;
						}

						if (found != string::npos)
						{
							size_t _param_begin = strFullConditionName.find_first_not_of(' ', found + 1);
							if (_param_begin != std::string::npos)
							{
								size_t _param_end = strFullConditionName.find_last_not_of(" )");
								if (_param_end != std::string::npos && _param_end > _param_begin + 1)
									pCondition->lstConditionParam.push_back(make_pair("params_in_JSON", strFullConditionName.substr(_param_begin, _param_end - _param_begin + 1)));
							}
						}
					}
				}
				else
				{
					// no condition.
				}

				vector<CTransition::ActionPair> vectActions;
				getActionList(callback, *itTransitionNode, vectActions);

				if (pToState == nullptr)
					LOG_D(TAG, "Target state==nullptr, source state name=%s", pFromState->GetName().c_str());
				assert(pToState != nullptr);
				CTransition* pNewTrans = pFromState->AddTrans(pToState, pEvent, vectActions, pCondition.get(), bIsNoTargetTransition);
			}	// for (int i = 0; i < nTransitionCount; ++i)
		}	//	for (int i = 0; i < nStateCount; ++i)
	}
    return true;
}

void CStateMachineImpl::ResetFSM()
{
	for (map<string, CState*>::const_iterator iterItem=m_mapStates.begin(); iterItem != m_mapStates.end(); ++iterItem)
	{
		if (iterItem->second->hasHistory())
			iterItem->second->HistoryEntry()->set_CurrentState(nullptr);
		if (iterItem->second->hasDeepHistory())
			iterItem->second->DeepHistoryEntry()->set_CurrentState(nullptr);
	}
	m_mapStates[m_strEntryStateID]->Enter();
}

void CStateMachineImpl::_Split(vector<string>& vectOutput, const string& strSrc, const string& separator)
{
	size_t found;
	string str = strSrc;
	found = str.find_first_of(separator);
	while (found != string::npos)
	{
		vectOutput.push_back(str.substr(0, found));
		str = str.substr(found + 1);
		found = str.find_first_not_of("\n ");
		if (found != string::npos)
			str = str.substr(found);
		found = str.find_first_of(separator);
	}
	if (str.length() > 0)
	{
		vectOutput.push_back(str);
	}
}

template<class T>
void CStateMachineImpl::getActionList(BioSys::DNA* callback, const xml_node<>& root_node, vector<T>& action_pair_list)
{
	for (xml_node<>* _action_node = root_node.first_node(); _action_node; _action_node = _action_node->next_sibling())
	{
		action_pair_list.push_back(T());
		T& _action_pair = action_pair_list.back();
		if (strcmp(_action_node->name(), TAG_NAMES[RAISE]) == 0)
		{
			const char* _action_name = _action_node->first_attribute(ATTR_NAMES[EVENT]) == nullptr ? nullptr : _action_node->first_attribute(ATTR_NAMES[EVENT])->value();
			if (_action_name == nullptr)
			{
				LOG_E(TAG, "Action \"%s\" with event name=NULL in transition \"%s\" or state \"%s\"", TAG_NAMES[RAISE], root_node.first_attribute(ATTR_NAMES[EVENT])->value(), root_node.first_attribute(ATTR_NAMES[ID])->value());
				assert(false);
				return;
			}
			string _actionName = _action_name;
			ReplaceDataModel(_actionName);
			pair<string, int> _action_key = make_pair(_actionName, RAISE);
			if (m_mapActions.count(_action_key) > 0)
				_action_pair.pAction = m_mapActions[_action_key];
			else
			{
				_action_pair.pAction = new Action(callback, _actionName.c_str(), RAISE);
				m_mapActions[_action_key] = _action_pair.pAction;
			}
		}
		else if (strcmp(_action_node->name(), TAG_NAMES[SEND]) == 0)
		{
			const char* _action_name = _action_node->first_attribute(ATTR_NAMES[EVENT]) == nullptr ? nullptr : _action_node->first_attribute(ATTR_NAMES[EVENT])->value();
			if (_action_name == nullptr)
			{
				LOG_E(TAG, "Action \"%s\" with event name=NULL in transition \"%s\" or state \"%s\"", TAG_NAMES[SEND], root_node.first_attribute(ATTR_NAMES[EVENT])->value(), root_node.first_attribute(ATTR_NAMES[ID])->value());
				assert(false);
				return;
			}
			string _actionName = _action_name;
			ReplaceDataModel(_actionName);
			pair<string, int> _action_key = make_pair(_actionName, SEND);
			if (m_mapActions.count(_action_key) > 0)
				_action_pair.pAction = m_mapActions[_action_key];
			else
			{
				_action_pair.pAction = new Action(callback, _actionName.c_str(), SEND);
				m_mapActions[_action_key] = _action_pair.pAction;
			}
			for (xml_node<>* _param_node = _action_node->first_node(TAG_NAMES[PARAM]); _param_node; _param_node = _param_node->next_sibling(TAG_NAMES[PARAM]))
			{
				//if (_param_node.first_attribute(ATTR_NAMES[NAME]) == nullptr || _param_node.first_attribute(ATTR_NAMES[EXPR]) == nullptr)
				if (_param_node->first_attribute(ATTR_NAMES[NAME]) == nullptr)
				{
					LOG_E(TAG, "!!! Invalid parameter. 'Send' field in state \"%s\" is null.", root_node.parent()->first_attribute(ATTR_NAMES[ID])->value());
					action_pair_list.pop_back();
					assert(false);
				}
				else if (_param_node->first_attribute(ATTR_NAMES[EXPR]) == nullptr)
				{
					//LOG_E(TAG, "!!! Invalid value of parameter \"%s\". expr of 'send' field in state \"%s\" is null.", _param_node.first_attribute(ATTR_NAMES[NAME]), root_node.getParentNode().first_attribute(ATTR_NAMES[ID]));
					std::string _expr = "";
					ReplaceDataModel(_expr);
					_action_pair.lstActionParam.push_back(make_pair(_param_node->first_attribute(ATTR_NAMES[NAME])->value(), _expr));
				}
				else
				{
					std::string _expr = _param_node->first_attribute(ATTR_NAMES[EXPR]) == nullptr ? "" : _param_node->first_attribute(ATTR_NAMES[EXPR])->value();
					ReplaceDataModel(_expr);
					_action_pair.lstActionParam.push_back(make_pair(_param_node->first_attribute(ATTR_NAMES[NAME])->value(), _expr));
				}
			}
		}
		else if (strcmp(_action_node->name(), TAG_NAMES[SCRIPT]) == 0)
		{
			vector<string> action_name_list;
			_Split(action_name_list, _action_node->value()==nullptr ? "" : _action_node->value(), ";");
			if (action_name_list.size() == 0)
			{
				LOG_E(TAG, "!!! Invalid script. Script of state \"%s\" is null.", root_node.parent()->first_attribute(ATTR_NAMES[ID])->value());
				action_pair_list.pop_back();
				assert(false);
			}
			for (auto _action_full_name : action_name_list)
			{
				string _action_name = _action_full_name;
				std::size_t _found = _action_full_name.find_first_of("(");
				if (_found != string::npos)
					_action_name = _action_full_name.substr(0, _found);

				ReplaceDataModel(_action_name);

				pair<string, int> _action_key = make_pair(_action_name, SCRIPT);
				if (m_mapActions.count(_action_key) > 0)
					_action_pair.pAction = m_mapActions[_action_key];
				else
				{
					_action_pair.pAction = new Action(callback, _action_name.c_str(), SCRIPT);
					m_mapActions[_action_key] = _action_pair.pAction;
				}
				if (_found != string::npos)
				{
					size_t _param_begin = _action_full_name.find_first_not_of(' ', _found + 1);
					if (_param_begin != std::string::npos)
					{
						size_t _param_end = _action_full_name.find_last_not_of(" )");
						if (_param_end != std::string::npos && _param_end > _param_begin + 1)
						{
							string _params = _action_full_name.substr(_param_begin, _param_end - _param_begin + 1);
							ReplaceDataModel(_params);
							_action_pair.lstActionParam.push_back(make_pair("params_in_JSON", _params));
						}
					}
				}
			}
		}
		else if (strcmp(_action_node->name(), TAG_NAMES[ASSIGN]) == 0)
		{
			const char _action_name[] = "Assign";
			pair<string, int> _action_key = make_pair(_action_name, ASSIGN);
			if (m_mapActions.count(_action_key) > 0)
				_action_pair.pAction = m_mapActions[_action_key];
			else
			{
				_action_pair.pAction = new Action(callback, _action_name, ASSIGN);
				m_mapActions[_action_key] = _action_pair.pAction;
			}
			//if (_action_node.first_attribute(ATTR_NAMES[LOCATION]) == nullptr || _action_node.first_attribute(ATTR_NAMES[EXPR]) == nullptr)
			if (_action_node->first_attribute(ATTR_NAMES[LOCATION]) == nullptr)
			{
				//LOG_E(TAG, "Assign model with location==null or expression==null!");
				LOG_E(TAG, "Assign model with location==null");
				assert(false);
			}
			else
			{
				std::string _location = _action_node->first_attribute(ATTR_NAMES[LOCATION]) == nullptr ? "" : _action_node->first_attribute(ATTR_NAMES[LOCATION])->value();
				ReplaceDataModel(_location);
				std::string _expr = "";
				if (_action_node->first_attribute(ATTR_NAMES[EXPR]) != nullptr)
					_expr = _action_node->first_attribute(ATTR_NAMES[EXPR]) == nullptr ? "" : _action_node->first_attribute(ATTR_NAMES[EXPR])->value();
				ReplaceDataModel(_expr);
				_action_pair.lstActionParam.push_back(make_pair(_location, _expr));
			}
		}
		else if (strcmp(_action_node->name(), TAG_NAMES[LOG]) == 0)
		{
			const char _action_name[] = "Log";
			pair<string, int> _action_key = make_pair(_action_name, LOG);
			if (m_mapActions.count(_action_key) > 0)
				_action_pair.pAction = m_mapActions[_action_key];
			else
			{
				_action_pair.pAction = new Action(callback, _action_name, LOG);
				m_mapActions[_action_key] = _action_pair.pAction;
			}
			std::string _label = _action_node->first_attribute(ATTR_NAMES[LABEL]) == nullptr ? "" : _action_node->first_attribute(ATTR_NAMES[LABEL])->value();
			ReplaceDataModel(_label);
			_action_pair.lstActionParam.push_back(make_pair(ATTR_NAMES[LABEL], _label));
			std::string _expr = _action_node->first_attribute(ATTR_NAMES[EXPR]) == nullptr ? "" : _action_node->first_attribute(ATTR_NAMES[EXPR])->value();
			ReplaceDataModel(_expr);
			_action_pair.lstActionParam.push_back(make_pair(ATTR_NAMES[EXPR], _expr));
		}
		else if (strcmp(_action_node->name(), TAG_NAMES[IF]) == 0)
		{
			if (_action_node->first_attribute(ATTR_NAMES[CONDITION]) == nullptr)
			{
				LOG_E(TAG, "If statement with condition==null");
				assert(false);
				return;
			}

			const char* _action_name = _action_node->name();
			//std::string _condition = _action_node.first_attribute(ATTR_NAMES[CONDITION]);
			//ReplaceDataModel(_condition);
			//_action_pair.lstActionParam.push_back(make_pair(ATTR_NAMES[CONDITION], _condition));
			unique_ptr<IFStatement::ConditionPair> pCondition;
			const char* cszConditionName = _action_node->first_attribute(ATTR_NAMES[CONDITION]) == nullptr ? nullptr : _action_node->first_attribute(ATTR_NAMES[CONDITION])->value();
			string strFullConditionName = cszConditionName;
			string strConditionName = "";

			ReplaceDataModel(strFullConditionName);

			std::size_t found = strFullConditionName.find_first_of("(");
			if (found != string::npos)
			{
				std::size_t _invalid_function_name = strFullConditionName.find_last_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_");
				if (_invalid_function_name == string::npos)
				{
					strConditionName = strFullConditionName.substr(0, found);
					if (strConditionName.length() == 0)
						strConditionName = strFullConditionName;
				}
				else
				{
					strConditionName = strFullConditionName;
				}
			}
			else
			{
				strConditionName = strFullConditionName;
			}

			if (strConditionName.length() > 0)
			{
				pCondition = unique_ptr<IFStatement::ConditionPair>(new IFStatement::ConditionPair());
				if (m_mapConditions.count(strConditionName) > 0)
					pCondition->pCondition = m_mapConditions[strConditionName];
				else
				{
					pCondition->pCondition = new Condition(callback, strConditionName.c_str());
					m_mapConditions[strConditionName] = pCondition->pCondition;
				}

				if (found != string::npos)
				{
					size_t _param_begin = strFullConditionName.find_first_not_of(' ', found + 1);
					if (_param_begin != std::string::npos)
					{
						size_t _param_end = strFullConditionName.find_last_not_of(" )");
						if (_param_end != std::string::npos && _param_end > _param_begin + 1)
							pCondition->lstConditionParam.push_back(make_pair("params_in_JSON", strFullConditionName.substr(_param_begin, _param_end - _param_begin + 1)));
					}
				}
			}

			vector<IFStatement::ActionPair> vectActions;
			getActionList(callback, *_action_node, vectActions);
			IFStatement* _IFStatement = new IFStatement(callback, vectActions, pCondition.get(), _action_name, IF);
			_action_pair.pAction = _IFStatement;
		}
		else if (strcmp(_action_node->name(), TAG_NAMES[ELSEIF]) == 0)
		{
			const char* _action_name = _action_node->name();
			_action_pair.pAction = new Action(callback, _action_name, ELSEIF);

			std::string _condition = _action_node->first_attribute(ATTR_NAMES[CONDITION]) == nullptr ? "" : _action_node->first_attribute(ATTR_NAMES[CONDITION])->value();
			ReplaceDataModel(_condition);
			_action_pair.lstActionParam.push_back(make_pair(ATTR_NAMES[CONDITION], _condition));
		}
		else if (strcmp(_action_node->name(), TAG_NAMES[ELSE]) == 0)
		{
			const char* _action_name = _action_node->name();
			_action_pair.pAction = new Action(callback, _action_name, ELSE);
		}
		else if (strcmp(_action_node->name(), TAG_NAMES[FOREACH]) == 0)
		{
			vector<LoopStatement::ActionPair> vectActions;
			getActionList(callback, *_action_node, vectActions);

			const char* _action_name = _action_node->name();
			_action_pair.pAction = new LoopStatement(callback, vectActions, _action_name, FOREACH);

			std::string _array = _action_node->first_attribute(ATTR_NAMES[ARRAY]) == nullptr ? "" : _action_node->first_attribute(ATTR_NAMES[ARRAY])->value();
			ReplaceDataModel(_array);
			_action_pair.lstActionParam.push_back(make_pair(ATTR_NAMES[ARRAY], _array));
			std::string _index = _action_node->first_attribute(ATTR_NAMES[INDEX]) == nullptr ? "" : _action_node->first_attribute(ATTR_NAMES[INDEX])->value();
			ReplaceDataModel(_index);
			_action_pair.lstActionParam.push_back(make_pair(ATTR_NAMES[INDEX], _index));
			std::string _item = _action_node->first_attribute(ATTR_NAMES[ITEM]) == nullptr ? "" : _action_node->first_attribute(ATTR_NAMES[ITEM])->value();
			ReplaceDataModel(_item);
			_action_pair.lstActionParam.push_back(make_pair(ATTR_NAMES[ITEM], _item));
		}
		else
		{
			action_pair_list.pop_back();
		}
	}
}

bool CStateMachineImpl::CheckIfTransientState(const xml_node<>& root_node)
{
	for (xml_node<>* itTransitionNode = root_node.first_node(TAG_NAMES[TRANSITION]); itTransitionNode; itTransitionNode = itTransitionNode->next_sibling(TAG_NAMES[TRANSITION]))
	{
		// event
		xml_attribute<>* _event_attr = itTransitionNode->first_attribute(ATTR_NAMES[EVENT]);
		if (_event_attr == nullptr || _event_attr->value()[0] == '\0')
			return true;
	}
	return false;
}

void CStateMachineImpl::getInvokeList(BioSys::DNA* callback, const xml_node<>& root_node, int type, vector<CTransition::ActionPair>& action_pair_list)
{
	for (xml_node<>* _invoke = root_node.first_node(TAG_NAMES[type]); _invoke; _invoke = _invoke->next_sibling(TAG_NAMES[type]))
	{
		action_pair_list.push_back(CTransition::ActionPair());
		CTransition::ActionPair& _action_pair = action_pair_list.back();

		const char* _id = _invoke->first_attribute(ATTR_NAMES[ID]) == nullptr ? nullptr : _invoke->first_attribute(ATTR_NAMES[ID])->value();
		const char* _src = _invoke->first_attribute(ATTR_NAMES[SRC]) == nullptr ? nullptr : _invoke->first_attribute(ATTR_NAMES[SRC])->value();
		const char* _srcexpr = _invoke->first_attribute(ATTR_NAMES[SRCEXPR]) == nullptr ? nullptr : _invoke->first_attribute(ATTR_NAMES[SRCEXPR])->value();
		const char* _autoforward = _invoke->first_attribute(ATTR_NAMES[AUTOFORWARD])==nullptr ? nullptr : _invoke->first_attribute(ATTR_NAMES[AUTOFORWARD])->value();
		string _src_str;
		if (_src == nullptr && _srcexpr == nullptr)
		{
			if (_id != nullptr)
			{
				LOG_E(TAG, "!!! Invalid src. src of invoke with id=\"%s\" in state \"%s\" is null.", _id, root_node.first_attribute(ATTR_NAMES[ID])->value());
			}
			else
			{
				LOG_E(TAG, "!!! Invalid src. src of invoke with id=\"unknown\" in state \"%s\" is null.", root_node.first_attribute(ATTR_NAMES[ID])->value());
			}
			action_pair_list.pop_back();
			assert(false);
		}
		else if (_srcexpr != nullptr)
		{
			_src_str = _srcexpr;
			ReplaceDataModel(_src_str);
			_srcexpr = _src_str.data();

		}
		else
		{
			_src_str = _src;
			ReplaceDataModel(_src_str);
			_src = _src_str.data();

		}
		if (_id == nullptr)
			_id = _src;
		const char* _action_name = _id;
		pair<string, int> _action_key = make_pair(_action_name, type);
		if (m_mapActions.count(_action_key) > 0)
		{
			LOG_W(TAG, "Invoke in state \"%s\" with the same id \"%s\", is that right?", root_node.first_attribute(ATTR_NAMES[ID])->value(), _id);
			_action_pair.pAction = m_mapActions[_action_key];
		}
		else
		{
			_action_pair.pAction = new Action(callback, _action_name, type);
			m_mapActions[_action_key] = _action_pair.pAction;
		}
		_action_pair.lstActionParam.push_back(make_pair(ATTR_NAMES[ID], _id));
		if (_src != nullptr)
			_action_pair.lstActionParam.push_back(make_pair(ATTR_NAMES[SRC], _src));
		if (_srcexpr != nullptr)
			_action_pair.lstActionParam.push_back(make_pair(ATTR_NAMES[SRCEXPR], _srcexpr));
		if (_autoforward != nullptr)
			_action_pair.lstActionParam.push_back(make_pair(ATTR_NAMES[AUTOFORWARD], _autoforward));
		else
			_action_pair.lstActionParam.push_back(make_pair(ATTR_NAMES[AUTOFORWARD], "true"));

		for (xml_node<>* _param_node = _invoke->first_node(TAG_NAMES[PARAM]); _param_node; _param_node = _param_node->next_sibling(TAG_NAMES[PARAM]))
		{
			//if (_param_node.first_attribute(ATTR_NAMES[NAME]) == nullptr || _param_node.first_attribute(ATTR_NAMES[EXPR]) == nullptr)
			if (_param_node->first_attribute(ATTR_NAMES[NAME]) == nullptr)
			{
				LOG_E(TAG, "!!! Invalid parameter. 'invoke' field in state \"%s\" is null.", root_node.parent()->first_attribute(ATTR_NAMES[ID])->value());
				action_pair_list.pop_back();
				assert(false);
			}
			else if (_param_node->first_attribute(ATTR_NAMES[EXPR]) == nullptr)
			{
				//LOG_E(TAG, "!!! Invalid value of parameter \"%s\". expr of 'invoke' field in state \"%s\" is null.", _param_node.first_attribute(ATTR_NAMES[NAME]), root_node.getParentNode().first_attribute(ATTR_NAMES[ID]));
				std::string _expr = "";
				ReplaceDataModel(_expr);
				_action_pair.lstActionParam.push_back(make_pair(_param_node->first_attribute(ATTR_NAMES[NAME])->value(), _expr));
			}
			else
			{
				std::string _expr = _param_node->first_attribute(ATTR_NAMES[EXPR]) == nullptr ? "" : _param_node->first_attribute(ATTR_NAMES[EXPR])->value();
				ReplaceDataModel(_expr);
				_action_pair.lstActionParam.push_back(make_pair(_param_node->first_attribute(ATTR_NAMES[NAME])->value(), _expr));
			}
		}
	}
}

bool CStateMachineImpl::CreateDataModel(const xml_node<>& xNode, DataModelMap& mapDataModel)
{
	vector<int> vectDataID = { DATA_MODEL };
	for (auto const& nDataID : vectDataID)
	{
		const xml_node<>* _data_model_node = xNode.first_node(TAG_NAMES[nDataID]);
		if (_data_model_node == nullptr)
			return true;
		for (xml_node<>* itDataNode = _data_model_node->first_node(TAG_NAMES[DATA]); itDataNode; itDataNode = itDataNode->next_sibling(TAG_NAMES[DATA]))
		{
			const char* _data_name = itDataNode->first_attribute(ATTR_NAMES[ID]) == nullptr ? nullptr : itDataNode->first_attribute(ATTR_NAMES[ID])->value();

			if (_data_name == nullptr)
			{
				LOG_E(TAG, "!!! CStateMachineImpl::CreateDataModel() with data name = null in state \"%s\"", xNode.first_attribute(ATTR_NAMES[ID])->value());
				assert(false);
				return false;
			}
			else if (mapDataModel.count(_data_name) > 0)
			{
				LOG_W(TAG, "!!! CStateMachineImpl::CreateDataModel() - data name:%s has already in mapDataModel", _data_name);
				//assert(false);
				//return false;
				continue;
			}
			else if (itDataNode->first_attribute(ATTR_NAMES[EXPR]) == nullptr)
			{
				LOG_E(TAG, "!!! CStateMachineImpl::CreateDataModel() - data name:%s has null expr", _data_name);
				assert(false);
				return false;
			}
			else
			{
				string _expr = itDataNode->first_attribute(ATTR_NAMES[EXPR]) == nullptr ? "" : itDataNode->first_attribute(ATTR_NAMES[EXPR])->value();
				if (_expr.size() > 2 && _expr[0] == ':' && _expr[1] == ':')
				{
					Read(_expr.substr(2), _expr);
					mapDataModel.insert(std::make_pair(_data_name, _expr));
				}
				else
				{
					mapDataModel.insert(std::make_pair(_data_name, _expr));
				}
			}
		}
	}
	return true;
}

void CStateMachineImpl::ReplaceDataModel(std::string& expr)
{
	for (const auto& data : m_mapDataModel)
	{
		size_t _pos = expr.find(data.first);
		while (_pos != std::string::npos)
		{
			if (_pos > 0 && (std::isalnum(expr[_pos - 1]) || expr[_pos - 1] == '_')
				|| _pos + data.first.size() < expr.size() - 1 && (std::isalnum(expr[_pos + data.first.size()]) || expr[_pos + data.first.size()] == '_'))
			{
				_pos = expr.find(data.first, _pos + data.first.size() + 1);
			}
			else
			{
				//expr = std::regex_replace(expr, std::regex(data.first), data.second);
				expr.replace(expr.begin() + _pos, expr.begin() + _pos + data.first.size(), data.second);
				_pos = expr.find(data.first);;
			}
		}
	}
}

void CStateMachineImpl::SetPriorityEventList(const std::string& event_id, std::vector<IEvent*>& event_arry)
{
	using namespace std;
	size_t _pos = event_id.size();
	while ((_pos = event_id.rfind('.', _pos - 1)) != std::string::npos)
	{
		EventMap::iterator iterItem = m_mapEvents.find(event_id.substr(0, _pos) + ".*");
		if (iterItem != m_mapEvents.end())
			event_arry.push_back(iterItem->second[0]);
	}
	EventMap::iterator iterItem = m_mapEvents.find("*");
	if (iterItem != m_mapEvents.end())
		event_arry.push_back(iterItem->second[0]);
}

const Obj<BioSys::IModel> CStateMachineImpl::model()
{
	return owner_->model();
};

template<typename T>
const char* CStateMachineImpl::GetType(const T& value)
{
	return typeid(value).name();
}

template <typename T>
void CStateMachineImpl::Deserialize(const char* type, const DynaArray& data, T& value)
{
	if (type == "")
		throw std::exception();

	const char* _target_type = GetType(value);

	assert(strcmp(type, _target_type) == 0);

	ByteArray _data(data.size());
	memcpy(_data.data(), data.data(), data.size());
	zpp::serializer::memory_input_archive in(_data);
	in(value);
}

template<typename T>
void CStateMachineImpl::Read(const string& name, T& value)
{
	DynaArray _stored_type;
	DynaArray _buf;
	model()->Read(name, _stored_type, _buf);
	try {
		Deserialize(_stored_type.data(), _buf, value);
	}
	catch (...)
	{
		if (name.substr(0, strlen("Bio.Cell.")) == "Bio.Cell.")
		{
			LOG_D(TAG, "No such key in Model, key name: %s", name.c_str());
		}
		else
		{
			LOG_W(TAG, "No such key in Model, key name: %s", name.c_str());
		}
	}
}
