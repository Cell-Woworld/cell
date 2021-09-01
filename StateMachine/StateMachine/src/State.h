#ifndef __STATE_H__
#define __STATE_H__

#pragma warning(disable : 4251 4530)

#include <string>
#include <vector>
#include <deque>
#include "Transition.h"

class CHistory;
class IEvent;
class CDeepHistory;
class IAction;
class ICondition;
class IState;

class CState
{
public:
    CState(const char* szName ="", IState* pCallback=nullptr);
    virtual ~CState();
    void InitialState(CState* s);
    virtual CTransition* AddTrans(CState* s, IEvent* e, std::vector<CTransition::ActionPair> const &a = std::vector<CTransition::ActionPair>(), CTransition::ConditionPair* c = nullptr, bool isNoTarget = false);
    void AddState(CState* s);
    int EventFired(const std::vector<IEvent*>& e);
    int ChangeState(CTransition* pTrans, bool bHistoryPath = false);
    virtual void Enter(bool bHistoryPath = false, bool isNoTarget = false);
    void set_CurrentState(CState* left);
    virtual CState* get_CurrentState() const;
    void set_ParentState(CState* left);
    CState* get_ParentState() const;
    CState* get_StartState() const;
    bool isCompositeState();
    bool hasHistory();
    bool hasDeepHistory();
    virtual CHistory* HistoryEntry();
    virtual CDeepHistory* DeepHistoryEntry();
    void set_ConcurrentState(bool left);
    bool get_ConcurrentState() const;
    void get_ActiveStates(const char* szActiveStatList[], const int iMaxListSize, int& iTotalCount);
    void set_CurrentStateLink();

public:
    const std::string& GetName() {  return m_strName;   };

protected:
    typedef std::deque<CTransition*> TransVector;
    typedef std::vector<CState*> StateVector;
    virtual void OnEnter();
    virtual void OnExit();
    virtual void EnterNotify();
    virtual void ExitNotify();
    void findTrans(IEvent* e, TransVector& transVector );
    bool eval(CTransition::ConditionPair *condition);

protected:
    CState* CurrentState;
    CState* ParentState;
    CState* StartState;
    IAction* m_pStateAction;
    StateVector m_vectState;
    TransVector m_vectTrans;
	std::string m_strName;
	std::string full_path_name_;

protected:
    CHistory* m_pHistory;
    CDeepHistory* m_pDeepHistory;
    bool ConcurrentState;

public:
	void add_invoke_action(std::vector<CTransition::ActionPair>& invoke_list, std::vector<CTransition::ActionPair>& uninvoke_list)
	{
		invoke_actions_.swap(invoke_list);
		uninvoke_actions_.swap(uninvoke_list);
	}
	void add_entry_action(std::vector<CTransition::ActionPair>& param_list)
	{
		onentry_actions_.swap(param_list);
	}
	void add_exit_action(std::vector<CTransition::ActionPair>& param_list)
	{
		onexit_actions_.swap(param_list);
	}
protected:
	std::vector<CTransition::ActionPair> invoke_actions_;
	std::vector<CTransition::ActionPair> uninvoke_actions_;
	std::vector<CTransition::ActionPair> onentry_actions_;
	std::vector<CTransition::ActionPair> onexit_actions_;
};

#endif //__STATE_H__
