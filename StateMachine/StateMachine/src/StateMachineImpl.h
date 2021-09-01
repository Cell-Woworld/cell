// StateMachineImpl.h: interface for the CStateMachineImpl class.
//
//////////////////////////////////////////////////////////////////////

#ifndef __STATEMACHINEIMPL_H__
#define __STATEMACHINEIMPL_H__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#pragma warning (disable:4786)

#include <string>
#include <map>
#include <vector>
#include "Transition.h"
#include "type_def.h"

class ITCXMLNode;
class IState;
class CState;
class IEvent;
class ICondition;
class IAction;
class IMessageExchange;
class CStateMachineImpl;
namespace BioSys {
	class DNA;
    class IModel;
};

class CStateMachineImpl
{
    static const char* TAG_NAMES[];
	static const char* ATTR_NAMES[];

private:
    typedef std::map<std::string, CState*> StateMap;
    typedef std::map<std::string, std::vector<IEvent*>> EventMap;
    typedef std::map<std::string, ICondition*> ConditionMap;
    typedef std::map<std::pair<std::string, int>, IAction*> ActionMap;
    typedef std::map<std::string, std::string> DataModelMap;

public:
    CStateMachineImpl();
    virtual ~CStateMachineImpl();

public:
    bool Load(const char* szLogicMapFilename, BioSys::DNA* callback);
    int EventFired(const char* szEventID);
    void GetActiveStates(const char* lstActiveState[], const int iMaxListSize);
    void SetActiveStates(const char* lstActiveState[], const int iMaxListSize);
	void ResetFSM();

private:
    void ClearAll();

    template< typename _Map >
    void DeleteValuePtrAndClearMap(_Map &Map);
    template< typename _Map >
    void DeletePtrInArrayAndClearMap(_Map &Map);

    bool CreateMap(BioSys::DNA* callback, const ITCXMLNode& xNode, CState* pParentNode=nullptr);

    bool LinkTransitions(BioSys::DNA* callback, const ITCXMLNode& xNode);

	void _Split(std::vector<std::string>& vectOutput, const std::string& strSrc, const std::string& separator);

	void getActionList(BioSys::DNA* callback, const ITCXMLNode& root_node, std::vector<CTransition::ActionPair>& action_pair_list);
	void getInvokeList(BioSys::DNA* callback, const ITCXMLNode& root_node, int type, std::vector<CTransition::ActionPair>& action_pair_list);
	bool CheckIfTransientState(const ITCXMLNode& root_node);

    bool CreateDataModel(const ITCXMLNode& xNode, DataModelMap& mapDataModel);
    void ReplaceDataModel(std::string& expr);

    void SetPriorityEventList(const std::string& event_id, std::vector<IEvent*>& event_arry);

    virtual const Obj<BioSys::IModel> model();
    template<typename T>
    const char* GetType(const T& value);
    template<typename T>
    void Deserialize(const char* type, const DynaArray& data, T& value);
    template<typename T>
    void Read(const String& name, T& value);

private:
	std::string m_strEntryStateID;
    StateMap m_mapStates;
    StateMap m_mapHistory;          // for search only
    EventMap m_mapEvents;
    ConditionMap m_mapConditions;
    ActionMap m_mapActions;
    DataModelMap m_mapDataModel;
    BioSys::DNA* owner_;
};

#endif	// __STATEMACHINEIMPL_H__
