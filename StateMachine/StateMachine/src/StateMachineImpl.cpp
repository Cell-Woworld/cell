// StateMachineImpl.cpp: implementation of the CStateMachineImpl class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#include "StateMachineImpl.h"
#include "../inc/IXMLParser.h"

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

#include "History.h"
#include "DeepHistory.h"

#include "internal/DNA.h"
#include "internal/IModel.h"
#include "internal/utils/serializer.h"

#pragma warning (disable:4996)

#define TAG "StateMachine"

using namespace std;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
enum ENUM_TAG
{
	RAISE = 0,
	SEND,
	SCRIPT,
	ASSIGN,
	LOG,
	INVOKE,
	UNINVOKE,
	FINAL_STATE,

	STATE,
	HISTORY,
    SHALLOW_HISTORY,
	DEEP_HISTORY,

	ON_ENTRY,
	ON_EXIT,

	PARAM,

    TRANSITION,

	DATA_MODEL,
	DATA,
};

const char* CStateMachineImpl::TAG_NAMES[] =
{
	"raise",
	"send",
	"script",
	"assign",
	"log",
	"invoke",
	"invoke",
	"final",

	"state",
	"history",
    "shallow",
    "deep",

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
	TARGET,
	NAME,
	LOCATION,
	LABEL,
	EXPR,
	AUTOFORWARD,
};

