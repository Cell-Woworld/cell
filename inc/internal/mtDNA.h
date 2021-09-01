#pragma once
#include "type_def.h"
#include "IBiomolecule.h"

namespace nThread {
	class CThreadPool;
};

BIO_BEGIN_NAMESPACE

class mtDNA
{
public:
	PUBLIC_API mtDNA(int max_thread_count = 0);
	PUBLIC_API virtual ~mtDNA();
public:
	virtual void AddTask(Obj<IBiomolecule> callback);

private:
	nThread::CThreadPool* threadpool_;
};

BIO_END_NAMESPACE