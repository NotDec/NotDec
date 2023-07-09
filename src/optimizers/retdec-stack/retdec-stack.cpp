/**
* @file src/bin2llvmir/optimizations/stack/stack.cpp
* @brief Reconstruct stack.
* @copyright (c) 2017 Avast Software, licensed under the MIT license
* @copyright Modified by NotDec project
*/
#include <cstdlib>
#include <iostream>

#include <llvm/IR/Constants.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Operator.h>
#include <llvm/IR/PassManager.h>

#include "optimizers/retdec-stack/retdec-abi.h"
#include "optimizers/retdec-stack/retdec-stack.h"

// #include "retdec/bin2llvmir/analyses/reaching_definitions.h"
// #include "retdec/bin2llvmir/optimizations/stack/stack.h"
// #include "retdec/bin2llvmir/providers/asm_instruction.h"
#include "optimizers/retdec-stack/retdec-ir-modifier.h"

using namespace llvm;

namespace retdec {
namespace bin2llvmir {

PreservedAnalyses StackAnalysis::run(Module &M, ModuleAnalysisManager &AM) {
	_module = &M;
	if (_abi == nullptr) {
		std::cerr << __FILE__ << ":" << __LINE__ << ": " << "Error: ABI not set.";
		std::abort();
	}
	if (run()) {
		return PreservedAnalyses::none();
	} else {
		return PreservedAnalyses::all();
	}
}

bool StackAnalysis::runOnModuleCustom(
		llvm::Module& m,
		Abi* abi)
{
	_module = &m;
	_abi = abi;
	return run();
}

bool StackAnalysis::run()
{
	ReachingDefinitionsAnalysis RDA;
	RDA.runOnModule(*_module, _abi);

	for (auto& f : *_module)
	{
		// 从store load指令映射到intConstant，即栈偏移
		std::map<Value*, Value*> val2val;
		for (inst_iterator I = inst_begin(f), E = inst_end(f); I != E;)
		{
			Instruction& i = *I;
			++I;

			if (StoreInst *store = dyn_cast<StoreInst>(&i))
			{
				// if (AsmInstruction::isLlvmToAsmInstruction(store))
				// {
				// 	continue;
				// }

				handleInstruction(
						RDA,
						store,
						store->getValueOperand(),
						store->getValueOperand()->getType(),
						val2val);

				if (isa<GlobalVariable>(store->getPointerOperand()))
				{
					continue;
				}

				handleInstruction(
						RDA,
						store,
						store->getPointerOperand(),
						store->getValueOperand()->getType(),
						val2val);
			}
			else if (LoadInst* load = dyn_cast<LoadInst>(&i))
			{
				if (isa<GlobalVariable>(load->getPointerOperand()))
				{
					continue;
				}

				handleInstruction(
						RDA,
						load,
						load->getPointerOperand(),
						load->getType(),
						val2val);
			}
		}
	}

	IrModifier::eraseUnusedInstructionsRecursive(_toRemove);

	return false;
}

// 有三种情况
// 1. inst是store，val是store的val
// 2. inst是store，val是store的ptr
// 3. inst是load，val是load的ptr
// 而run函数基本就是转调这个函数
// LLVM里的指针都是带类型的，即带了指向的东西的大小和类型。所以只要看所有的栈指针的使用，就可以知道栈上的东西。
void StackAnalysis::handleInstruction(
		ReachingDefinitionsAnalysis& RDA,
		llvm::Instruction* inst,
		llvm::Value* val,
		llvm::Type* type,
		std::map<llvm::Value*, llvm::Value*>& val2val)
{
	std::cerr << __FILE__ << ":" << __LINE__ << ": " << llvmObjToString(inst) << std::endl;

	auto root = SymbolicTree::PrecomputedRdaWithValueMap(RDA, val, &val2val);
	std::cerr << __FILE__ << ":" << __LINE__ << ": " << root << std::endl;

	// TODO 如果把sp map到0，则相关栈操作数最终可以化简到常量。
	// 如果已经map了，则一般属于情况2和情况3
	// 如果没有映射，也没有SP，就直接报错返回。
	if (!root.isVal2ValMapUsed())
	{
		bool stackPtr = false;
		// 可能是什么表达式树。后序遍历判断是否包含栈寄存器。
		for (SymbolicTree* n : root.getPostOrder())
		{
			if (_abi->isStackPointerRegister(n->value))
			{
				stackPtr = true;
				break;
			}
		}
		if (!stackPtr)
		{
			std::cerr << __FILE__ << ":" << __LINE__ << ": " << "===> no SP" << std::endl;
			return;
		}
	}

	// 来自调试信息和来自用户配置的变量。
	// auto* debugSv = getDebugStackVariable(inst->getFunction(), root);
	// auto* configSv = getConfigStackVariable(inst->getFunction(), root);

	// 直接化简成stack offset
	root.simplifyNode();
	std::cerr << __FILE__ << ":" << __LINE__ << ": " << root << std::endl;

	// 简化后重新获取试试
	// if (debugSv == nullptr)
	// {
	// 	debugSv = getDebugStackVariable(inst->getFunction(), root);
	// }

	// if (configSv == nullptr)
	// {
	// 	configSv = getConfigStackVariable(inst->getFunction(), root);
	// }

	// 如果简化失败，就放弃
	auto* ci = dyn_cast_or_null<ConstantInt>(root.value);
	if (ci == nullptr)
	{
		return;
	}

	// 写入val2valmap，当前的store指令是访问的ci的offset处。
	if (auto* s = dyn_cast<StoreInst>(inst))
	{
		if (s->getValueOperand() == val)
		{
			val2val[inst] = ci;
		}
	}

	std::cerr << __FILE__ << ":" << __LINE__ << ": " << "===> " << llvmObjToString(ci) << std::endl;
	std::cerr << __FILE__ << ":" << __LINE__ << ": " << "===> " << ci->getSExtValue() << std::endl;

	std::string name = "";
	Type* t = type;

	// if (debugSv)
	// {
	// 	name = debugSv->getName();
	// 	t = llvm_utils::stringToLlvmTypeDefault(_module, debugSv->type.getLlvmIr());
	// }
	// else if (configSv)
	// {
	// 	name = configSv->getName();
	// 	t = llvm_utils::stringToLlvmTypeDefault(_module, configSv->type.getLlvmIr());
	// }

	std::string realName;
	// if (debugSv)
	// {
	// 	realName = debugSv->getName();
	// }
	// else if (configSv)
	// {
	// 	realName = configSv->getName();
	// }

	// offset 输入IrModifier，创建alloca指令
	// IrModifier irModif(_module, _config);
	// auto p = irModif.getStackVariable(
	// 		inst->getFunction(),
	// 		ci->getSExtValue(),
	// 		t,
	// 		name,
	// 		realName,
	// 		debugSv || configSv);

	// 改为直接创建Alloca指令。
	assert(inst->getFunction()->getInstructionCount() != 0);
	AllocaInst* a = new AllocaInst(t, 0, (name.empty() ? "stack_var_" : (name + "_"))
			+ std::to_string(ci->getSExtValue()), &*llvm::inst_begin(inst->getFunction()));

	std::cerr << __FILE__ << ":" << __LINE__ << ": " << "===> " << llvmObjToString(a) << std::endl;
	std::cerr << __FILE__ << ":" << __LINE__ << ": " << "===> " << llvmObjToString(inst) << std::endl;
	std::cerr << __FILE__ << ":" << __LINE__ << ": " << std::endl;

	auto* s = dyn_cast<StoreInst>(inst);
	auto* l = dyn_cast<LoadInst>(inst);
	// 当处理的是store指令，传入的是指针
	// 把store替换对新的store，ptr是新的alloca。
	if (s && s->getPointerOperand() == val)
	{
		auto* conv = IrModifier::convertValueToType(
				s->getValueOperand(),
				a->getType()->getPointerElementType(),
				inst);
		new StoreInst(conv, a, inst);
		_toRemove.insert(s);
	}
	// 当处理的是load指令，传入的是指针
	// 把相关的load替换为对alloca的load。
	else if (l && l->getPointerOperand() == val)
	{
		auto* nl = new LoadInst(a->getType(), a, "", l);
		auto* conv = IrModifier::convertValueToType(nl, l->getType(), l);
		l->replaceAllUsesWith(conv);
		_toRemove.insert(l);
	}
	// 当处理的是store指令，传入的是val？
	else
	{
		auto* conv = IrModifier::convertValueToType(a, val->getType(), inst);
		_toRemove.insert(val);
		inst->replaceUsesOfWith(val, conv);
	}
}

// 获取基于寄存器的偏移
std::optional<int> StackAnalysis::getBaseOffset(SymbolicTree& root)
{
	std::optional<int> baseOffset;
	if (auto* ci = dyn_cast_or_null<ConstantInt>(root.value))
	{
		baseOffset = ci->getSExtValue();
	}
	else
	{
		for (SymbolicTree* n : root.getLevelOrder())
		{
			// Load<Reg> + Constant，则返回常量
			if (isa<AddOperator>(n->value)
					&& n->ops.size() == 2
					&& isa<LoadInst>(n->ops[0].value)
					&& isa<ConstantInt>(n->ops[1].value))
			{
				auto* l = cast<LoadInst>(n->ops[0].value);
				auto* ci = cast<ConstantInt>(n->ops[1].value);
				if (_abi->isRegister(l->getPointerOperand()))
				{
					baseOffset = ci->getSExtValue();
				}
				break;
			}
		}
	}

	return baseOffset;
}

} // namespace bin2llvmir
} // namespace retdec
