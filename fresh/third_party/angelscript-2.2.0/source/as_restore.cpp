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
// as_restore.cpp
//
// Functions for saving and restoring module bytecode
// asCRestore was originally written by Dennis Bollyn, dennis@gyrbo.be

#include "as_config.h"
#include "as_restore.h"
#include "as_bytecodedef.h"

#define WRITE_NUM(N) stream->Write(&(N), sizeof(N))
#define READ_NUM(N) stream->Read(&(N), sizeof(N))

asCRestore::asCRestore(asCModule* _module, asIBinaryStream* _stream, asCScriptEngine* _engine)
 : module(_module), stream(_stream), engine(_engine)
{
#ifdef USE_ASM_VM
	for( int n = 0; n < BC_MAXBYTECODE; n++ )
		relocToBC.Insert(relocTable[n], n);
#endif
}

int asCRestore::Save() 
{
	unsigned long i, count;

	// structTypes[]
	count = module->structTypes.GetLength();
	WRITE_NUM(count);
	for( i = 0; i < count; ++i )
	{
		WriteObjectTypeDeclaration(module->structTypes[i]);
	}

	// usedTypeIndices[]
	count = module->usedTypeIndices.GetLength();
	WRITE_NUM(count);
	for( i = 0; i < count; ++i )
	{
		WriteObjectType(module->usedTypeIndices[i]);
	}

	// scriptGlobals[]
	count = module->scriptGlobals.GetLength();
	WRITE_NUM(count);
	for( i = 0; i < count; ++i ) 
		WriteProperty(module->scriptGlobals[i]);

	// globalMem size (can restore data using @init())
	count = module->globalMem.GetLength();
	WRITE_NUM(count);
	
	// initFunction
	WriteFunction(&module->initFunction);

	// exitFunction
	WriteFunction(&module->exitFunction);

	// scriptFunctions[]
	count = module->scriptFunctions.GetLength();
	WRITE_NUM(count);
	for( i = 0; i < count; ++i )
		WriteFunction(module->scriptFunctions[i]);

	// stringConstants[]
	count = module->stringConstants.GetLength();
	WRITE_NUM(count);
	for( i = 0; i < count; ++i ) 
		WriteBSTR(&module->stringConstants[i]);

	// importedFunctions[] and bindInformations[]
	count = module->importedFunctions.GetLength();
	WRITE_NUM(count);
	for( i = 0; i < count; ++i )
	{
		WriteFunction(module->importedFunctions[i]);
		WRITE_NUM(module->bindInformations[i].importFrom);
	}

	return asSUCCESS;
}

int asCRestore::Restore() 
{
	// Before starting the load, make sure that 
	// any existing resources have been freed
	module->Reset();

	unsigned long i, count;

	asCScriptFunction* func;
	asCProperty* prop;
	asBSTR bstr;

	// structTypes[]
	READ_NUM(count);
	module->structTypes.Allocate(count, 0);
	for( i = 0; i < count; ++i )
	{
		asCObjectType *ot = new asCObjectType(engine);
		ReadObjectTypeDeclaration(ot);
		engine->structTypes.PushLast(ot);
		module->structTypes.PushLast(ot);
		ot->refCount++;

		// Set the type indices for the object type declarations
		ot->idx = engine->AllocateTypeIndex();
		engine->allObjectTypes[ot->idx] = ot;
	}

	// usedTypeIndices[]
	READ_NUM(count);
	module->usedTypeIndices.Allocate(count, 0);
	for( i = 0; i < count; ++i )
	{
		module->usedTypeIndices.PushLast(ReadObjectType());
	}

	// scriptGlobals[]
	READ_NUM(count);
	module->scriptGlobals.Allocate(count, 0);
	for( i = 0; i < count; ++i ) 
	{
		prop = new asCProperty;
		ReadProperty(prop);
		module->scriptGlobals.PushLast(prop);
	}

	// globalMem size
	READ_NUM(count);
	module->globalMem.Allocate(count, 0);

	// initFunction
	ReadFunction(&module->initFunction);

	// exitFunction
	ReadFunction(&module->exitFunction);

	// scriptFunctions[]
	READ_NUM(count);
	module->scriptFunctions.Allocate(count, 0);
	for( i = 0; i < count; ++i ) 
	{
		func = new asCScriptFunction;
		ReadFunction(func);
		module->scriptFunctions.PushLast(func);
	}

	// stringConstants[]
	READ_NUM(count);
	module->stringConstants.Allocate(count, 0);
	for(i=0;i<count;++i) 
	{
		bstr = ReadBSTR();
		module->stringConstants.PushLast(bstr);
	}
	
	// importedFunctions[] and bindInformations[]
	READ_NUM(count);
	module->importedFunctions.Allocate(count, 0);
	module->bindInformations.SetLength(count);
	for(i=0;i<count;++i)
	{
		func = new asCScriptFunction;
		ReadFunction(func);
		module->importedFunctions.PushLast(func);

		READ_NUM(module->bindInformations[i].importFrom);
		module->bindInformations[i].importedFunction = -1;
	}
	
	// Fake building
	module->isBuildWithoutErrors = true;

	// Init system functions properly
	engine->PrepareEngine();

	module->CallInit();

	return 0;
}

