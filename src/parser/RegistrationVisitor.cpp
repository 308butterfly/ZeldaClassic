/**
 * Registration handler, to ensure that anything global can be referenced in any order
 * Begun: 26th June, 2019
 * Author: Venrob
 */

#include "parserDefs.h"
#include "RegistrationVisitor.h"
#include <cassert>
#include "Scope.h"
#include "CompileError.h"

#include "../ffscript.h"
extern FFScript FFCore;
using std::string;
using std::vector;
using namespace ZScript;

////////////////////////////////////////////////////////////////
// RegistrationVisitor

RegistrationVisitor::RegistrationVisitor(Program& program)
	: program(program), hasChanged(false)
{
	scope = &program.getScope();
	caseRoot(program.getRoot());
	assert(dynamic_cast<RootScope*>(scope)); //Ensure no scope corruption
}

void RegistrationVisitor::visit(AST& node, void* param)
{
	if(node.isDisabled()) return; //Don't visit disabled nodes.
	if(registered(node)) return; //Don't double-register
	RecursiveVisitor::visit(node, param);
}

template <class Container>
void RegistrationVisitor::regvisit(AST& host, Container const& nodes, void* param)
{
	for (typename Container::const_iterator it = nodes.begin();
		 it != nodes.end(); ++it)
	{
		if (breakRecursion(host, param)) return;
		visit(**it, param);
	}
}

void RegistrationVisitor::caseDefault(AST& host, void* param)
{
	doRegister(host);
}

//Handle the root file specially!
void RegistrationVisitor::caseRoot(ASTFile& host, void* param)
{
	int recursionLimit = REGISTRATION_REC_LIMIT;
	while(--recursionLimit)
	{
		caseFile(host, param);
		if(registered(host)) return;
		if(!hasChanged) return; //Nothing new was registered on this pass. Only errors remain.
		hasChanged = false;
	}
	//Failed recursionLimit
	handleError(CompileError::RegistrationRecursion(&host, REGISTRATION_REC_LIMIT));
}

void RegistrationVisitor::caseFile(ASTFile& host, void* param)
{
	if(host.scope)
		scope = host.scope;
	else
		scope = host.scope = scope->makeFileChild(host.asString());
	regvisit(host, host.options, param);
	if (breakRecursion(host, param)) {scope = scope->getParent(); return;}
	regvisit(host, host.use, param);
	if (breakRecursion(host, param)) {scope = scope->getParent(); return;}
	regvisit(host, host.dataTypes, param);
	if (breakRecursion(host, param)) {scope = scope->getParent(); return;}
	regvisit(host, host.scriptTypes, param);
	if (breakRecursion(host, param)) {scope = scope->getParent(); return;}
	regvisit(host, host.imports, param);
	if (breakRecursion(host, param)) {scope = scope->getParent(); return;}
	regvisit(host, host.variables, param);
	if (breakRecursion(host, param)) {scope = scope->getParent(); return;}
	regvisit(host, host.functions, param);
	if (breakRecursion(host, param)) {scope = scope->getParent(); return;}
	regvisit(host, host.namespaces, param);
	if (breakRecursion(host, param)) {scope = scope->getParent(); return;}
	regvisit(host, host.scripts, param);
	if(registered(host, host.options) && registered(host, host.use) && registered(host, host.dataTypes)
		&& registered(host, host.scriptTypes) && registered(host, host.imports) && registered(host, host.variables)
		&& registered(host, host.functions) && registered(host, host.namespaces) && registered(host, host.scripts))
	{
		doRegister(host);
	}
	scope = scope->getParent();
}

void RegistrationVisitor::caseSetOption(ASTSetOption& host, void* param)
{
	visit(host.expr.get(), param);
	if(!registered(host.expr.get())) return; //Non-initialized constant
	
	// If the option name is "default", set the default option instead.
	if (host.name == "default")
	{
		CompileOptionSetting setting = host.getSetting(this, scope);
		if (!setting) return; // error
		scope->setDefaultOption(setting);
		return;
	}
	
	// Make sure the option is valid.
	if (!host.option.isValid())
	{
		handleError(CompileError::UnknownOption(&host, host.name));
		return;
	}

	// Set the option to the provided value.
	CompileOptionSetting setting = host.getSetting(this, scope);
	if (!setting) return; // error
	scope->setOption(host.option, setting);
	doRegister(host);
}

