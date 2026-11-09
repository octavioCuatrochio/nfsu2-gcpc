// FEng pool blocks: constructors / destructors of the two instantiations. Original: zFe2.cpp (GC: templates
// FEPoolNode<T,N>, inlined); PC: 53E710/53E910 (ctors), 53EB20/53EC20 (scalar deleting dtors, block vtable slot 0).
#include "FEPool.h"

namespace {
void MsgRespCtor(void *p) { FEMessageResponse::Construct(p); }
void MsgRespDtor(void *p) { static_cast<FEMessageResponse *>(p)->Destruct(); }
void ScriptCtor(void *p) { FEScript::Construct(p); }
void ScriptDtor(void *p) { static_cast<FEScript *>(p)->Destruct(); }
}  // namespace

void FEMessageResponseBlock_Construct(void *mem)
{
    static_cast<FEMessageResponseBlock *>(mem)->Construct(bridge::FEMessageResponseBlockVTable(), MsgRespCtor);
}

void FEMessageResponseBlock_Destruct(FEMessageResponseBlock *b)
{
    b->Destruct(bridge::FEMessageResponseBlockVTable(), MsgRespDtor);
}

// Blocks come from plain operator new (bridge::Alloc), so bit 0 frees with operator delete (PC 575630).
FEMessageResponseBlock *FEMessageResponseBlock_ScalarDtor(FEMessageResponseBlock *b, uint32_t flags)
{
    FEMessageResponseBlock_Destruct(b);
    if (flags & 1) bridge::FreeArray(b);
    return b;
}

void FEScriptBlock_Construct(void *mem)
{
    static_cast<FEScriptBlock *>(mem)->Construct(bridge::FEScriptBlockVTable(), ScriptCtor);
}

void FEScriptBlock_Destruct(FEScriptBlock *b)
{
    b->Destruct(bridge::FEScriptBlockVTable(), ScriptDtor);
}

FEScriptBlock *FEScriptBlock_ScalarDtor(FEScriptBlock *b, uint32_t flags)
{
    FEScriptBlock_Destruct(b);
    if (flags & 1) bridge::FreeArray(b);
    return b;
}