void asCRestore::WriteString(asCString* str) 
{
	unsigned long len = str->GetLength();
	WRITE_NUM(len);
	stream->Write(str->AddressOf(), len);
}

void asCRestore::WriteFunction(asCScriptFunction* func) 
{
	int i, count;

	WriteString(&func->name);

	WriteDataType(&func->returnType);

	count = func->parameterTypes.GetLength();
	WRITE_NUM(count);
	for( i = 0; i < count; ++i ) 
		WriteDataType(&func->parameterTypes[i]);

	WRITE_NUM(func->id);
	
	count = func->byteCode.GetLength();
	WRITE_NUM(count);
	WriteByteCode(func->byteCode.AddressOf(), count);

	count = func->objVariablePos.GetLength();
	WRITE_NUM(count);
	for( i = 0; i < count; ++i )
	{
		WriteObjectType(func->objVariableTypes[i]);
		WRITE_NUM(func->objVariablePos[i]);
	}

	WRITE_NUM(func->stackNeeded);

	WriteObjectType(func->objectType);

	int length = func->lineNumbers.GetLength();
	WRITE_NUM(length);
	for( i = 0; i < length; ++i )
		WRITE_NUM(func->lineNumbers[i]);
}

void asCRestore::WriteProperty(asCProperty* prop) 
{
	WriteString(&prop->name);
	WriteDataType(&prop->type);
	WRITE_NUM(prop->index);
}

void asCRestore::WriteBSTR(asBSTR* str) 
{
	unsigned long count;

	count = strlen((char*)*str);
	WRITE_NUM(count);

	stream->Write(*str, count);
}

void asCRestore::WriteDataType(asCDataType* dt) 
{
	if( dt->IsScriptArray() )
	{
		bool b = true;
		WRITE_NUM(b);

		b = dt->IsObjectHandle();
		WRITE_NUM(b);
		b = dt->IsReadOnly();
		WRITE_NUM(b);
		b = dt->IsHandleToConst();
		WRITE_NUM(b);
		b = dt->IsReference();
		WRITE_NUM(b);

		asCDataType sub = dt->GetSubType();
		WriteDataType(&sub);
	}
	else
	{
		bool b = false;
		WRITE_NUM(b);

		int t = dt->GetTokenType();
		WRITE_NUM(t);
		WriteObjectType(dt->GetObjectType());
		b = dt->IsObjectHandle();
		WRITE_NUM(b);
		b = dt->IsReadOnly();
		WRITE_NUM(b);
		b = dt->IsHandleToConst();
		WRITE_NUM(b);
		b = dt->IsReference();
		WRITE_NUM(b);
	}
}

void asCRestore::WriteObjectType(asCObjectType* ot) 
{
	// Only write the object type name
	if( ot )
		WriteString(&ot->name);
	else
	{
		// Write a null string
		asDWORD null = 0;
		WRITE_NUM(null);
	}
}

void asCRestore::WriteObjectTypeDeclaration(asCObjectType *ot)
{
	// name
	WriteString(&ot->name);
	// size
	int size = ot->size;
	WRITE_NUM(size);
	// properties[]
	size = ot->properties.GetLength();
	WRITE_NUM(size);
	for( int n = 0; n < ot->properties.GetLength(); n++ )
	{
		WriteProperty(ot->properties[n]);
	}

	// TODO:
	// methods[]
	// behaviours
}

void asCRestore::ReadString(asCString* str) 
{
	unsigned long len;
	READ_NUM(len);
	str->SetLength(len);
	stream->Read(str->AddressOf(), len);
	str->RecalculateLength();
}

void asCRestore::ReadFunction(asCScriptFunction* func) 
{
	int i, count;
	asCDataType dt;
	int num;

	ReadString(&func->name);

	ReadDataType(&func->returnType);

	READ_NUM(count);
	func->parameterTypes.Allocate(count, 0);
	for( i = 0; i < count; ++i ) 
	{
		ReadDataType(&dt);
		func->parameterTypes.PushLast(dt);
	}

	READ_NUM(func->id);
	
	READ_NUM(count);
	func->byteCode.Allocate(count, 0);
	ReadByteCode(func->byteCode.AddressOf(), count);
	func->byteCode.SetLength(count);

	READ_NUM(count);
	func->objVariablePos.Allocate(count, 0);
	func->objVariableTypes.Allocate(count, 0);
	for( i = 0; i < count; ++i )
	{
		func->objVariableTypes.PushLast(ReadObjectType());
		READ_NUM(num);
		func->objVariablePos.PushLast(num);
	}

	READ_NUM(func->stackNeeded);

	func->objectType = ReadObjectType();

	int length;
	READ_NUM(length);
	func->lineNumbers.SetLength(length);
	for( i = 0; i < length; ++i )
		READ_NUM(func->lineNumbers[i]);
}

