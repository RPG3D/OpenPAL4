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



//
// as_gcobject.h
//
// A garbage collectable object
//



#ifndef AS_GCOBJECT_H
#define AS_GCOBJECT_H

#include "as_config.h"
#include "as_array.h"

class asCObjectType;

class asCGCObject
{
public:
	asCGCObject();
	virtual ~asCGCObject();
	void Init(asCObjectType *objType);

	void AddRef();
	void Release();

	// Necessary GC methods
	virtual void Destruct() = 0;
	virtual void CountReferences() = 0;
	virtual void AddUnmarkedReferences(asCArray<asCGCObject*> &unmarked) = 0;
	virtual void ReleaseAllHandles() = 0;

//protected:
	int refCount;
	asCObjectType *objType;
	int gcCount;
};

void GCObject_AddRef(asCGCObject *self);
void GCObject_Release(asCGCObject *self);
void GCObject_AddRef_Generic(asIScriptGeneric *gen);
void GCObject_Release_Generic(asIScriptGeneric *gen);

#endif