// Declarations
void RegistrationVisitor::caseScript(ASTScript& host, void* param)
{
	visit(host.type.get());
	if(!registered(host.type.get())) return;
	
	Script& script = host.script ? *host.script : *(host.script = program.addScript(host, *scope, this));
	if (breakRecursion(host)) return;
	
	string name = script.getName();

	// Recurse on script elements with its scope.
	scope = &script.getScope();
	regvisit(host, host.options, param);
	if (breakRecursion(host, param)) {scope = scope->getParent(); return;}
	regvisit(host, host.use, param);
	if (breakRecursion(host, param)) {scope = scope->getParent(); return;}
	regvisit(host, host.types, param);
	if (breakRecursion(host, param)) {scope = scope->getParent(); return;}
	regvisit(host, host.variables, param);
	if (breakRecursion(host, param)) {scope = scope->getParent(); return;}
	regvisit(host, host.functions, param);
	scope = scope->getParent();
	if (breakRecursion(host)) return;
	//
	if(registered(host, host.options) && registered(host, host.use) && registered(host, host.types)
		&& registered(host, host.variables) && registered(host, host.functions))
	{
		doRegister(host);
	}
	else return;
	//
	if(script.getType() == ScriptType::untyped) return;
	// Check for a valid run function.
	vector<Function*> possibleRuns =
		//script.getScope().getLocalFunctions("run");
		script.getScope().getLocalFunctions(FFCore.scriptRunString);
	if (possibleRuns.size() == 0)
	{
		handleError(CompileError::ScriptNoRun(&host, name, FFCore.scriptRunString));
		if (breakRecursion(host)) return;
	}
	if (possibleRuns.size() > 1)
	{
		handleError(CompileError::TooManyRun(&host, name, FFCore.scriptRunString));
		if (breakRecursion(host)) return;
	}
	if (*possibleRuns[0]->returnType != DataType::ZVOID)
	{
		handleError(CompileError::ScriptRunNotVoid(&host, name, FFCore.scriptRunString));
		if (breakRecursion(host)) return;
	}
}

void RegistrationVisitor::caseNamespace(ASTNamespace& host, void* param)
{
	Namespace& namesp = host.namesp ? *host.namesp : (*(host.namesp = program.addNamespace(host, *scope, this)));
	if (breakRecursion(host)) return;

	// Recurse on script elements with its scope.
	// Namespaces' parent scope is RootScope*, not FileScope*. Store the FileScope* temporarily.
	Scope* temp = scope;
	scope = &namesp.getScope();
	regvisit(host, host.options, param);
	if (breakRecursion(host, param)) {scope = temp; return;}
	regvisit(host, host.dataTypes, param);
	if (breakRecursion(host, param)) {scope = temp; return;}
	regvisit(host, host.scriptTypes, param);
	if (breakRecursion(host, param)) {scope = temp; return;}
	regvisit(host, host.use, param);
	if (breakRecursion(host, param)) {scope = temp; return;}
	regvisit(host, host.variables, param);
	if (breakRecursion(host, param)) {scope = temp; return;}
	regvisit(host, host.functions, param);
	if (breakRecursion(host, param)) {scope = temp; return;}
	regvisit(host, host.namespaces, param);
	if (breakRecursion(host, param)) {scope = temp; return;}
	regvisit(host, host.scripts, param);
	scope = temp;
	if(registered(host, host.options) && registered(host, host.use) && registered(host, host.dataTypes)
		&& registered(host, host.scriptTypes) && registered(host, host.variables) && registered(host, host.functions)
		&& registered(host, host.namespaces) && registered(host, host.scripts))
	{
		doRegister(host);
	}
}

void RegistrationVisitor::caseImportDecl(ASTImportDecl& host, void* param)
{
	//Check if the import is valid, or to be stopped by header guard. -V
	if(host.wasChecked() || getRoot(*scope)->checkImport(&host, *lookupOption(*scope, CompileOption::OPT_HEADER_GUARD) / 10000.0, this))
	{
		host.check();
		visit(host.getTree(), param);
		if(registered(host.getTree())) doRegister(host);
	}
	else
	{
		host.disable(); //Do not use this import; it is a duplicate, and duplicates have been disallowed! -V
	}
}

