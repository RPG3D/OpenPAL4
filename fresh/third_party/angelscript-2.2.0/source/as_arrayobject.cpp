/*
   AngelCode Scripting Library
   Copyright (c) 2003-2005 Andreas Jönsson

   This software is provided 'as-is', without any express or implied 
   warranty. In no event will the authors be held liable for any 
   damages arising from the use of this software.

   Permission is granted to anyone to use this software for any 
   purpose, including commercial applications, and to alter it and 
   redistribute it freely, subject to the following restrictions:

   1. The origin of this software must not be misrepresented; you 
      must not claim that you wrote the original software. If you use
	  this software in a product, an acknowledgment in the product 
	  documentation would be appreciated but is not required.

   2. Altered source versions must be plainly marked as such, and 
      must not be misrepresented as being the original software.

   3. This notice may not be removed or altered from any source 
      distribution.

   The original version of this library can be located at:
   http://www.angelcode.com/angelscript/

   Andreas Jönsson
   andreas@angelcode.com
*/


#include <assert.h>
#include <new>
#include <malloc.h>

#include "as_config.h"

#include "as_arrayobject.h"
#include "as_scriptengine.h"
#include "as_texts.h"
#include "as_scriptstruct.h"

struct sArrayBuffer
{
	asDWORD numElements;
	asBYTE  data[1];
};

void ArrayObjectConstructor(int ot, asCArrayObject *self)
{
	new(self) asCArrayObject(0, ot);
}

static void ArrayObjectConstructor_Generic(asIScriptGeneric *gen)
{
	int ot = gen->GetArgDWord(0);
	asCArrayObject *obj = (asCArrayObject*)gen->GetObject();

	ArrayObjectConstructor(ot, obj);
}

static void ArrayObjectConstructor2(asUINT length, int ot, asCArrayObject *self)
{
	new(self) asCArrayObject(length, ot);
}

static void ArrayObjectConstructor2_Generic(asIScriptGeneric *gen)
{
	asUINT length = gen->GetArgDWord(0);
	int ot = gen->GetArgDWord(1);
	asCArrayObject *obj = (asCArrayObject*)gen->GetObject();

	ArrayObjectConstructor2(length, ot, obj);
}

static asCArrayObject &ArrayObjectAssignment(asCArrayObject *other, asCArrayObject *self)
{
	return *self = *other;
}

static void ArrayObjectAssignment_Generic(asIScriptGeneric *gen)
{
	asCArrayObject *other = (asCArrayObject*)gen->GetArgObject(0);
	asCArrayObject *self = (asCArrayObject*)gen->GetObject();

	*self = *other;

	gen->SetReturnObject(self);
}

static void *ArrayObjectAt(asUINT index, asCArrayObject *self)
{
	return self->at(index);
}

static void ArrayObjectAt_Generic(asIScriptGeneric *gen)
{
	asUINT index = gen->GetArgDWord(0);
	asCArrayObject *self = (asCArrayObject*)gen->GetObject();

	gen->SetReturnDWord((asDWORD)self->at(index));
}

static asUINT ArrayObjectLength(asCArrayObject *self)
{
	return self->length();
}

static void ArrayObjectLength_Generic(asIScriptGeneric *gen)
{
	asCArrayObject *self = (asCArrayObject*)gen->GetObject();

	gen->SetReturnDWord(self->length());
}

static void ArrayObjectResize(asUINT size, asCArrayObject *self)
{
	self->resize(size);
}

static void ArrayObjectResize_Generic(asIScriptGeneric *gen)
{
	asUINT size = gen->GetArgDWord(0);
	asCArrayObject *self = (asCArrayObject*)gen->GetObject();

	self->resize(size);
}


