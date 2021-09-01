#include "State.h"

#include <assert.h>
#include <list>
#include <stack>

#include "IAction.h"
#include "DeepHistory.h"
#include "IEvent.h"
#include "History.h"
#include "Transition.h"
#include "ICondition.h"
#include "IState.h"
#include "internal/IStateMachine.h"

using namespace std;

CState::CState(const char* szName, IState* pCallback)
{
    CurrentState = nullptr;
    StartState = nullptr;
    ParentState = nullptr;
    m_pHistory = nullptr;
    m_pDeepHistory = nullptr;
    ConcurrentState = false;
    m_strName = szName;
}

CState::~CState()
{
    /*
    if (m_pHistory != nullptr)
    {
    delete m_pHistory;
    m_pHistory = nullptr;
    };
    if (m_pDeepHistory != nullptr)
    {
    delete m_pDeepHistory;
    m_pDeepHistory = nullptr;
    };
    */
    while (!m_vectTrans.empty())
    {
        delete m_vectTrans.back();
        m_vectTrans.pop_back();
    }
}

void CState::InitialState(CState* s)
{
    StartState = s;
}

CTransition* CState::AddTrans(CState* s, IEvent* e, std::vector<CTransition::ActionPair> const &a, CTransition::ConditionPair* c, bool isNoTarget)
{
	CTransition* newTransition = nullptr;
    if (c != nullptr)
    {
        if ( (m_vectTrans.size() > 0) && (m_vectTrans.back()->getCondition()->pCondition == nullptr) )
        {
            TransVector::iterator itTrans = m_vectTrans.end();
            itTrans--;
            while ( (itTrans != m_vectTrans.begin()) && ((*itTrans)->getCondition()->pCondition == nullptr) )
            {
                itTrans--;
            }

            if ((*itTrans)->getCondition()->pCondition != nullptr)
            {
                itTrans++;
            }
			newTransition = new CTransition( s, e, a, c, isNoTarget);
            m_vectTrans.insert( itTrans, newTransition);
        }
        else
        {
			newTransition = new CTransition( s, e, a, c, isNoTarget);
            m_vectTrans.push_back( newTransition );
        }
    }
    else
    {
		newTransition = new CTransition( s, e, a, c, isNoTarget);
        m_vectTrans.push_back( newTransition );
    }
	return newTransition;
}

void CState::AddState(CState* s)
{
    m_vectState.push_back( s );
    s->set_ParentState( this );
}

int CState::EventFired(const std::vector<IEvent*>& e)
{
    CState* pEventFiredState = this;
    while ( pEventFiredState->isCompositeState() )
    {
        //assert ( pEventFiredState->CurrentState != nullptr );
        if ( pEventFiredState->get_ConcurrentState() == true)
        {
            for (unsigned int i=0; i<pEventFiredState->m_vectState.size(); i++)
                m_vectState[i]->EventFired(e);
            return IStateMachine::TO_NEXT_STATE;
        }
        else if ( pEventFiredState->CurrentState != nullptr )
            pEventFiredState = pEventFiredState->CurrentState;
        else
        {
            return IStateMachine::NOTHING_TO_DO;
        }
    }
    CState* pCurrentState = pEventFiredState;
    while (pEventFiredState!=nullptr)
    {
        TransVector v;
        int i = 0;
        while (v.empty() && i < e.size())
        { 
            pEventFiredState->findTrans( e[i], v );
            i++;
        }
		int nIndex = 0;
        TransVector::iterator pos = v.begin();
        for( ; pos != v.end(); ++pos, nIndex++ )
        {
			CTransition::ConditionPair *c = (*pos)->getCondition();
			if (eval(c))
            {
                int iRetval = pCurrentState->ChangeState( *pos );
                return iRetval;
            }
        }
        pEventFiredState = pEventFiredState->get_ParentState();
    }
    return IStateMachine::NOTHING_TO_DO;
}