void RegistrationVisitor::caseUsing(ASTUsingDecl& host, void* param)
{
	//Handle adding scope
	ASTExprIdentifier* iden = host.getIdentifier();
	vector<string> components = iden->components;
	Scope* temp = host.always ? getRoot(*scope) : scope;
	int numMatches = temp->useNamespace(components, iden->delimiters);
	if(numMatches > 1)
		handleError(CompileError::TooManyUsing(&host, iden->asString()));
	else if(numMatches == -1)
		handleError(CompileError::DuplicateUsing(&host, iden->asString()));
	else if(numMatches==1)
		doRegister(host);
}

void RegistrationVisitor::caseDataTypeDef(ASTDataTypeDef& host, void* param)
{
	visit(host.type.get());
	if (breakRecursion(*host.type.get())) return;
	if(!registered(host.type.get())) return;
	// Add type to the current scope under its new name.
	DataType const& type = host.type->resolve(*scope, this);
	if(!scope->addDataType(host.name, &type, &host))
	{
		DataType const& originalType = *lookupDataType(*scope, host.name);
		if (originalType != type)
			handleError(
				CompileError::RedefDataType(
					&host, host.name, originalType.getName()));
		return;
	}
	doRegister(host);
}

void RegistrationVisitor::caseCustomDataTypeDef(ASTCustomDataTypeDef& host, void* param)
{
	if(!host.type)
	{
		//Don't allow use of a name that already exists
		if(DataType const* existingType = lookupDataType(*scope, host.name))
		{
			handleError(
				CompileError::RedefDataType(
					&host, host.name, existingType->getName()));
			return;
		}
		
		//Construct a new constant type
		DataTypeCustomConst* newConstType = new DataTypeCustomConst("const " + host.name);
		//Construct the base type
		DataTypeCustom* newBaseType = new DataTypeCustom(host.name, newConstType, newConstType->getCustomId());
		
		//Set the type to the base type
		host.type.reset(new ASTDataType(newBaseType, host.location));
		//Set the enum type to the const type
		host.definition->baseType.reset(new ASTDataType(newConstType, host.location));
		
		DataType::addCustom(newBaseType);
		
		//This call should never fail, because of the error check above.
		scope->addDataType(host.name, newBaseType, &host);
		if (breakRecursion(*host.type.get())) return;
	}
	visit(host.definition.get());
	if(registered(host.definition.get())) doRegister(host);
}

void RegistrationVisitor::caseScriptTypeDef(ASTScriptTypeDef& host, void* param)
{
	// Resolve the base type under current scope.
	ScriptType type = resolveScriptType(*host.oldType, *scope);
	if (!type.isValid()) return;

	// Add type to the current scope under its new name.
	if (!scope->addScriptType(host.newName, type, &host))
	{
		ScriptType originalType = lookupScriptType(*scope, host.newName);
		if (originalType != type)
			handleError(
				CompileError::RedefScriptType(
					&host, host.newName, originalType.getName()));
		return;
	}
	doRegister(host);
}

void RegistrationVisitor::caseDataDeclList(ASTDataDeclList& host, void* param)
{
	// Resolve the base type.
	DataType const& baseType = host.baseType->resolve(*scope, this);
    if (breakRecursion(*host.baseType.get())) return;
	if (!&baseType || !baseType.isResolved()) return;

	// Don't allow void type.
	if (baseType == DataType::ZVOID)
	{
		handleError(CompileError::VoidVar(&host, host.asString()));
		return;
	}

	// Check for disallowed global types.
	if (scope->isGlobal() && !baseType.canBeGlobal())
	{
		handleError(CompileError::RefVar(&host, baseType.getName()));
		return;
	}

	// Recurse on list contents.
	visit(host, host.getDeclarations());
	if (breakRecursion(host)) return;
	if(registered(host, host.getDeclarations())) doRegister(host);
}

