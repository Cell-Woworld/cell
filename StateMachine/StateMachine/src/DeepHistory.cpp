#include "History.h"
#include "DeepHistory.h"

CDeepHistory::CDeepHistory(const char* szName)
    : CState(szName)
{
}

void CDeepHistory::Enter(bool bHistoryPath, bool isNoTarget)
{
    if( CurrentState == nullptr )
        CurrentState = this->get_ParentState()->get_StartState();
    //	ChangeState(CurrentState, true);
    CState *pDestParent = CurrentState->get_ParentState();
    CState *pParent=pDestParent, *pSon=CurrentState;
    while( pParent )
    {
        pParent->set_CurrentState( pSon );
        pSon = pParent;
        pParent = pSon->get_ParentState();
    }
    pDestParent->set_CurrentState( CurrentState );
    CurrentState->Enter( true , isNoTarget);
    /*
    if ( pDestParent->hasHistory() )
    pDestParent->HistoryEntry()->set_CurrentState( pDestParent->get_CurrentState() );
    if ( pDestParent->hasDeepHistory() )
    pDestParent->DeepHistoryEntry()->set_CurrentState( pDestParent->get_CurrentState() );
    */
}

CState* CDeepHistory::get_CurrentState() const
{
	if (CurrentState == nullptr)
		return get_ParentState()->get_StartState();
	else
	{
		CState* pDeepCurState = CurrentState;
		while (pDeepCurState->get_CurrentState() != nullptr)
			pDeepCurState = pDeepCurState->get_CurrentState();
		return pDeepCurState;
	}
}