int CState::ChangeState(CTransition* pTrans, bool bHistoryPath)
{
    CState* s = pTrans->getState();
    const vector<CTransition::ActionPair> a = pTrans->getAction();
    if (s == this)
    {
		if (!pTrans->isNoTarget())
			ExitNotify();
        for (int i=0; i<a.size(); i++)
        {
            if (a[i].pAction != nullptr)
			    a[i].pAction->execute(a[i].lstActionParam);
            else
                printf("StateMachine::ChangeState() no such action when event \"%s\" fired\n", pTrans->getEvent()->GetName());
        }
		if (!pTrans->isNoTarget())
			EnterNotify();
		if (get_ParentState()->get_CurrentState() == this)
			return IStateMachine::RECYCLE_TO_SELF;
		else
			return IStateMachine::TO_NEXT_STATE;
    }
    else if( s->get_ParentState() == this)
    {
        // the destination state is one of the sub-state
        for (int i = 0; i<a.size(); i++)
		{
            if (a[i].pAction != nullptr)
			    a[i].pAction->execute(a[i].lstActionParam);
            else
                printf("StateMachine::ChangeState() no such action when event \"%s\" fired\n", pTrans->getEvent()->GetName());
        }
        set_CurrentState( s );
        s->Enter(bHistoryPath);

        if (m_pHistory != nullptr)
            m_pHistory->set_CurrentState( CurrentState );
        if (m_pDeepHistory != nullptr)
            m_pDeepHistory->set_CurrentState( CurrentState );
    }
    else
    {
        if (pTrans->get_Ancestor()!=nullptr)
        {   // root state of transition has been traversed before
            CState *r = this;
            while ( r != pTrans->get_Ancestor() )
            {
				if (!pTrans->isNoTarget())
					r->ExitNotify();
                r = r->get_ParentState();
            }

			for (int i = 0; i<a.size(); i++)   //must between OnExit() and OnEnter()
            {
                if (a[i].pAction != nullptr)
                    a[i].pAction->execute(a[i].lstActionParam);
                else
                    printf("StateMachine::ChangeState() no such action when event \"%s\" fired\n", pTrans->getEvent()->GetName());
            }

            r = s->get_ParentState();
            CState *pNext = s;
            while ( r != pTrans->get_Ancestor() )
            {
                if (r->get_ConcurrentState()==false)
                {
                    r->set_CurrentState( pNext );
                    if (pNext != s) // it will be assigned after Enter() to avoid infinite recursion
                    {
                        if (r->hasHistory())
                            r->HistoryEntry()->set_CurrentState( r->get_CurrentState() );
                        if (r->hasDeepHistory())
                            r->DeepHistoryEntry()->set_CurrentState( r->get_CurrentState() );
                    }
                }
                pNext = r;
                r = r->get_ParentState();
            }
            if (r->get_ConcurrentState()==false)
			{
                r->set_CurrentState( pNext );
                if (pNext != s) // it will be assigned after Enter() to avoid infinite recursion
                {
                    if (r->hasHistory())
                        r->HistoryEntry()->set_CurrentState( r->get_CurrentState() );
                    if (r->hasDeepHistory())
                        r->DeepHistoryEntry()->set_CurrentState( r->get_CurrentState() );
                }
			}
            r = pNext;
            while ( r!=s )
            {
				if (!pTrans->isNoTarget())
					r->EnterNotify();
                r = r->get_CurrentState();
            }
            //r->EnterNotify();
        }
        else
        {
            list<CState*> oriStates;
            list<CState*> destStates;
            CState *r = s->get_ParentState();
            while( r )
            {
                destStates.push_front(r);
                r = r->get_ParentState();
            }
            r = this;

            while( r )
            {
                oriStates.push_front(r);
                r = r->get_ParentState();
            }

            // find the root state

            while(  (oriStates.size()>0)&&(destStates.size()>0)
                &&(oriStates.front() == destStates.front()) )
            {
                r = oriStates.front();
                oriStates.pop_front();
                destStates.pop_front();
            }

            pTrans->set_Ancestor(r);

            list<CState*>::reverse_iterator pos;
            for( pos = oriStates.rbegin(); pos != oriStates.rend(); ++pos )
            {
				if (!pTrans->isNoTarget())
	                (*pos)->ExitNotify();
            }

			for (int i = 0; i<a.size(); i++)   //must between OnExit() and OnEnter()
            {
                if (a[i].pAction != nullptr)
                    a[i].pAction->execute(a[i].lstActionParam);
                else
                    printf("StateMachine::ChangeState() no such action when event \"%s\" fired\n", pTrans->getEvent()->GetName());
            }
            CState *next = 0;
            while( destStates.size() > 0 )
            {
                next = destStates.front();
                destStates.pop_front();

                if (r->get_ConcurrentState()==false)
                {
                    r->set_CurrentState( next );
                    if (r->hasHistory())
                        r->HistoryEntry()->set_CurrentState( r->get_CurrentState() );
                    if (r->hasDeepHistory())
                        r->DeepHistoryEntry()->set_CurrentState( r->get_CurrentState() );
                }
				if (!pTrans->isNoTarget())
					next->EnterNotify();
                r = next;
            };
            //r->ChangeState( s, bHistoryPath);
            if (r)
            {
                if (r->get_ConcurrentState()==false)
                    r->set_CurrentState( s );
            }
        }
        CState *pDestParent = s->get_ParentState();
		s->Enter(bHistoryPath, pTrans->isNoTarget());
        if (pDestParent != nullptr)
        {
            if (pDestParent->hasHistory())
                pDestParent->HistoryEntry()->set_CurrentState( pDestParent->get_CurrentState() );
            if (pDestParent->hasDeepHistory())
                pDestParent->DeepHistoryEntry()->set_CurrentState( pDestParent->get_CurrentState() );
        }
    }
	if (s->get_CurrentState() == this)
		return IStateMachine::RECYCLE_TO_SELF;
	else
		return IStateMachine::TO_NEXT_STATE;
	//return IStateMachine::TO_NEXT_STATE;
}