void RegistrationVisitor::caseDataEnum(ASTDataEnum& host, void* param)
{
	// Resolve the base type.
	DataType const& baseType = host.baseType->resolve(*scope, this);
    if (breakRecursion(*host.baseType.get())) return;
	if (!baseType.isResolved()) return;

	// Don't allow void type.
	if (baseType == DataType::ZVOID)
	{
		handleError(CompileError::VoidVar(&host, host.asString()));
		return;
	}

	// Check for disallowed global types.
	if (scope->isGlobal() && !baseType.canBeGlobal())
	{
		handleError(CompileError::RefVar(&host, baseType.getName()));
		return;
	}

	//Handle initializer assignment
	long ipart = -1, dpart = 0;
	std::vector<ASTDataDecl*> decls = host.getDeclarations();
	for(vector<ASTDataDecl*>::iterator it = decls.begin();
		it != decls.end(); ++it)
	{
		ASTDataDecl* declaration = *it;
		if(ASTExpr* init = declaration->getInitializer())
		{
			if(init->getCompileTimeValue())
			{
				long val = *init->getCompileTimeValue();
				ipart = val/10000;
				dpart = val%10000;
			}
			else return;
		}
		else
		{
			ASTNumberLiteral* value = new ASTNumberLiteral(new ASTFloat(++ipart, dpart, host.location), host.location);
			declaration->setInitializer(value);
		}
		visit(declaration, param);
		if(breakRecursion(host, param)) return;
	}
	if(registered(host, host.getDeclarations())) doRegister(host);
}

void RegistrationVisitor::caseDataDecl(ASTDataDecl& host, void* param)
{
	// First do standard recursing.
	RecursiveVisitor::caseDataDecl(host, paramRead);
	if (breakRecursion(host)) return;
	if(!(registered(host, host.extraArrays) && (!host.getInitializer() || registered(host.getInitializer())))) return;
	// Then resolve the type.
	DataType const& type = *host.resolveType(scope, this);
	if (breakRecursion(host)) return;
	if (!type.isResolved()) return;

	// Don't allow void type.
	if (type == DataType::ZVOID)
	{
		handleError(CompileError::VoidVar(&host, host.name));
		return;
	}

	// Check for disallowed global types.
	if (scope->isGlobal() && !type.canBeGlobal())
	{
		handleError(CompileError::RefVar(
				            &host, type.getName() + " " + host.name));
		return;
	}

	// Currently disabled syntaxes:
	if (getArrayDepth(type) > 1)
	{
		handleError(CompileError::UnimplementedFeature(
				            &host, "Nested Array Declarations"));
		return;
	}

	// Is it a constant?
	bool isConstant = false;
	if (type.isConstant())
	{
		// A constant without an initializer doesn't make sense.
		if (!host.getInitializer())
		{
			handleError(CompileError::ConstUninitialized(&host));
			return;
		}

		// Inline the constant if possible.
		isConstant = host.getInitializer()->getCompileTimeValue(this, scope);
		//The dataType is constant, but the initializer is not. This is not allowed in Global or Script scopes, as it causes crashes. -V
		if(!isConstant && (scope->isGlobal() || scope->isScript()))
		{
			handleError(CompileError::ConstNotConstant(&host, host.name));
			return;
		}
	}

	if (isConstant)
	{
		if (scope->getLocalDatum(host.name))
		{
			handleError(CompileError::VarRedef(&host, host.name));
			return;
		}
		
		long value = *host.getInitializer()->getCompileTimeValue(this, scope);
		Constant::create(*scope, host, type, value, this);
	}
	else
	{
		if (scope->getLocalDatum(host.name))
		{
			handleError(CompileError::VarRedef(&host, host.name));
			return;
		}

		Variable::create(*scope, host, type, this);
	}
	doRegister(host);
}

void RegistrationVisitor::caseDataDeclExtraArray(ASTDataDeclExtraArray& host, void* param)
{
	// Type Check size expressions.
	RecursiveVisitor::caseDataDeclExtraArray(host);
	if (breakRecursion(host)) return;
	if(!registered(host, host.dimensions)) return;
	
	// Iterate over sizes.
	for (vector<ASTExpr*>::const_iterator it = host.dimensions.begin();
		 it != host.dimensions.end(); ++it)
	{
		ASTExpr& size = **it;

		// Make sure each size can cast to float.
		if (!size.getReadType(scope, this)->canCastTo(DataType::FLOAT))
		{
			handleError(CompileError::NonIntegerArraySize(&host));
			return;
		}

		// Make sure that the size is constant.
		if (!size.getCompileTimeValue(this, scope))
		{
			handleError(CompileError::ExprNotConstant(&host));
			return;
		}
		
		if(optional<long> theSize = size.getCompileTimeValue(this, scope))
		{
			if(*theSize % 10000)
			{
				handleError(CompileError::ArrayDecimal(&host));
			}
			theSize = (*theSize / 10000);
			if(*theSize < 1 || *theSize > 214748)
			{
				handleError(CompileError::ArrayInvalidSize(&host));
				return;
			}
		}
	}
	doRegister(host);
}

