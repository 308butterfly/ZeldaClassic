
#include "../precompiled.h" //always first

#include "DataStructs.h"
#include "../zsyssimple.h"
#include <assert.h>
#include <iostream>

using std::cout;
using std::endl;

////////////////////////////////////////////////////////////////
// FunctionSignature

FunctionSignature::FunctionSignature(string const& name, vector<ZVarTypeId> const& paramTypeIds)
	: name(name), paramTypeIds(paramTypeIds)
{}

int FunctionSignature::compare(FunctionSignature const& other) const
{
	int c = name.compare(other.name);
	if (c) return c;
	c = paramTypeIds.size() - other.paramTypeIds.size();
	if (c) return c;
	for (int i = 0; i < paramTypeIds.size(); ++i)
	{
		c = paramTypeIds[i] - other.paramTypeIds[i];
		if (c) return c;
	}
	return 0;
}

////////////////////////////////////////////////////////////////
// FunctionTypeIds

FunctionTypeIds::FunctionTypeIds(ZVarTypeId returnTypeId, vector<ZVarTypeId> const& paramTypeIds)
	: returnTypeId(returnTypeId), paramTypeIds(paramTypeIds)
{}

int FunctionTypeIds::compare(FunctionTypeIds const& other) const
{
	int c = returnTypeId - other.returnTypeId;
	if (c) return c;
	c = paramTypeIds.size() - other.paramTypeIds.size();
	if (c) return c;
	for (int i = 0; i < paramTypeIds.size(); ++i)
	{
		c = paramTypeIds[i] - other.paramTypeIds[i];
		if (c) return c;
	}
	return 0;
}

FunctionTypeIds const FunctionTypeIds::null;

////////////////////////////////////////////////////////////////
// VariableSymbols

int VariableSymbols::addVariable(string name, int type)
{

    map<string, pair<int, int> >::iterator it = symbols.find(name);
    
    if(it != symbols.end())
        return -1;
        
    int id = ScriptParser::getUniqueVarID();
    symbols[name] = pair<int, int>(type, id);
    return id;
}

bool VariableSymbols::containsVariable(string name)
{
    return (symbols.find(name) != symbols.end());
}

int VariableSymbols::getID(string name)
{
    map<string, pair<int, int> >::iterator it = symbols.find(name);
    assert(it != symbols.end());
    return it->second.second;
}

////////////////////////////////////////////////////////////////
// FunctionSymbols

int FunctionSymbols::addFunction(string name, int rettype, vector<int> paramtype)
{
    map<pair<string, vector<int> >, pair<int, int> >::iterator it = symbols.find(pair<string, vector<int> >(name, paramtype));
    
    if(it != symbols.end())
        return -1;
        
    int id = ScriptParser::getUniqueFuncID();
    symbols[pair<string, vector<int> >(name, paramtype)] = pair<int, int>(rettype, id);
    map<string, vector<int> >::iterator it2 = ambiguous.find(name);
    
    if(it2 == ambiguous.end())
    {
        vector<int> newv;
        newv.push_back(id);
        ambiguous[name] = newv;
    }
    else
    {
        (*it2).second.push_back(id);
    }
    
    return id;
}

bool FunctionSymbols::containsFunction(string name, vector<int> &params)
{
    return symbols.find(pair<string, vector<int> >(name,params)) != symbols.end();
}

int FunctionSymbols::getID(string name, vector<int> &params)
{
    map<pair<string, vector<int> >, pair<int, int> >::iterator it = symbols.find(pair<string, vector<int> >(name, params));
    assert(it != symbols.end());
    return it->second.second;
}

vector<int> FunctionSymbols::getFuncIDs(string name)
{
    map<string, vector<int> >::iterator it = ambiguous.find(name);
    
    if(it == ambiguous.end())
        return vector<int>();
        
    return it->second;
}

////////////////////////////////////////////////////////////////
// SymbolTable

SymbolTable::SymbolTable(map<string, long> *consts)
	: nodeIds(), possibleNodeFuncIds(), varTypes(), funcTypes(), constants(consts)
{}

// Nodes

int SymbolTable::getNodeId(AST* node) const
{
	map<AST*, int>::const_iterator it = nodeIds.find(node);
	if (it == nodeIds.end()) return -1;
	return it->second;
}

void SymbolTable::putNodeId(AST* node, int id)
{
    nodeIds[node] = id;
}

vector<int> SymbolTable::getPossibleNodeFuncIds(AST* node) const
{
	map<AST*, vector<int>>::const_iterator it = possibleNodeFuncIds.find(node);
	if (it == possibleNodeFuncIds.end()) return vector<int>();
	return it->second;
}

void SymbolTable::putPossibleNodeFuncIds(AST* node, vector<int> possibleFuncIds)
{
	possibleNodeFuncIds[node] = possibleFuncIds;
}