void CState::Enter(bool bHistoryPath, bool isNoTarget)
{
	if (!isNoTarget)
		EnterNotify();
    if (bHistoryPath==false)
    {
        if ( StartState )
        {
			set_CurrentState(StartState);
			if (m_pHistory != nullptr)
				m_pHistory->set_CurrentState(CurrentState);
			if (m_pDeepHistory != nullptr)
				m_pDeepHistory->set_CurrentState(CurrentState);
			StartState->Enter(bHistoryPath, isNoTarget);
        }
        else
        {
            if (  (isCompositeState()) && (ConcurrentState)  )
            {
				for (unsigned int i = 0; i < m_vectState.size(); i++)
				{
					//if (!isNoTarget)
						m_vectState[i]->Enter(bHistoryPath, isNoTarget);
				}
            }
        }
    }
    else
    {
        if( CurrentState )
        {
			//set_CurrentState(CurrentState);
			if (m_pHistory != nullptr)
				m_pHistory->set_CurrentState(CurrentState);
			if (m_pDeepHistory != nullptr)
				m_pDeepHistory->set_CurrentState(CurrentState);
			CurrentState->Enter(bHistoryPath, isNoTarget);
		}
        else if ( StartState )
        {
			set_CurrentState(StartState);
			if (m_pHistory != nullptr)
				m_pHistory->set_CurrentState(StartState);
			if (m_pDeepHistory != nullptr)
				m_pDeepHistory->set_CurrentState(StartState);
			StartState->Enter(bHistoryPath, isNoTarget);
		}
		else
        {
            if (  (isCompositeState()) && (ConcurrentState)  )
            {
				for (unsigned int i = 0; i < m_vectState.size(); i++)
				{
					//if (!isNoTarget)
						m_vectState[i]->Enter(bHistoryPath, isNoTarget);
				}
			}
        }
    }
}

void CState::set_CurrentState(CState* left)
{
    CurrentState = left;
}

CState* CState::get_CurrentState() const
{
    return CurrentState;
}

void CState::set_ParentState(CState* left)
{
    ParentState = left;
}

CState* CState::get_ParentState() const
{
    return ParentState;
}

CState* CState::get_StartState() const
{
    return StartState;
}

bool CState::isCompositeState()
{
    return m_vectState.size() > 0;
}

CHistory* CState::HistoryEntry()
{
    if (m_pHistory==nullptr)
    {
#ifdef _DEBUG
        //char szName[255];
        //strcpy(szName, m_strName.c_str());
        //strcat_s(szName, "_Histroy";
        //m_pHistory=new CHistory(szName);
        string strHistoryName = m_strName;
		strHistoryName += string("_History");
        m_pHistory=new CHistory(strHistoryName.c_str());
#else
        m_pHistory=new CHistory();
#endif
        AddState(m_pHistory);
    }
    return m_pHistory;
}