void RegistrationVisitor::caseFuncDecl(ASTFuncDecl& host, void* param)
{
	if(host.getFlag(FUNCFLAG_INVALID))
	{
		handleError(CompileError::BadFuncModifiers(&host, host.invalidMsg));
		return;
	}
	/* This option is being disabled for now, as inlining of user functions is being disabled -V
	if(*lookupOption(*scope, CompileOption::OPT_FORCE_INLINE)
		&& !host.isRun())
	{
		host.setFlag(FUNCFLAG_INLINE);
	}*/
	// Resolve the return type under current scope.
	DataType const& returnType = host.returnType->resolve(*scope, this);
	if (breakRecursion(*host.returnType.get())) return;
	if (!returnType.isResolved()) return;

	// Gather the parameter types.
	vector<DataType const*> paramTypes;
	vector<ASTDataDecl*> const& params = host.parameters.data();
	for (vector<ASTDataDecl*>::const_iterator it = params.begin();
		 it != params.end(); ++it)
	{
		ASTDataDecl& decl = **it;

		// Resolve the parameter type under current scope.
		DataType const& type = *decl.resolveType(scope, this);
		if (breakRecursion(decl)) return;
		if (!type.isResolved()) return;

		// Don't allow void params.
		if (type == DataType::ZVOID)
		{
			handleError(CompileError::FunctionVoidParam(&decl, decl.name));
			return;
		}

		paramTypes.push_back(&type);
	}

	// Add the function to the scope.
	Function* function = scope->addFunction(
			&returnType, host.name, paramTypes, host.getFlags(), &host);
	host.func = function;

	// If adding it failed, it means this scope already has a function with
	// that name.
	if (function == NULL)
	{
		handleError(CompileError::FunctionRedef(&host, host.name));
		return;
	}

	function->node = &host;
	doRegister(host);
}

// Expressions -- Needed for constant evaluation
void RegistrationVisitor::caseExprConst(ASTExprConst& host, void* param)
{
	RecursiveVisitor::caseExprConst(host, param);
	if (host.getCompileTimeValue(this, scope)) doRegister(host);
}

void RegistrationVisitor::caseExprAssign(ASTExprAssign& host, void* param)
{
	visit(host.left.get(), paramWrite);
	if (breakRecursion(host)) return;
	visit(host.right.get(), paramRead);
	if (breakRecursion(host)) return;	
	if(!(registered(host.left.get()) && registered(host.right.get()))) return;
	DataType const* ltype = host.left->getWriteType(scope, this);
	if (!ltype)
	{
		handleError(
			CompileError::NoWriteType(
				host.left.get(), host.left->asString()));
		return;
	}
	doRegister(host);
}

void RegistrationVisitor::caseExprIdentifier(ASTExprIdentifier& host, void* param)
{
	// Bind to named variable.
	host.binding = lookupDatum(*scope, host, this);
	if (!host.binding) return;

	// Can't write to a constant.
	if (param == paramWrite || param == paramReadWrite)
	{
		if (host.binding->type.isConstant())
		{
			handleError(CompileError::LValConst(&host, host.asString()));
			return;
		}
	}
	doRegister(host);
}

