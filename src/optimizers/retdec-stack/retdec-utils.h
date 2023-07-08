#include <llvm/IR/Module.h>
#include <llvm/Support/raw_ostream.h>

/**
 * Print any LLVM object which implements @c print(llvm::raw_string_ostream&)
 * method into std::string.
 * @param t LLVM object to print.
 * @return String with printed object.
 */
template<typename T>
std::string llvmObjToString(const T* t)
{
	std::string str;
	llvm::raw_string_ostream ss(str);
	if (t)
		t->print(ss);
	else
		ss << "nullptr";
	return ss.str();
}
std::string llvmObjToString(const llvm::Module* t);