CDeepHistory* CState::DeepHistoryEntry()
{
    if (m_pDeepHistory==nullptr)
    {
#ifdef _DEBUG
        //char szName[255];
        //strcpy(szName, m_szName);
        //strcat_s(szName, "_DeepHistory";
        string strHistoryName = m_strName;
		strHistoryName += string("_DeepHistory");
        m_pDeepHistory=new CDeepHistory(strHistoryName.c_str());
#else
        m_pDeepHistory=new CDeepHistory();
#endif
        AddState(m_pDeepHistory);
    }
    return m_pDeepHistory;
}

bool CState::hasHistory()
{
    return (m_pHistory!=nullptr);
}

bool CState::hasDeepHistory()
{
    return (m_pDeepHistory!=nullptr);
}

void CState::OnEnter()
{
#ifdef _DEBUG
    printf("CState::OnEnter() in State[%s]\n", m_strName.c_str());
#endif
	for (auto _action : invoke_actions_)
		_action.pAction->execute(_action.lstActionParam);

	for (auto _action : onentry_actions_)
		_action.pAction->execute(_action.lstActionParam);
}

void CState::OnExit()
{
#ifdef _DEBUG
    printf("CState::OnExit() from State[%s]\n", m_strName.c_str());
#endif
	for (auto _action : onexit_actions_)
		_action.pAction->execute(_action.lstActionParam);

	for (auto _action : uninvoke_actions_)
		_action.pAction->execute(_action.lstActionParam);
}

void CState::EnterNotify()
{
	OnEnter();
}

void CState::ExitNotify()
{
	OnExit();
}

void CState::findTrans(IEvent* e, TransVector& transVector )
{
    TransVector::iterator pos = m_vectTrans.begin( );
    for ( ; pos != m_vectTrans.end( ); ++pos )
    {
        if ( (*pos)->getEvent() == e )
        {
            transVector.push_back(*pos);
        }
    }
}
void CState::set_ConcurrentState(bool left)
{
    ConcurrentState = left;
};

bool CState::get_ConcurrentState() const
{
    return ConcurrentState;
};

void CState::get_ActiveStates(const char* szActiveStatList[], const int iMaxListSize, int& iTotalCount)
{
    if (iTotalCount >= iMaxListSize)
        return;

    CState* pCurrentState = this;
    while ( pCurrentState->isCompositeState() )
    {
        //assert ( pEventFiredState->CurrentState != nullptr );
        if ( pCurrentState->get_ConcurrentState() == true)
        {
            for (unsigned int i=0; i<pCurrentState->m_vectState.size(); i++)
                m_vectState[i]->get_ActiveStates(szActiveStatList, iMaxListSize, iTotalCount);
            return;
        }
        else if ( pCurrentState->CurrentState != nullptr )
            pCurrentState = pCurrentState->CurrentState;
        else
        {
            return;
        }
    }

	std::stack<CState*> _state_stack;
	CState* _working_state = pCurrentState;
	while (_working_state != nullptr)
	{
		_state_stack.push(_working_state);
		_working_state = _working_state->get_ParentState();
	}
	full_path_name_ = "";
	_state_stack.pop();			// ignore "RootState"
	while (!_state_stack.empty())	
	{
		full_path_name_ += _state_stack.top()->GetName() + "/";
		_state_stack.pop();
	}
	if (full_path_name_.size() > 0)
		full_path_name_.back() = '\0';
	szActiveStatList[iTotalCount] = full_path_name_.c_str();
    iTotalCount++;
};

void CState::set_CurrentStateLink()
{
    CState* pCurrentState = this;
    while (pCurrentState->get_ParentState() != nullptr)
    {
        CState* pParentState = pCurrentState->get_ParentState();
        if (pParentState->get_ConcurrentState() == false)
            pParentState->set_CurrentState(pCurrentState);
        if (pParentState->m_pHistory != nullptr)
            pParentState->m_pHistory->set_CurrentState(pCurrentState);
        if (pParentState->m_pDeepHistory != nullptr)
            pParentState->m_pDeepHistory->set_CurrentState(this);
        pCurrentState = pParentState;
    }
}

bool CState::eval(CTransition::ConditionPair *condition )
{
    if ((condition) && (condition->pCondition))
    {
        return condition->pCondition->eval(condition->lstConditionParam);
    }
    else
    {
        return true;
    }
}