void asCRestore::ReadProperty(asCProperty* prop) 
{
	ReadString(&prop->name);
	ReadDataType(&prop->type);
	READ_NUM(prop->index);
}

asBSTR asCRestore::ReadBSTR() 
{
	unsigned long count;

	READ_NUM(count);
	asBSTR str = asBStrAlloc(count);

	stream->Read(str, count);
	
	return str;
}

void asCRestore::ReadDataType(asCDataType* dt) 
{
	bool b;
	READ_NUM(b);
	if( b ) 
	{
		bool isObjectHandle;
		READ_NUM(isObjectHandle);
		bool isReadOnly;
		READ_NUM(isReadOnly);
		bool isHandleToConst;
		READ_NUM(isHandleToConst);
		bool isReference;
		READ_NUM(isReference);

		asCDataType sub;
		ReadDataType(&sub);

		*dt = sub;
		dt->MakeArray(engine);
		if( isObjectHandle )
		{
			dt->MakeReadOnly(isHandleToConst);
			dt->MakeHandle(true);
		}
		dt->MakeReadOnly(isReadOnly);
		dt->MakeReference(isReference);
	}
	else
	{
		eTokenType tokenType;
		READ_NUM(tokenType);
		asCObjectType *objType = ReadObjectType();
		bool isObjectHandle;
		READ_NUM(isObjectHandle);
		bool isReadOnly;
		READ_NUM(isReadOnly);
		bool isHandleToConst;
		READ_NUM(isHandleToConst);
		bool isReference;
		READ_NUM(isReference);

		if( tokenType == ttIdentifier )
			*dt = asCDataType::CreateObject(objType, false);
		else
			*dt = asCDataType::CreatePrimitive(tokenType, false);
		if( isObjectHandle )
		{
			dt->MakeReadOnly(isHandleToConst);
			dt->MakeHandle(true);
		}
		dt->MakeReadOnly(isReadOnly);
		dt->MakeReference(isReference);
	}
}

asCObjectType* asCRestore::ReadObjectType() 
{
	// Read the object type name
	asCString typeName;
	ReadString(&typeName);

	// Find the registered object type in the engine
	asCObjectType *ot = engine->GetObjectType(typeName);
	if( !ot )
		ot = module->GetObjectType(typeName);

	return ot;
}

void asCRestore::ReadObjectTypeDeclaration(asCObjectType *ot)
{
	// name
	ReadString(&ot->name);
	// size
	int size;
	READ_NUM(size);
	ot->size = size;
	// properties[]
	READ_NUM(size);
	ot->properties.Allocate(size,0);
	for( int n = 0; n < size; n++ )
	{
		asCProperty *prop = new asCProperty;
		ReadProperty(prop);
		ot->properties.PushLast(prop);
	}

	// Use the default script struct behaviours
	ot->beh.construct = engine->scriptTypeBehaviours.beh.construct;
	ot->beh.constructors.PushLast(ot->beh.construct);
	ot->beh.addref = engine->scriptTypeBehaviours.beh.addref;
	ot->beh.release = engine->scriptTypeBehaviours.beh.release;
	ot->beh.copy = engine->scriptTypeBehaviours.beh.copy;
	ot->beh.operators.PushLast(ttAssignment);
	ot->beh.operators.PushLast(ot->beh.copy);

	// TODO:
	// methods[]
}

void asCRestore::WriteByteCode(asBYTE *bc, int length)
{
	while( length )
	{
#ifdef USE_ASM_VM
		asDWORD reloc = *(asDWORD*)bc;
		asBYTE c = FindByteCode(reloc);
#else
		asBYTE c = *bc;
#endif
		bc += 4;
		WRITE_NUM(c);
		for( int n = 4; n < bcSize[c]; n++ )
			WRITE_NUM(*bc++);

		length -= bcSize[c];
	}
}

void asCRestore::ReadByteCode(asBYTE *bc, int length)
{
	while( length )
	{
		asBYTE c;
		READ_NUM(c);
#ifdef USE_ASM_VM
		*(asDWORD*)bc = relocTable[c];
#else
		*(asDWORD*)bc = asDWORD(c);
#endif
		bc += 4;
		for( int n = 4; n < bcSize[c]; n++ )
			READ_NUM(*bc++);

		length -= bcSize[c];
	}
}

#ifdef USE_ASM_VM
asBYTE asCRestore::FindByteCode(asDWORD relocation)
{
	asDWORD bc;
	relocToBC.MoveTo(relocation);
	bc = relocToBC.GetValue();

	return asBYTE(bc);
}
#endif

