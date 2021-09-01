#pragma once
#ifdef __cplusplus
BIO_BEGIN_NAMESPACE
	class IBiomolecule;
	extern "C" IBiomolecule* Cell_CreateInstance(const char* name);
	typedef IBiomolecule* (*CREATE_CELL_INSTANCE_FUNCTION)(const char*);
#ifdef STATIC_API
	class RNA;
	typedef RNA* (*CREATE_RNA_INSTANCE_FUNCTION)(BioSys::IBiomolecule* owner);
	extern Map<String, CREATE_RNA_INSTANCE_FUNCTION> RNA_Map_;
#endif
BIO_END_NAMESPACE
#else
#endif