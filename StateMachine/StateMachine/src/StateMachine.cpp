// StateMachine.cpp : Defines the entry point for the DLL application.
//
//  Feature list:
//      1. state: composite state, history, deep history
//      2. event
//      3. guard condition
//      4. transition
//      5. action
//  Not ready feature:
//      1. transient state
//      2. concurrency

#include "internal/IStateMachine.h"
#include "StateMachineImpl.h"
#include <assert.h>

IStateMachine::IStateMachine()
	: m_pStateMachineImpl (new CStateMachineImpl())
{
}

IStateMachine::~IStateMachine()
{
    Destroy();
}

bool IStateMachine::Load(const char* szLogicMap, BioSys::DNA* callback, bool isFile)
{
    if (m_pStateMachineImpl == nullptr)
        return false;
    else
        return m_pStateMachineImpl->Load(szLogicMap, callback, isFile);
}

int IStateMachine::EventFired(const char* szEventID)
{
    if (m_pStateMachineImpl == nullptr)
    {
        return false;
    }
    else
    {
        int iRetval = m_pStateMachineImpl->EventFired(szEventID);
        return iRetval;
    }
}

void IStateMachine::GetActiveStates(const char* lstActiveState[], int iMaxListSize)
{
    if (m_pStateMachineImpl != nullptr)
        m_pStateMachineImpl->GetActiveStates(lstActiveState, iMaxListSize);
}

void IStateMachine::SetActiveStates(const char* lstActiveState[], int iMaxListSize)
{
    if (m_pStateMachineImpl != nullptr)
        m_pStateMachineImpl->SetActiveStates(lstActiveState, iMaxListSize);
}

void IStateMachine::ResetFSM()
{
    if (m_pStateMachineImpl != nullptr)
        m_pStateMachineImpl->ResetFSM();
}

void IStateMachine::Destroy()
{
    if (m_pStateMachineImpl != nullptr)
    {
        delete m_pStateMachineImpl;
        m_pStateMachineImpl = nullptr;
    }
}
