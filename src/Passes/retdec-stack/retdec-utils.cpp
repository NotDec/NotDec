#include <llvm/IR/CFG.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Value.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/Casting.h>


/**
 * We need special function for @c Module printing because
 * its @c print method takes one more parameter.
 * @param t Module to print.
 * @return String with printed module.
 */
std::string llvmObjToString(const llvm::Module* t)
{
	std::string str;
	llvm::raw_string_ostream ss(str);
	if (t)
		t->print(ss, nullptr);
	else
		ss << "nullptr";
	return ss.str();
}
