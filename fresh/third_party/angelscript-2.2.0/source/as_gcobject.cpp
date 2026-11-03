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
#include "as_scriptengine.h"

#include "as_gcobject.h"

asCGCObject::asCGCObject()
{
	refCount = 1;
	objType = 0;
	gcCount = -1; // Initially marked as live
}

asCGCObject::~asCGCObject()
{
	if( objType ) objType->refCount--;
}

void asCGCObject::Init(asCObjectType *objType)
{
	this->objType = objType;
	objType->refCount++;

	if( objType->flags & asOBJ_POTENTIAL_CIRCLE )
		objType->engine->AddScriptObjectToGC(this);		
}

void asCGCObject::AddRef()
{
	// TODO: Must be made atomic
	// Increase counter and clear flag set by GC
	refCount = (refCount & 0x7FFFFFFF) + 1;
}

void asCGCObject::Release()
{
	// TODO: Must be made atomic
	// Decrease counter and clear flag set by GC
	refCount = (refCount & 0x7FFFFFFF) - 1;
	if( refCount == 0 )
	{
		Destruct();
	}
}

void GCObject_AddRef(asCGCObject *self)
{
	self->AddRef();
}

void GCObject_Release(asCGCObject *self)
{
	self->Release();
}

void GCObject_AddRef_Generic(asIScriptGeneric *gen)
{
	asCGCObject *self = (asCGCObject*)gen->GetObject();

	self->AddRef();
}

void GCObject_Release_Generic(asIScriptGeneric *gen)
{
	asCGCObject *self = (asCGCObject*)gen->GetObject();

	self->Release();
}