void RegisterArrayObject(asCScriptEngine *engine)
{
	int r;
	r = engine->RegisterSpecialObjectType(asDEFAULT_ARRAY, sizeof(asCArrayObject), asOBJ_CLASS_CDA | asOBJ_SCRIPT_ARRAY); assert( r >= 0 );
#ifndef AS_MAX_PORTABILITY
	r = engine->RegisterSpecialObjectBehaviour(engine->defaultArrayObjectType, asBEHAVE_CONSTRUCT, "void f(int)", asFUNCTIONPR(ArrayObjectConstructor, (int, asCArrayObject*), void), asCALL_CDECL_OBJLAST); assert( r >= 0 );
	r = engine->RegisterSpecialObjectBehaviour(engine->defaultArrayObjectType, asBEHAVE_CONSTRUCT, "void f(uint, int)", asFUNCTIONPR(ArrayObjectConstructor2, (asUINT, int, asCArrayObject*), void), asCALL_CDECL_OBJLAST); assert( r >= 0 );
	r = engine->RegisterSpecialObjectBehaviour(engine->defaultArrayObjectType, asBEHAVE_ADDREF, "void f()", asFUNCTION(GCObject_AddRef), asCALL_CDECL_OBJLAST); assert( r >= 0 );
	r = engine->RegisterSpecialObjectBehaviour(engine->defaultArrayObjectType, asBEHAVE_RELEASE, "void f()", asFUNCTION(GCObject_Release), asCALL_CDECL_OBJLAST); assert( r >= 0 );
	r = engine->RegisterSpecialObjectBehaviour(engine->defaultArrayObjectType, asBEHAVE_ASSIGNMENT, "void[] &f(void[]&in)", asFUNCTION(ArrayObjectAssignment), asCALL_CDECL_OBJLAST); assert( r >= 0 );
	r = engine->RegisterSpecialObjectBehaviour(engine->defaultArrayObjectType, asBEHAVE_INDEX, "int f(uint)", asFUNCTION(ArrayObjectAt), asCALL_CDECL_OBJLAST); assert( r >= 0 );
	r = engine->RegisterSpecialObjectBehaviour(engine->defaultArrayObjectType, asBEHAVE_INDEX, "int f(uint) const", asFUNCTION(ArrayObjectAt), asCALL_CDECL_OBJLAST); assert( r >= 0 );
	r = engine->RegisterSpecialObjectMethod(asDEFAULT_ARRAY, "uint length() const", asFUNCTION(ArrayObjectLength), asCALL_CDECL_OBJLAST); assert( r >= 0 );
	r = engine->RegisterSpecialObjectMethod(asDEFAULT_ARRAY, "void resize(uint)", asFUNCTION(ArrayObjectResize), asCALL_CDECL_OBJLAST); assert( r >= 0 );
#else
	r = engine->RegisterSpecialObjectBehaviour(engine->defaultArrayObjectType, asBEHAVE_CONSTRUCT, "void f(int)", asFUNCTION(ArrayObjectConstructor_Generic), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterSpecialObjectBehaviour(engine->defaultArrayObjectType, asBEHAVE_CONSTRUCT, "void f(uint, int)", asFUNCTION(ArrayObjectConstructor2_Generic), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterSpecialObjectBehaviour(engine->defaultArrayObjectType, asBEHAVE_ADDREF, "void f()", asFUNCTION(GCObject_AddRef_Generic), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterSpecialObjectBehaviour(engine->defaultArrayObjectType, asBEHAVE_RELEASE, "void f()", asFUNCTION(GCObject_Release_Generic), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterSpecialObjectBehaviour(engine->defaultArrayObjectType, asBEHAVE_ASSIGNMENT, "void[] &f(void[]&in)", asFUNCTION(ArrayObjectAssignment_Generic), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterSpecialObjectBehaviour(engine->defaultArrayObjectType, asBEHAVE_INDEX, "int f(uint)", asFUNCTION(ArrayObjectAt_Generic), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterSpecialObjectBehaviour(engine->defaultArrayObjectType, asBEHAVE_INDEX, "int f(uint) const", asFUNCTION(ArrayObjectAt_Generic), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterSpecialObjectMethod(asDEFAULT_ARRAY, "uint length() const", asFUNCTION(ArrayObjectLength_Generic), asCALL_GENERIC); assert( r >= 0 );
	r = engine->RegisterSpecialObjectMethod(asDEFAULT_ARRAY, "void resize(uint)", asFUNCTION(ArrayObjectResize_Generic), asCALL_GENERIC); assert( r >= 0 );
#endif 
}

asCArrayObject &asCArrayObject::operator=(asCArrayObject &other)
{
	if( buffer )
	{
		DeleteBuffer(buffer);
		buffer = 0;
	}

	// Copy all elements from the other array
	CreateBuffer(&buffer, other.buffer->numElements);
	CopyBuffer(buffer, other.buffer);	

	return *this;
}

asCArrayObject::asCArrayObject(asUINT length, int ot) : asCGCObject()
{
	asIScriptContext *ctx = asGetActiveContext();
	engine = (asCScriptEngine *)ctx->GetEngine();

	Init(engine->allObjectTypes[ot]);

	// Determine element size
	if( objType->subType )
	{
		elementSize = 4;
	}
	else
	{
		if( objType->tokenType == ttDouble )
			elementSize = 8;
		else if( objType->tokenType == ttInt || objType->tokenType == ttUInt ||
			     objType->tokenType == ttBits || objType->tokenType == ttFloat )
			elementSize = 4;
		else if( objType->tokenType == ttInt16 || objType->tokenType == ttUInt16 ||
				 objType->tokenType == ttBits16 )
			elementSize = 2;
		else
			elementSize = 1;
	}

	arrayType = objType->arrayType;

	CreateBuffer(&buffer, length);
}

asCArrayObject::~asCArrayObject()
{
	if( buffer )
	{
		DeleteBuffer(buffer);
		buffer = 0;
	}

	// The GCObject's destructor will be called after this
}

asUINT asCArrayObject::length()
{
	return buffer->numElements;
}

void asCArrayObject::resize(asUINT numElements)
{
	sArrayBuffer *newBuffer;
	if( objType->subType )
	{
		// Allocate memory for the buffer
		newBuffer = (sArrayBuffer*)malloc(sizeof(sArrayBuffer)-1+sizeof(void*)*numElements);
		newBuffer->numElements = numElements;

		// Copy the elements from the old buffer
		int c = numElements > buffer->numElements ? buffer->numElements : numElements;
		asDWORD **d = (asDWORD**)newBuffer->data;
		asDWORD **s = (asDWORD**)buffer->data;
		for( int n = 0; n < c; n++ )
			d[n] = s[n];
		
		if( numElements > buffer->numElements )
		{
			Construct(newBuffer, buffer->numElements, numElements);
		}
		else if( numElements < buffer->numElements )
		{
			Destruct(buffer, numElements, buffer->numElements);
		}
	}
	else
	{
		// Allocate memory for the buffer
		newBuffer = (sArrayBuffer*)malloc(sizeof(sArrayBuffer)-1+elementSize*numElements);
		newBuffer->numElements = numElements;

		int c = numElements > buffer->numElements ? buffer->numElements : numElements;
		memcpy(newBuffer->data, buffer->data, c*elementSize);
	}

	// Release the old buffer
	free(buffer);

	buffer = newBuffer;
}

void *asCArrayObject::at(asUINT index)
{
	if( index >= buffer->numElements )
	{
		asIScriptContext *ctx = asGetActiveContext();
		if( ctx )
			ctx->SetException(TXT_OUT_OF_BOUNDS);
		return 0;
	}
	else
	{
		if( objType->subType && !(arrayType & 1) )
			return *(void**)(buffer->data + sizeof(void*)*index);
		else
			return buffer->data + elementSize*index;
	}
}

void asCArrayObject::CreateBuffer(sArrayBuffer **buf, asUINT numElements)
{
	if( objType->subType )
	{
		*buf = (sArrayBuffer*)malloc(sizeof(sArrayBuffer)-1+sizeof(void*)*numElements);
		(*buf)->numElements = numElements;
	}
	else
	{
		*buf = (sArrayBuffer*)malloc(sizeof(sArrayBuffer)-1+elementSize*numElements);
		(*buf)->numElements = numElements;
	}

	Construct(*buf, 0, numElements);
}

void asCArrayObject::DeleteBuffer(sArrayBuffer *buf)
{
	Destruct(buf, 0, buf->numElements);

	// Free the buffer
	free(buf);
}

void asCArrayObject::Construct(sArrayBuffer *buf, asUINT start, asUINT end)
{
	if( arrayType & 1 )
	{
		// Set all object handles to null
		asDWORD *d = (asDWORD*)(buf->data + start * sizeof(void*));
		memset(d, 0, (end-start)*sizeof(void*));
	}
	else if( objType->subType )
	{
		// Call the constructor on all objects
		asCObjectType *subType = objType->subType;
		if( subType->flags & (asOBJ_SCRIPT_STRUCT | asOBJ_SCRIPT_ARRAY) )
		{
			asDWORD **max = (asDWORD**)(buf->data + end * sizeof(void*));
			asDWORD **d = (asDWORD**)(buf->data + start * sizeof(void*));

			if( subType->flags & asOBJ_SCRIPT_STRUCT ) 
			{
				for( ; d < max; d++ )
				{
					*d = (asDWORD*)engine->CallAlloc(subType->idx);
					ScriptStruct_Construct(subType->idx, (asCScriptStruct*)*d);
				}
			}
			else
			{
				for( ; d < max; d++ )
				{
					*d = (asDWORD*)engine->CallAlloc(subType->idx);
					ArrayObjectConstructor(subType->idx, (asCArrayObject*)*d);
				}
			}
		}
		else
		{
			int funcIndex = subType->beh.construct;
			asDWORD **max = (asDWORD**)(buf->data + end * sizeof(void*));
			asDWORD **d = (asDWORD**)(buf->data + start * sizeof(void*));

			if( funcIndex )
			{
				for( ; d < max; d++ )
				{
					*d = (asDWORD*)engine->CallAlloc(subType->idx);
					engine->CallObjectMethod(*d, funcIndex);
				}
			}
			else
			{
				for( ; d < max; d++ )
					*d = (asDWORD*)engine->CallAlloc(subType->idx);
			}
		}
	}
}

void asCArrayObject::Destruct(sArrayBuffer *buf, asUINT start, asUINT end)
{
	asUINT esize = elementSize;
	bool doDelete = true;
	if( objType->subType )
	{
		int funcIndex;
		if( objType->subType->beh.release )
		{
			funcIndex = objType->subType->beh.release;
			doDelete = false;
		}
		else
			funcIndex = objType->subType->beh.destruct;

		// Call the destructor on all of the objects
		asDWORD **max = (asDWORD**)(buf->data + end * sizeof(void*));
		asDWORD **d   = (asDWORD**)(buf->data + start * sizeof(void*));

		if( doDelete )
		{
			if( funcIndex )
			{
				for( ; d < max; d++ )
				{
					if( *d )
					{
						engine->CallObjectMethod(*d, funcIndex);
						engine->CallFree(objType->subType->idx, *d);
					}
				}
			}
			else
			{
				for( ; d < max; d++ )
				{
					if( *d )
						engine->CallFree(objType->subType->idx, *d);
				}
			}
		}
		else
		{
			for( ; d < max; d++ )
			{
				if( *d )
					engine->CallObjectMethod(*d, funcIndex);
			}
		}
	}
}


void asCArrayObject::CopyBuffer(sArrayBuffer *dst, sArrayBuffer *src)
{
	asUINT esize;
	if( arrayType & 1 )
	{
		// Copy the references and increase the reference counters
		int funcIndex = objType->subType->beh.addref;

		if( dst->numElements > 0 && src->numElements > 0 )
		{
			int count = dst->numElements > src->numElements ? src->numElements : dst->numElements;

			asDWORD **max = (asDWORD**)(dst->data + count * sizeof(void*));
			asDWORD **d   = (asDWORD**)dst->data;
			asDWORD **s   = (asDWORD**)src->data;
			
			for( ; d < max; d++, s++ )
			{
				*d = *s;
				if( *d )
					engine->CallObjectMethod(*d, funcIndex);
			}
		}
	}
	else
	{
		esize = elementSize;
		int funcIndex = 0;
		if( objType->subType )
		{
			funcIndex = objType->subType->beh.copy;
			esize = objType->subType->size;
		}

		if( dst->numElements > 0 && src->numElements > 0 )
		{
			int count = dst->numElements > src->numElements ? src->numElements : dst->numElements;
			if( objType->subType )
			{
				// Call the assignment operator on all of the objects
				asDWORD **max = (asDWORD**)(dst->data + count * sizeof(void*));
				asDWORD **d   = (asDWORD**)dst->data;
				asDWORD **s   = (asDWORD**)src->data;

				if( funcIndex )
				{
					for( ; d < max; d++, s++ )
						engine->CallObjectMethod(*d, *s, funcIndex);
				}
				else
				{
					for( ; d < max; d++, s++ )
						memcpy(*d, *s, esize);
				}
			}
			else
			{
				// Primitives are copied byte for byte
				memcpy(dst->data, src->data, count*esize);
			}
		}
	}
}

void asCArrayObject::Destruct()
{
	// Call the destructor, which will also call the GCObject's destructor
	this->~asCArrayObject();

	// Free the memory
	free(this);
}

void asCArrayObject::CountReferences()
{
	asCObjectType *subType = objType->subType;
	if( subType && subType->flags & asOBJ_POTENTIAL_CIRCLE )
	{
		asCGCObject **d = (asCGCObject**)buffer->data;
		for( int n = 0; n < buffer->numElements; n++ )
		{
			if( d[n] )
				d[n]->gcCount--;
		}
	}
}

void asCArrayObject::AddUnmarkedReferences(asCArray<asCGCObject*> &unmarked)
{
	asCObjectType *subType = objType->subType;
	if( subType && subType->flags & asOBJ_POTENTIAL_CIRCLE )
	{
		asCGCObject **d = (asCGCObject**)buffer->data;
		for( int n = 0; n < buffer->numElements; n++ )
		{
			if( d[n] && d[n]->gcCount == 0 )
				unmarked.PushLast(d[n]);
		}
	}
}

void asCArrayObject::ReleaseAllHandles()
{
	asCObjectType *subType = objType->subType;
	if( subType && subType->flags & asOBJ_POTENTIAL_CIRCLE )
	{
		asCGCObject **d = (asCGCObject**)buffer->data;
		for( int n = 0; n < buffer->numElements; n++ )
		{
			if( d[n] )
			{
				d[n]->Release();
				d[n] = 0;
			}
		}
	}
}
