#include "internal/mtDNA.h"
#include "ThreadPool/header/CThreadPool.hpp"

USING_BIO_NAMESPACE

mtDNA::mtDNA(int max_thread_count)
	:threadpool_(max_thread_count == 0 ? new nThread::CThreadPool() : new nThread::CThreadPool(max_thread_count))
{
}

mtDNA::~mtDNA()
{
	threadpool_->wait_until_all_usable();
	delete threadpool_;
}

void mtDNA::AddTask(Obj<IBiomolecule> callback)
{
	auto Task = [callback]()
	{
		callback->activate();
	};
  	threadpool_->add_and_detach(Task);
	//threadpool_->add(Task);
	threadpool_->join_all();
}