void RegistrationVisitor::caseExprArrow(ASTExprArrow& host, void* param)
{
	// Recurse on left.
	visit(host.left.get());
	syncDisable(host, *host.left);
    if (breakRecursion(host)) return;
	if(!registered(host.left.get())) return;

	// Grab the left side's class.
	DataTypeClass const* leftType = dynamic_cast<DataTypeClass const*>(
			&getNaiveType(*host.left->getReadType(scope, this), scope));
	if(!host.leftClass)
	{
		if (!leftType)
		{
			handleError(CompileError::ArrowNotPointer(&host));
			return;
		}
		host.leftClass = program.getTypeStore().getClass(leftType->getClassId());
	}

	// Find read function.
	if (!host.readFunction && (!param || param == paramRead || param == paramReadWrite))
	{
		host.readFunction = lookupGetter(*host.leftClass, host.right);
		if (!host.readFunction)
		{
			handleError(
					CompileError::ArrowNoVar(
							&host,
							host.right + (host.index ? "[]" : "")));
			return;
		}
		vector<DataType const*>& paramTypes = host.readFunction->paramTypes;
		if (paramTypes.size() != (host.index ? 2 : 1) || *paramTypes[0] != *leftType)
		{
			handleError(
					CompileError::ArrowNoVar(
							&host,
							host.right + (host.index ? "[]" : "")));
			return;
		}
	}

	// Find write function.
	if (!host.writeFunction && (param == paramWrite || param == paramReadWrite))
	{
		host.writeFunction = lookupSetter(*host.leftClass, host.right);
		if (!host.writeFunction)
		{
			handleError(
					CompileError::ArrowNoVar(
							&host,
							host.right + (host.index ? "[]" : "")));
			return;
		}
		vector<DataType const*>& paramTypes = host.writeFunction->paramTypes;
		if (paramTypes.size() != (host.index ? 3 : 2)
		    || *paramTypes[0] != *leftType)
		{
			handleError(
					CompileError::ArrowNoVar(
							&host,
							host.right + (host.index ? "[]" : "")));
			return;
		}
	}

	if (host.index)
	{
		visit(host.index.get());
        if (breakRecursion(host)) return;
		if(!registered(host.index.get())) return;
    }
	doRegister(host);
}

void RegistrationVisitor::caseExprIndex(ASTExprIndex& host, void* param)
{
	visit(host.array.get());
	syncDisable(host, *host.array);
	if (breakRecursion(host)) return;
	visit(host.index.get());
	syncDisable(host, *host.index);
	if (breakRecursion(host)) return;
	if(registered(host.array.get()) && registered(host.index.get())) doRegister(host);
}

void RegistrationVisitor::caseExprCall(ASTExprCall& host, void* param)
{
	handleError(CompileError::GlobalVarFuncCall(&host));
}

void RegistrationVisitor::caseExprNegate(ASTExprNegate& host, void* param)
{
	analyzeUnaryExpr(host);
}

void RegistrationVisitor::caseExprNot(ASTExprNot& host, void* param)
{
	analyzeUnaryExpr(host);
}

void RegistrationVisitor::caseExprBitNot(ASTExprBitNot& host, void* param)
{
	analyzeUnaryExpr(host);
}

void RegistrationVisitor::caseExprIncrement(ASTExprIncrement& host, void* param)
{
	analyzeUnaryExpr(host);
}

void RegistrationVisitor::caseExprPreIncrement(ASTExprPreIncrement& host, void* param)
{
	analyzeUnaryExpr(host);
}

void RegistrationVisitor::caseExprDecrement(ASTExprDecrement& host, void* param)
{
	analyzeUnaryExpr(host);
}

void RegistrationVisitor::caseExprPreDecrement(ASTExprPreDecrement& host, void* param)
{
	analyzeUnaryExpr(host);
}

void RegistrationVisitor::caseExprCast(ASTExprCast& host, void* param)
{
	analyzeUnaryExpr(host);
}

void RegistrationVisitor::caseExprAnd(ASTExprAnd& host, void* param)
{
	analyzeBinaryExpr(host);
}

void RegistrationVisitor::caseExprOr(ASTExprOr& host, void* param)
{
	analyzeBinaryExpr(host);
}

void RegistrationVisitor::caseExprGT(ASTExprGT& host, void* param)
{
	analyzeBinaryExpr(host);
}

void RegistrationVisitor::caseExprGE(ASTExprGE& host, void* param)
{
	analyzeBinaryExpr(host);
}

void RegistrationVisitor::caseExprLT(ASTExprLT& host, void* param)
{
	analyzeBinaryExpr(host);
}

void RegistrationVisitor::caseExprLE(ASTExprLE& host, void* param)
{
	analyzeBinaryExpr(host);
}