// Variables

ZVarTypeId SymbolTable::getVarTypeId(int varId) const
{
    map<int, ZVarTypeId>::const_iterator it = varTypes.find(varId);

    if (it == varTypes.end())
    {
        box_out("Internal Error: Can't find the variable type!");
        box_eol();
        return -1;
    }

    return it->second;
}

ZVarTypeId SymbolTable::getVarTypeId(AST* obj) const
{
    map<AST*, int>::const_iterator it = nodeIds.find(obj);

    if (it == nodeIds.end())
    {
        box_out("Internal Error: Can't find the AST ID!");
        box_eol();
        return -1;
    }

    return getVarTypeId(it->second);
}

void SymbolTable::putVarTypeId(int varId, ZVarTypeId typeId)
{
	varTypes[varId] = typeId;
}

// Functions

ZVarTypeId SymbolTable::getFuncReturnTypeId(int funcId) const
{
	map<int, FunctionTypeIds>::const_iterator it = funcTypes.find(funcId);
	if (it == funcTypes.end()) return -1;
	return it->second.returnTypeId;
}

ZVarTypeId SymbolTable::getFuncReturnTypeId(AST* node) const
{
    return getFuncReturnTypeId(getNodeId(node));
}

vector<ZVarTypeId> SymbolTable::getFuncParamTypeIds(int funcId) const
{
	map<int, FunctionTypeIds>::const_iterator it = funcTypes.find(funcId);
	if (it == funcTypes.end()) return vector<ZVarTypeId>();
	return it->second.paramTypeIds;
}

void SymbolTable::putFuncTypeIds(int funcId, ZVarTypeId returnTypeId, vector<ZVarTypeId> const& paramTypeIds)
{
	funcTypes[funcId] = FunctionTypeIds(returnTypeId, paramTypeIds);
}

// 

bool SymbolTable::isConstant(string name) const
{
    return constants->find(name) != constants->end();
}

long SymbolTable::getConstantVal(string name) const
{
    return (*constants)[name];
}

void SymbolTable::printDiagnostics()
{
    cout << (unsigned int)varTypes.size() << " variable symbols" << endl;
    cout << (unsigned int)funcTypes.size() << " function symbols" << endl;
}

////////////////////////////////////////////////////////////////
// Scope
bool Scope::addNamedChild(string name, Scope *child)
{
    map<string, Scope *>::iterator it = namedChildren.find(name);
    
    if(it != namedChildren.end())
        return false;
        
    namedChildren[name]=child;
    return true;
}

Scope::~Scope()
{
    map<string, Scope *>::iterator it;
    
    for(it = namedChildren.begin(); it != namedChildren.end(); it++)
    {
        delete it->second;
    }
    
    namedChildren.clear();
}

int Scope::getVarInScope(string nspace, string name)
{
    if(nspace == "" && getVarSymbols().containsVariable(name))
        return getVarSymbols().getID(name);
        
    map<string, Scope *>::iterator it = namedChildren.find(nspace);
    
    if(it != namedChildren.end())
    {
        int id = (*it).second->getVarInScope("", name);
        
        if(id != -1)
            return id;
    }
    
    if(parent == NULL)
        return -1;
        
    return parent->getVarInScope(nspace, name);
}

vector<int> Scope::getFuncsInScope(string nspace, string name)
{
    vector<int> rval;
    
    if(nspace == "")
    {
        vector<int> thisscope = getFuncSymbols().getFuncIDs(name);
        
        for(vector<int>::iterator it = thisscope.begin(); it != thisscope.end(); it++)
        {
            rval.push_back(*it);
        }
    }
    
    map<string, Scope *>::iterator it = namedChildren.find(nspace);
    
    if(it != namedChildren.end())
    {
        vector<int> childscope = (*it).second->getFuncsInScope("", name);
        
        for(vector<int>::iterator it2 = childscope.begin(); it2 != childscope.end(); it2++)
        {
            rval.push_back(*it2);
        }
    }
    
    if(parent != NULL)
    {
        vector<int> abovescope = parent->getFuncsInScope(nspace,name);
        
        for(vector<int>::iterator it2 = abovescope.begin(); it2 != abovescope.end(); it2++)
        {
            rval.push_back(*it2);
        }
    }
    
    return rval;
}

Scope *Scope::getNamedChild(string name)
{
    return namedChildren[name];
}

////////////////////////////////////////////////////////////////

int StackFrame::getOffset(int vid)
{
    map<int, int>::iterator it = stackoffset.find(vid);
    
    if(it == stackoffset.end())
    {
        box_out("Internal Error: Can't find stack offset for variable!");
        box_eol();
        return 0;
    }
    
    return stackoffset[vid];
}

