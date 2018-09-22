#ifndef ZSCRIPT_ZSCRIPT_H
#define ZSCRIPT_ZSCRIPT_H

#include <vector>
#include <map>
#include "AST.h"
#include "CompilerUtils.h"
#include "Types.h"

namespace ZScript
{
	////////////////////////////////////////////////////////////////
	// Forward Declarations
	
	// from CompileError.h
	class CompileErrorHandler;

	// from Scope.h
	class Scope;
	class RootScope;
	class ScriptScope;
	class FunctionScope;
	class ClassScope;
	
	// local
	class Program;
	class Script;
	class Variable;
	class BuiltinVariable;
	class Function;

	////////////////////////////////////////////////////////////////
	// Program
	
	class Program : private NoCopy
	{
	public:
		Program(ASTFile&, CompileErrorHandler*);
		~Program();

		ASTFile& getRoot() {return root_;}
		RootScope& getScope() const {return *rootScope_;}

		std::vector<Script*> scripts;
		Script* getScript(std::string const& name) const;
		Script* getScript(ASTScript* node) const;
		Script* addScript(ASTScript&, Scope&, CompileErrorHandler*);

		// Gets the non-internal (user-defined) global scope functions.
		std::vector<Function*> getUserGlobalFunctions() const;

		// Gets all user-defined functions.
		std::vector<Function*> getUserFunctions() const;

		// Return a list of all errors in the script declaration.
		std::vector<CompileError const*> getErrors() const;
		// Does this script have a declaration error?
		bool hasError() const {return getErrors().size();}

	private:
		std::map<std::string, Script*> scriptsByName_;
		std::map<ASTScript*, Script*> scriptsByNode_;

		RootScope* rootScope_;
		ASTFile& root_;
	};

	// Gets all defined functions.
	std::vector<Function*> getFunctions(Program const&);

	////////////////////////////////////////////////////////////////
	// Script

	class UserScript;
	class BuiltinScript;
	
	class Script
	{
	public:
		virtual ~Script();

		virtual ScriptType getType() const = 0;
		virtual std::string const& getName() const = 0;
		virtual ASTScript* getNode() const = 0;
		virtual ScriptScope& getScope() = 0;
		virtual ScriptScope const& getScope() const = 0;

		std::vector<Opcode*> code;

	protected:
		Script(Program& program);

	private:
		Program& program;
	};

	class UserScript : public Script
	{
		friend UserScript* createScript(
				Program&, Scope&, ASTScript&, CompileErrorHandler*);

	public:
		ScriptType getType() const /*override*/;
		std::string const& getName() const /*override*/ {return node.name;};
		ASTScript* getNode() const /*override*/ {return &node;};
		ScriptScope& getScope() /*override*/ {return *scope;}
		ScriptScope const& getScope() const /*override*/ {return *scope;}

	private:
		UserScript(Program&, ASTScript&);

		ASTScript& node;
		ScriptScope* scope;
	};

	class BuiltinScript : public Script
	{
		friend BuiltinScript* createScript(
				Program&, Scope&, ScriptType, std::string const& name,
				CompileErrorHandler*);

	public:
		ScriptType getType() const /*override*/ {return type;}
		std::string const& getName() const /*override*/ {return name;};
		ASTScript* getNode() const /*override*/ {return NULL;};
		ScriptScope& getScope() /*override*/ {return *scope;}
		ScriptScope const& getScope() const /*override*/ {return *scope;}
		
	private:
		BuiltinScript(Program&, ScriptType, std::string const& name);
		
		ScriptType type;
		std::string name;
		ScriptScope* scope;
	};

	UserScript* createScript(
			Program&, Scope&, ASTScript&, CompileErrorHandler* = NULL);
	BuiltinScript* createScript(
			Program&, Scope&, ScriptType, std::string const& name,
			CompileErrorHandler* = NULL);
	
	Function* getRunFunction(Script const&);
	optional<int> getLabel(Script const&);

	////////////////////////////////////////////////////////////////
	// Datum
	
	// Something that can be resolved to a data value.
	class Datum
	{
	public:
		// The containing scope.
		Scope& scope;

		// The type of this data.
		DataType type;

		// Id for lookup tables.
		int const id;

		// Get the data's name.
		virtual optional<std::string> getName() const {return nullopt;}
		
		// Get the value at compile time.
		virtual optional<long> getCompileTimeValue() const {return nullopt;}

		// Get the declaring node.
		virtual AST* getNode() const {return NULL;}
		
		// Get the global register this uses.
		virtual optional<int> getGlobalId() const {return nullopt;}
		
	protected:
		Datum(Scope& scope, DataType type);

		// Call in static creation function to register with scope.
		bool tryAddToScope(CompileErrorHandler* = NULL);
	};

	// Is this datum a global value?
	bool isGlobal(Datum const& data);

	// Return the stack offset of the value.
	optional<int> getStackOffset(Datum const&);

	// A literal value that requires memory management.
	class Literal : public Datum
	{
	public:
		static Literal* create(
				Scope&, ASTLiteral&, DataType const&,
				CompileErrorHandler* = NULL);
		
		ASTLiteral* getNode() const {return &node;}