void RegistrationVisitor::caseExprEQ(ASTExprEQ& host, void* param)
{
	analyzeBinaryExpr(host);
}

void RegistrationVisitor::caseExprNE(ASTExprNE& host, void* param)
{
	analyzeBinaryExpr(host);
}

void RegistrationVisitor::caseExprPlus(ASTExprPlus& host, void* param)
{
	analyzeBinaryExpr(host);
}

void RegistrationVisitor::caseExprMinus(ASTExprMinus& host, void* param)
{
	analyzeBinaryExpr(host);
}

void RegistrationVisitor::caseExprTimes(ASTExprTimes& host, void* param)
{
	analyzeBinaryExpr(host);
}

void RegistrationVisitor::caseExprDivide(ASTExprDivide& host, void* param)
{
	analyzeBinaryExpr(host);
}

void RegistrationVisitor::caseExprModulo(ASTExprModulo& host, void* param)
{
	analyzeBinaryExpr(host);
}

void RegistrationVisitor::caseExprBitAnd(ASTExprBitAnd& host, void* param)
{
	analyzeBinaryExpr(host);
}

void RegistrationVisitor::caseExprBitOr(ASTExprBitOr& host, void* param)
{
	analyzeBinaryExpr(host);
}

void RegistrationVisitor::caseExprBitXor(ASTExprBitXor& host, void* param)
{
	analyzeBinaryExpr(host);
}

void RegistrationVisitor::caseExprLShift(ASTExprLShift& host, void* param)
{
	analyzeBinaryExpr(host);
}

void RegistrationVisitor::caseExprRShift(ASTExprRShift& host, void* param)
{
	analyzeBinaryExpr(host);
}

void RegistrationVisitor::caseExprTernary(ASTTernaryExpr& host, void* param)
{
	visit(host.left.get());
	syncDisable(host, *host.left);
	if (breakRecursion(host)) return;
	visit(host.middle.get());
	syncDisable(host, *host.middle);
	if (breakRecursion(host)) return;
	visit(host.right.get());
	syncDisable(host, *host.right);
	if (breakRecursion(host)) return;
	if(registered(host.left.get()) && registered(host.middle.get()) && registered(host.right.get())) doRegister(host);
}

//Types
void RegistrationVisitor::caseScriptType(ASTScriptType& host, void* param)
{
	ScriptType const& type = resolveScriptType(host, *scope);
	if(type.isValid()) doRegister(host);
}

void RegistrationVisitor::caseDataType(ASTDataType& host, void* param)
{
	DataType const& type = host.resolve(*scope, this);
	if(type.isResolved()) doRegister(host);
}

//Literals
void RegistrationVisitor::caseArrayLiteral(ASTArrayLiteral& host, void* param)
{
	RecursiveVisitor::caseArrayLiteral(host, param);
	if(registered(host.type.get()) && registered(host.size.get()) && registered(host, host.elements))
		doRegister(host);
}

void RegistrationVisitor::caseStringLiteral(ASTStringLiteral& host, void* param)
{
	// Add to scope as a managed literal.
	Literal::create(*scope, host, *host.getReadType(scope, this), this);
	doRegister(host);
}

//Helper Functions
void RegistrationVisitor::analyzeUnaryExpr(ASTUnaryExpr& host)
{
	visit(host.operand.get());
	syncDisable(host, *host.operand);
	if (breakRecursion(host)) return;
	if(registered(host.operand.get()))doRegister(host);
}

void RegistrationVisitor::analyzeBinaryExpr(ASTBinaryExpr& host)
{
	visit(host.left.get());
	syncDisable(host, *host.left);
	if (breakRecursion(host)) return;
	visit(host.right.get());
	syncDisable(host, *host.right);
	if (breakRecursion(host)) return;
	if((registered(host.left.get()) && registered(host.right.get()))) doRegister(host);
}

bool RegistrationVisitor::registered(AST& node) const
{
	return node.registered();
}

bool RegistrationVisitor::registered(AST* node) const
{
	if(node) return registered(*node);
	return true;
}

template <class Container>
bool RegistrationVisitor::registered(AST& host, Container const& nodes) const
{
	for(typename Container::const_iterator it = nodes.begin();
		it != nodes.end(); ++it)
	{
		if(!registered(*it)) return false;
	}
	return true;
}


