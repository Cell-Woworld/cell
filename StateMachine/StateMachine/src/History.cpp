#include "DeepHistory.h"
#include "History.h"
#include <assert.h>

CHistory::CHistory(const char* szName)
    : CState(szName)
{
}

void CHistory::Enter(bool bHistoryPath, bool isNoTarget)
{
    if( CurrentState == nullptr )
        CurrentState = this->get_ParentState()->get_StartState();
    assert(CurrentState != nullptr);
    if (CurrentState == nullptr)
        return;
    //	ChangeState(CurrentState);
    CState *pDestParent = CurrentState->get_ParentState();
    CState *pParent=pDestParent, *pSon=CurrentState;
    while( pParent )
    {
        pParent->set_CurrentState( pSon );
        pSon = pParent;
        pParent = pSon->get_ParentState();
    }
    pDestParent->set_CurrentState( CurrentState );
#ifdef DEBUG_DETAIL
    printf("  Before Entry History Current State   :name = [%s]\n",
        CurrentState->GetName());
#endif
    CurrentState->Enter( bHistoryPath, isNoTarget );
#ifdef DEBUG_DETAIL
    printf("  After Entry History Current State   :name = [%s]\n",
        CurrentState->GetName());
#endif
    /*
    if ( pDestParent->hasHistory() )
    pDestParent->HistoryEntry()->set_CurrentState( pDestParent->get_CurrentState() );
    if ( pDestParent->hasDeepHistory() )
    pDestParent->DeepHistoryEntry()->set_CurrentState( pDestParent->get_CurrentState() );
    */
}

CState* CHistory::get_CurrentState() const
{
	if (CurrentState == nullptr)
		return get_ParentState()->get_StartState();
	else
	{
		return CurrentState;
	}
}