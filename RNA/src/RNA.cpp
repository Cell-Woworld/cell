#include "RNA.h"
#include "RNAImpl.h"

BIO_BEGIN_NAMESPACE
#ifdef __cplusplus
#ifdef STATIC_API
Map<String, BioSys::CREATE_RNA_INSTANCE_FUNCTION> RNA_Map_;
extern "C" PUBLIC_API RNA* RNA_CreateInstance(IBiomolecule* owner, const char* name)
{
	return RNA_Map_[name](owner);
}
#endif
#else
#endif
BIO_END_NAMESPACE

USING_BIO_NAMESPACE
//using namespace google::protobuf;
RNA::RNA(IBiomolecule* owner, const char* name, RNA* callback)
	: impl_(new RNAImpl(owner, name, callback))
{
}

RNA::~RNA()
{
	delete impl_;
}

void RNA::Remove(const String& name, bool internal_only)
{
	impl_->Remove(name, internal_only);
}

void RNA::Clone(const String& target_name, const String& src_name)
{
	impl_->Clone(target_name, src_name);
}

void RNA::SendEvent(const String& name, const String& payload)
{
	impl_->SendEvent(name, payload);
}

String RNA::GetRootPath()
{
	return ((RNAImpl*)impl_)->get_root_path();
}