	private:
		Literal(Scope& scope, ASTLiteral& node, DataType const& type);

		ASTLiteral& node;
	};

	// A variable.
	class Variable : public Datum
	{
	public:
		static Variable* create(
				Scope&, ASTDataDecl&, DataType const&,
				CompileErrorHandler* = NULL);

		optional<std::string> getName() const {return node.name;}
		ASTDataDecl* getNode() const {return &node;}
		optional<int> getGlobalId() const {return globalId;}

	private:
		Variable(Scope& scope, ASTDataDecl& node, DataType const& type);

		ASTDataDecl& node;
		optional<int> globalId;
	};

	// A compiler generated variable.
	class BuiltinVariable : public Datum
	{
	public:
		static BuiltinVariable* create(
				Scope&, DataType const&, std::string const& name,
				CompileErrorHandler* = NULL);

		optional<std::string> getName() const {return name;}
		optional<int> getGlobalId() const {return globalId;}

	private:
		BuiltinVariable(Scope&, DataType const&, std::string const& name);

		std::string const name;
		optional<int> globalId;
	};

	// An inlined constant.
	class Constant : public Datum
	{
	public:
		static Constant* create(
				Scope&, ASTDataDecl&, DataType const&, long value,
				CompileErrorHandler* = NULL);

		optional<std::string> getName() const;

		optional<long> getCompileTimeValue() const {return value;}

		ASTDataDecl* getNode() const {return &node;}
	
	private:
		Constant(Scope&, ASTDataDecl&, DataType const&, long value);

		ASTDataDecl& node;
		long value;
	};

	// A builtin data value.
	class BuiltinConstant : public Datum
	{
	public:
		static BuiltinConstant* create(
				Scope&, DataType const&, std::string const& name, long value,
				CompileErrorHandler* = NULL);

		optional<std::string> getName() const {return name;}
		optional<long> getCompileTimeValue() const {return value;}

	private:
		BuiltinConstant(Scope&, DataType const&,
		                std::string const& name, long value);

		std::string name;
		long value;
	};

	////////////////////////////////////////////////////////////////
	// FunctionSignature

	// Comparable signature structure. Value semantics.
	class FunctionSignature
	{
	public:
		FunctionSignature(
				std::string const& name,
				std::vector<DataType> const& parameterTypes);
		FunctionSignature(Function const& function);

		// As per <=> operator.
		int compare(FunctionSignature const& rhs) const;

		std::string asString() const;
		operator std::string() const {return asString();}

		std::string name;
		std::vector<DataType> parameterTypes;
	};
	
	bool operator==(
			FunctionSignature const& lhs, FunctionSignature const& rhs);
	bool operator!=(
			FunctionSignature const& lhs, FunctionSignature const& rhs);
	bool operator<=(
			FunctionSignature const& lhs, FunctionSignature const& rhs);
	bool operator<(
			FunctionSignature const& lhs, FunctionSignature const& rhs);
	bool operator>=(
			FunctionSignature const& lhs, FunctionSignature const& rhs);
	bool operator>(
			FunctionSignature const& lhs, FunctionSignature const& rhs);
	
	////////////////////////////////////////////////////////////////
	// Function
	
	class Function
	{
	public:
		
		Function(DataType returnType, std::string const& name,
		         std::vector<DataType> const& paramTypes, int id);
		~Function();
		
		// Get the opcodes.
		std::vector<Opcode*> const& getCode() const {return ownedCode_;}
		// Get and remove the code for this function.
		std::vector<Opcode*> takeCode();
		// Add code for this function, transferring ownership.
		// Clears the input vector.
		void giveCode(std::vector<Opcode*>& code);
		
		FunctionSignature getSignature() const {
			return FunctionSignature(*this);}
		
		// If this is a script level function, return that script.
		Script* getScript() const;

		int getLabel() const;
		void clearLabel();

		// If this is a tracing function (enabled by #option trace)
		bool isTracing() const;

		// Data
		ASTFuncDecl* node;
		FunctionScope* internalScope;
		BuiltinVariable* thisVar;
		DataType returnType;
		std::string name;
		std::vector<DataType> paramTypes;
		int id;

	private:
		mutable optional<int> label_;

		// Code implementing this function.
		std::vector<Opcode*> ownedCode_;
	};

	// Is this function a "run" function?
	bool isRun(Function const&);

	// Get the size of the function stack.
	int getStackSize(Function const&);

	// Get the function's parameter count, including "this" if present.
	int getParameterCount(Function const&);

	////////////////////////////////////////////////////////////////
	// ZClass

	class ZClass
	{
	public:
		// Ids for the builtin classes.
		enum StdId
		{
#			define X(NAME, TYPE) \
			stdId##NAME,
#			include "classes.xtable"
#			undef X
			stdIdCount
		};
		
		static void generateStandard();
		static void clearStandard();
		static ZClass* getStandard(StdId id);
		
		ZClass(std::string const& name, DataType const& type);
		~ZClass();

		ClassScope const& getScope() const {return *scope_;}

		std::string const name;
		
	private:
		// Owned by the root scope.
		static std::vector<ZClass*> std_;
		
		ClassScope* scope_;
	};
}

#endif
