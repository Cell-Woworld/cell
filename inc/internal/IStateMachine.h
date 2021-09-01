#pragma once

class CStateMachineImpl;
namespace BioSys {
	class DNA;
};

class IStateMachine
{
public:
	enum ENUM_FIRE_EVENT_RESULT
	{
		NOTHING_TO_DO = -1,
		RECYCLE_TO_SELF = 0,
		TO_NEXT_STATE = 1,
	};
public:
	IStateMachine();
	virtual ~IStateMachine();
	virtual bool Load(const char* szLogicMapFilename, BioSys::DNA* callback);
	virtual int EventFired(const char* szEventID);
	virtual void GetActiveStates(const char* lstActiveState[], const int iMaxListSize);
	virtual void SetActiveStates(const char* lstActiveState[], const int iMaxListSize);
	virtual void ResetFSM();

private:
    void Destroy();

private:
    CStateMachineImpl* m_pStateMachineImpl;
};