const char* CStateMachineImpl::ATTR_NAMES[] =
{
	"id",
	"initial",
	"type",

	"event",
	"cond",

	"src",
	"target",
	"name",
	"location",
	"label",
	"expr",
	"autoforward",
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

protected:
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

bool CStateMachineImpl::Load(const char* szLogicMapFilename, BioSys::DNA* callback)
{
	owner_ = callback;
    ClearAll();

	// this open and parse the XML file:
	IXMLDomParser objXMLParser;
	objXMLParser.setRemoveClears(false);
	ITCXMLNode xMainNode = objXMLParser.openFileHelper(szLogicMapFilename, "scxml");

    if (xMainNode.isEmpty())
    {
		String _content;
		if (callback->get_content(szLogicMapFilename, _content) == true)
		{
			xMainNode = objXMLParser.parseString(_content.c_str(), "scxml");
		}
		else
		{
			LOG_E(TAG, "CStateMachineImpl::Load() - oXMLParser.Load(%s) - false.", szLogicMapFilename);
			assert(false);
			return false;
		}
    }

    // numbering all states according to XML
	m_mapStates[m_strEntryStateID] = new CState(m_strEntryStateID.c_str(), nullptr);
	if (false == CreateMap(callback, xMainNode, m_mapStates[m_strEntryStateID]))
    {
        LOG_E(TAG, "CStateMachineImpl::Load() - CreateStateMap() - false");
		assert(false);
		return false;
    }

	LOG_D(TAG, "Adding Transitions");
    // Link states with transitions
    if (LinkTransitions(callback, xMainNode) == false)
    {
		LOG_E(TAG, "CStateMachineImpl::Load() - LinkTransitions(oXMLParser) - false");
		assert(false);
		return false;
    }

	LOG_D(TAG, "Entering entry point");

    // power on the engine
    const char* szRootEntryStateID = nullptr;
	szRootEntryStateID = xMainNode.getAttribute(ATTR_NAMES[INITIAL_STATE]);

    if (szRootEntryStateID == nullptr)
    {
		LOG_D(TAG, "CStateMachineImpl::Load() - no entry assigned, apply the first state");
		const ITCXMLNode& _first_child = xMainNode.getChildNode(TAG_NAMES[STATE], 0);
		szRootEntryStateID = _first_child.getAttribute(ATTR_NAMES[ID]);
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
			String _state = String(lstActiveState[i]);
			size_t _pos = _state.find_last_of('/');
			if (_pos != String::npos && _pos < _state.size() - 1)
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

bool CStateMachineImpl::CreateMap(BioSys::DNA* callback, const ITCXMLNode& xNode, CState* pParentNode)
{
	vector<int> vectStateID = { STATE, FINAL_STATE};
	for (auto const &nStateID : vectStateID)
	{
		int nStateCount = xNode.nChildNode(TAG_NAMES[nStateID]);

		if (nStateCount < 0)
		{
			LOG_E(TAG, "!!! CStateMachineImpl::CreateStateMap() - keyCount:%d is illegal.", nStateCount);
			assert(false);
			return false;
		}
		for (int i = 0, itItem = 0; i < nStateCount; ++i)
		{
			ITCXMLNode itStateNode = xNode.getChildNode(TAG_NAMES[nStateID], &itItem);
			const char* _state_name = itStateNode.getAttribute(ATTR_NAMES[ID]);

			if (m_mapStates.count(_state_name) > 0)
			{
				LOG_E(TAG, "!!! CStateMachineImpl::CreateStateMap() - state name:%s has already in m_mapStates", _state_name);
				assert(false);
				return false;
			}

			CreateDataModel(itStateNode, m_mapDataModel);

			CState *pNewState = nullptr;
			if (CheckIfTransientState(itStateNode) == true)
				pNewState = new CTransientState(_state_name);
			else
				pNewState = new CState(_state_name);
			m_mapStates[_state_name] = pNewState;

			LOG_D(TAG, "%s[%s] attached", TAG_NAMES[nStateID], _state_name);
			if (pParentNode != nullptr)
			{
				pParentNode->AddState(m_mapStates[_state_name]);
			}

			if (itStateNode.nChildNode(TAG_NAMES[nStateID]) > 0)
			{
				if (CreateMap(callback, itStateNode, m_mapStates[_state_name]) == false)
				{
					assert(false);
					return false;
				}
			}

			const char* cszInitStateName = nullptr;
			if (itStateNode.getAttribute(ATTR_NAMES[INITIAL_STATE]) != nullptr)
				cszInitStateName = itStateNode.getAttribute(ATTR_NAMES[INITIAL_STATE]);
			else
			{
				const ITCXMLNode& _first_child = itStateNode.getChildNode(TAG_NAMES[STATE], 0);
				cszInitStateName = _first_child.getAttribute(ATTR_NAMES[ID]);
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
			getInvokeList(callback, itStateNode, INVOKE, _invoke_action_pair_list);
			vector<CTransition::ActionPair> _uninvoke_action_pair_list;
			getInvokeList(callback, itStateNode, UNINVOKE, _uninvoke_action_pair_list);
			m_mapStates[_state_name]->add_invoke_action(_invoke_action_pair_list, _uninvoke_action_pair_list);

			bool _onentry = itStateNode.nChildNode(TAG_NAMES[ON_ENTRY]) > 0;
			if (_onentry)
			{
				vector<CTransition::ActionPair> _action_pair_list;
				getActionList(callback, itStateNode.getChildNode(TAG_NAMES[ON_ENTRY]), _action_pair_list);
				m_mapStates[_state_name]->add_entry_action(_action_pair_list);
			}

			bool _onexit = itStateNode.nChildNode(TAG_NAMES[ON_EXIT]) > 0;
			if (_onexit)
			{
				vector<CTransition::ActionPair> _action_pair_list;
				getActionList(callback, itStateNode.getChildNode(TAG_NAMES[ON_EXIT]), _action_pair_list);
				m_mapStates[_state_name]->add_exit_action(_action_pair_list);
			}

			if (nStateID == FINAL_STATE)
			{
				vector<CTransition::ActionPair> _action_pair_list;
				_action_pair_list.push_back(CTransition::ActionPair());
				CTransition::ActionPair& _action_pair = _action_pair_list.back();
				const char* _action_name = itStateNode.getAttribute(ATTR_NAMES[ID]);
				pair<string, int> _action_key = make_pair(_action_name, FINAL_STATE);
				_action_pair.pAction = new Action(callback, _action_name, FINAL_STATE);
				m_mapActions[_action_key] = _action_pair.pAction;
				m_mapStates[_state_name]->add_entry_action(_action_pair_list);
			}

			int nTransitionCount = itStateNode.nChildNode(TAG_NAMES[TRANSITION]);

			for (int j = 0, itItem = 0; j < nTransitionCount; ++j)
			{
				ITCXMLNode itTransitionNode = itStateNode.getChildNode(TAG_NAMES[TRANSITION], &itItem);

				// event
				const char* szEventName = itTransitionNode.getAttribute(ATTR_NAMES[EVENT]);
				if (szEventName != nullptr)
				{
					string strEventName = itTransitionNode.getAttribute(ATTR_NAMES[EVENT]);
					ReplaceDataModel(strEventName);
					if (m_mapEvents.count(strEventName) == 0)
					{
						m_mapEvents[strEventName].push_back(new IEvent(strEventName.c_str()));
					}

					LOG_D(TAG, "%s[%s] attached", ATTR_NAMES[EVENT], strEventName.c_str());
				}
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
		int nHistoryCount = xNode.nChildNode(TAG_NAMES[HISTORY]);
		if (nHistoryCount < 0)
		{
			LOG_E(TAG, "!!! CStateMachineImpl::CreateStateMap() - keyCount:%d is illegal.", nHistoryCount);
			assert(false);
			return false;
		}
		for (int i = 0, itItem = 0; i < nHistoryCount; ++i)
		{
			ITCXMLNode xHistoryNode = xNode.getChildNode(TAG_NAMES[HISTORY], &itItem);
			if (!xHistoryNode.isEmpty())
			{
				const char* szType = xHistoryNode.getAttribute(ATTR_NAMES[TYPE]);
				const char* szName = xHistoryNode.getAttribute(ATTR_NAMES[ID]);
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
	}
	return true;
}

bool CStateMachineImpl::LinkTransitions(BioSys::DNA* callback, const ITCXMLNode& xNode)
{
	vector<int> vectStateID = { STATE, FINAL_STATE };
	for (auto const &nStateID : vectStateID)
	{
		int nStateCount = xNode.nChildNode(TAG_NAMES[nStateID]);

		if (nStateCount < 0)
		{
			LOG_E(TAG, "!!! CStateMachineImpl::CreateStateMap() - keyCount:%d is illegal.", nStateCount);
			assert(false);
			return false;
		}

		for (int i = 0, itItem = 0; i < nStateCount; ++i)
		{
			ITCXMLNode itStateNode = xNode.getChildNode(TAG_NAMES[nStateID], &itItem);
			if (itStateNode.nChildNode(TAG_NAMES[nStateID]) > 0)
			{
				if (LinkTransitions(callback, itStateNode) == false)
				{
					assert(false);
					return false;
				}
			}

			int nTransitionCount = itStateNode.nChildNode(TAG_NAMES[TRANSITION]);

			for (int j = 0, itItem = 0; j < nTransitionCount; ++j)
			{
				ITCXMLNode itTransitionNode = itStateNode.getChildNode(TAG_NAMES[TRANSITION], &itItem);

				CState* pFromState = m_mapStates[itStateNode.getAttribute(ATTR_NAMES[ID])];
				CState* pToState = nullptr;
				bool bIsNoTargetTransition = false;
				if (itTransitionNode.getAttribute(ATTR_NAMES[TARGET]) != nullptr)
				{
					const char* _state_name = itTransitionNode.getAttribute(ATTR_NAMES[TARGET]);
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
					if (pFromState->isCompositeState())
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
				if (itTransitionNode.getAttribute(ATTR_NAMES[EVENT]) != nullptr)
				{
					string strEventName = itTransitionNode.getAttribute(ATTR_NAMES[EVENT]);
					ReplaceDataModel(strEventName);
					pEvent = m_mapEvents[strEventName][0];
				}
				else if (pFromState->GetName().find('*') != string::npos)
				{
					LOG_E(TAG, "!!! Invalid transition: [%s]->[%s] without %s", pFromState->GetName().c_str(), pToState->GetName().c_str(), TAG_NAMES[EVENT]);
					assert(false);
				}

				unique_ptr<CTransition::ConditionPair> pCondition;
				const char* cszConditionName = itTransitionNode.getAttribute(ATTR_NAMES[CONDITION]);
				if (cszConditionName != nullptr)
				{
					string strFullConditionName = cszConditionName;
					string strConditionName = "";

					ReplaceDataModel(strFullConditionName);

					std::size_t found = strFullConditionName.find_first_of("(");
					if (found != string::npos)
					{
						std::size_t _invalid_function_name = strFullConditionName.find_last_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_");
						if (_invalid_function_name == String::npos)
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
				getActionList(callback, itTransitionNode, vectActions);

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

void CStateMachineImpl::getActionList(BioSys::DNA* callback, const ITCXMLNode& root_node, vector<CTransition::ActionPair>& action_pair_list)
{
	int _action_count = root_node.nChildNode();
	for (int i = 0; i < _action_count; i++)
	{
		action_pair_list.push_back(CTransition::ActionPair());
		CTransition::ActionPair& _action_pair = action_pair_list.back();
		const ITCXMLNode& _action_node = root_node.getChildNode(i);
		if (strcmp(_action_node.getName(), TAG_NAMES[RAISE]) == 0)
		{
			const char* _action_name = _action_node.getAttribute(ATTR_NAMES[EVENT]);
			if (_action_name == nullptr)
			{
				LOG_E(TAG, "Action \"%s\" with event name=NULL in transition \"%s\" or state \"%s\"", TAG_NAMES[RAISE], root_node.getAttribute(ATTR_NAMES[EVENT]), root_node.getAttribute(ATTR_NAMES[ID]));
				assert(false);
				return;
			}
			String _actionName = _action_name;
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
		else if (strcmp(_action_node.getName(), TAG_NAMES[SEND]) == 0)
		{
			const char* _action_name = _action_node.getAttribute(ATTR_NAMES[EVENT]);
			if (_action_name == nullptr)
			{
				LOG_E(TAG, "Action \"%s\" with event name=NULL in transition \"%s\" or state \"%s\"", TAG_NAMES[SEND], root_node.getAttribute(ATTR_NAMES[EVENT]), root_node.getAttribute(ATTR_NAMES[ID]));
				assert(false);
				return;
			}
			String _actionName = _action_name;
			ReplaceDataModel(_actionName);
			pair<string, int> _action_key = make_pair(_actionName, SEND);
			if (m_mapActions.count(_action_key) > 0)
				_action_pair.pAction = m_mapActions[_action_key];
			else
			{
				_action_pair.pAction = new Action(callback, _actionName.c_str(), SEND);
				m_mapActions[_action_key] = _action_pair.pAction;
			}
			int _param_count = _action_node.nChildNode(TAG_NAMES[PARAM]);
			for (int j = 0; j < _param_count; j++)
			{
				ITCXMLNode _param_node = _action_node.getChildNode(TAG_NAMES[PARAM], j);
				//if (_param_node.getAttribute(ATTR_NAMES[NAME]) == nullptr || _param_node.getAttribute(ATTR_NAMES[EXPR]) == nullptr)
				if (_param_node.getAttribute(ATTR_NAMES[NAME]) == nullptr)
				{
					LOG_E(TAG, "!!! Invalid parameter. 'Send' field in state \"%s\" is null.", root_node.getParentNode().getAttribute(ATTR_NAMES[ID]));
					action_pair_list.pop_back();
					assert(false);
				}
				else if (_param_node.getAttribute(ATTR_NAMES[EXPR]) == nullptr)
				{
					//LOG_E(TAG, "!!! Invalid value of parameter \"%s\". expr of 'send' field in state \"%s\" is null.", _param_node.getAttribute(ATTR_NAMES[NAME]), root_node.getParentNode().getAttribute(ATTR_NAMES[ID]));
					std::string _expr = "";
					ReplaceDataModel(_expr);
					_action_pair.lstActionParam.push_back(make_pair(_param_node.getAttribute(ATTR_NAMES[NAME]), _expr));
				}
				else
				{
					std::string _expr = _param_node.getAttribute(ATTR_NAMES[EXPR]);
					ReplaceDataModel(_expr);
					_action_pair.lstActionParam.push_back(make_pair(_param_node.getAttribute(ATTR_NAMES[NAME]), _expr));
				}
			}
		}
		else if (strcmp(_action_node.getName(), TAG_NAMES[SCRIPT]) == 0)
		{
			vector<string> action_name_list;
			_Split(action_name_list, _action_node.getText()==nullptr ? "" : _action_node.getText(), ";");
			if (action_name_list.size() == 0)
			{
				LOG_E(TAG, "!!! Invalid script. Script of state \"%s\" is null.", root_node.getParentNode().getAttribute(ATTR_NAMES[ID]));
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
							String _params = _action_full_name.substr(_param_begin, _param_end - _param_begin + 1);
							ReplaceDataModel(_params);
							_action_pair.lstActionParam.push_back(make_pair("params_in_JSON", _params));
						}
					}
				}
			}
		}
		else if (strcmp(_action_node.getName(), TAG_NAMES[ASSIGN]) == 0)
		{
			const char* _action_name = "Assign";
			pair<string, int> _action_key = make_pair(_action_name, ASSIGN);
			if (m_mapActions.count(_action_key) > 0)
				_action_pair.pAction = m_mapActions[_action_key];
			else
			{
				_action_pair.pAction = new Action(callback, _action_name, ASSIGN);
				m_mapActions[_action_key] = _action_pair.pAction;
			}
			//if (_action_node.getAttribute(ATTR_NAMES[LOCATION]) == nullptr || _action_node.getAttribute(ATTR_NAMES[EXPR]) == nullptr)
			if (_action_node.getAttribute(ATTR_NAMES[LOCATION]) == nullptr)
			{
				//LOG_E(TAG, "Assign model with location==null or expression==null!");
				LOG_E(TAG, "Assign model with location==null");
				assert(false);
			}
			else
			{
				std::string _location = _action_node.getAttribute(ATTR_NAMES[LOCATION]);
				ReplaceDataModel(_location);
				std::string _expr = "";
				if (_action_node.getAttribute(ATTR_NAMES[EXPR]) != nullptr)
					_expr = _action_node.getAttribute(ATTR_NAMES[EXPR]);
				ReplaceDataModel(_expr);
				_action_pair.lstActionParam.push_back(make_pair(_location, _expr));
			}
		}
		else if (strcmp(_action_node.getName(), TAG_NAMES[LOG]) == 0)
		{
			const char* _action_name = "Log";
			pair<string, int> _action_key = make_pair(_action_name, LOG);
			if (m_mapActions.count(_action_key) > 0)
				_action_pair.pAction = m_mapActions[_action_key];
			else
			{
				_action_pair.pAction = new Action(callback, _action_name, LOG);
				m_mapActions[_action_key] = _action_pair.pAction;
			}
			std::string _label = _action_node.getAttribute(ATTR_NAMES[LABEL]);
			ReplaceDataModel(_label);
			_action_pair.lstActionParam.push_back(make_pair(ATTR_NAMES[LABEL], _label));
			std::string _expr = _action_node.getAttribute(ATTR_NAMES[EXPR]);
			ReplaceDataModel(_expr);
			_action_pair.lstActionParam.push_back(make_pair(ATTR_NAMES[EXPR], _expr));
		}
		else
		{
			action_pair_list.pop_back();
		}
	}
}

bool CStateMachineImpl::CheckIfTransientState(const ITCXMLNode& root_node)
{
	int nTransitionCount = root_node.nChildNode(TAG_NAMES[TRANSITION]);

	for (int i = 0, itItem = 0; i < nTransitionCount; ++i)
	{
		ITCXMLNode itTransitionNode = root_node.getChildNode(TAG_NAMES[TRANSITION], &itItem);

		// event
		const char* szEventName = itTransitionNode.getAttribute(ATTR_NAMES[EVENT]);
		if (szEventName == nullptr || szEventName[0] == '\0')
			return true;
	}
	return false;
}

void CStateMachineImpl::getInvokeList(BioSys::DNA* callback, const ITCXMLNode& root_node, int type, vector<CTransition::ActionPair>& action_pair_list)
{
	int _invoke_count = root_node.nChildNode(TAG_NAMES[type]);
	if (_invoke_count > 0)
	{
		for (int j = 0; j < _invoke_count; j++)
		{
			action_pair_list.push_back(CTransition::ActionPair());
			CTransition::ActionPair& _action_pair = action_pair_list.back();

			const ITCXMLNode& _invoke = root_node.getChildNode(TAG_NAMES[type], j);
			const char* _id = _invoke.getAttribute(ATTR_NAMES[ID]);
			const char* _src = _invoke.getAttribute(ATTR_NAMES[SRC]);
			const char* _autoforward = _invoke.getAttribute(ATTR_NAMES[AUTOFORWARD]);
			String _src_str;
			if (_src == nullptr)
			{
				if (_id != nullptr)
				{
					LOG_E(TAG, "!!! Invalid src. src of invoke with id=\"%s\" in state \"%s\" is null.", _id, root_node.getAttribute(ATTR_NAMES[ID]));
				}
				else
				{
					LOG_E(TAG, "!!! Invalid src. src of invoke with id=\"unknown\" in state \"%s\" is null.", root_node.getAttribute(ATTR_NAMES[ID]));
				}
				action_pair_list.pop_back();
				assert(false);
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
				LOG_W(TAG, "Invoke in state \"%s\" with the same id \"%s\", is that right?", root_node.getAttribute(ATTR_NAMES[ID]), _id);
				_action_pair.pAction = m_mapActions[_action_key];
			}
			else
			{
				_action_pair.pAction = new Action(callback, _action_name, type);
				m_mapActions[_action_key] = _action_pair.pAction;
			}
			_action_pair.lstActionParam.push_back(make_pair(ATTR_NAMES[ID], _id));
			_action_pair.lstActionParam.push_back(make_pair(ATTR_NAMES[SRC], _src));
			if (_autoforward != nullptr)
				_action_pair.lstActionParam.push_back(make_pair(ATTR_NAMES[AUTOFORWARD], _autoforward));
			else
				_action_pair.lstActionParam.push_back(make_pair(ATTR_NAMES[AUTOFORWARD], "true"));
		}
	}
}

bool CStateMachineImpl::CreateDataModel(const ITCXMLNode& xNode, DataModelMap& mapDataModel)
{
	vector<int> vectDataID = { DATA_MODEL };
	for (auto const& nDataID : vectDataID)
	{
		int nDataModelCount = xNode.nChildNode(TAG_NAMES[nDataID]);

		if (nDataModelCount <= 0)
		{
			return true;
		}

		int nDataCount = xNode.getChildNode(TAG_NAMES[nDataID]).nChildNode(TAG_NAMES[DATA]);
		for (int i = 0; i < nDataCount; ++i)
		{
			ITCXMLNode itDataNode = xNode.getChildNode(TAG_NAMES[nDataID]).getChildNode(TAG_NAMES[DATA], i);
			const char* _data_name = itDataNode.getAttribute(ATTR_NAMES[ID]);

			if (_data_name == nullptr)
			{
				LOG_E(TAG, "!!! CStateMachineImpl::CreateDataModel() with data name = null in state \"%s\"", xNode.getAttribute(ATTR_NAMES[ID]));
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
			else if (itDataNode.getAttribute(ATTR_NAMES[EXPR]) == nullptr)
			{
				LOG_E(TAG, "!!! CStateMachineImpl::CreateDataModel() - data name:%s has null expr", _data_name);
				assert(false);
				return false;
			}
			else
			{
				String _expr = itDataNode.getAttribute(ATTR_NAMES[EXPR]);
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
void CStateMachineImpl::Read(const String& name, T& value)
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
