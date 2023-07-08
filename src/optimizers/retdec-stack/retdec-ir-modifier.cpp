/**
 * @file src/bin2llvmir/utils/ir_modifier.cpp
 * @brief Modify both LLVM IR and config.
 * @copyright (c) 2017 Avast Software, licensed under the MIT license
 */

#include <list>

#include <llvm/IR/InstIterator.h>


// #include "retdec/utils/string.h"
// #include "retdec/bin2llvmir/providers/abi/abi.h"
// #include "retdec/bin2llvmir/providers/asm_instruction.h"
// #include "retdec/bin2llvmir/providers/names.h"
// #include "retdec/bin2llvmir/utils/debug.h"
#include "optimizers/retdec-stack/retdec-ir-modifier.h"
// #include "retdec/bin2llvmir/utils/llvm.h"

using namespace llvm;

//
//==============================================================================
// Local functions.
//==============================================================================
//
namespace retdec {
namespace bin2llvmir {

using namespace retdec::bin2llvmir;

Instruction* insertBeforeAfter(Instruction* i, Instruction* b, Instruction* a)
{
	if (b)
	{
		i->insertBefore(b);
	}
	else
	{
		i->insertAfter(a);
	}
	return i;
}

Value* convertToType(
		Value* val,
		Type* type,
		Instruction* before,
		Instruction* after,
		bool constExpr)
{
	if (val == nullptr
			|| type == nullptr
			|| (!constExpr && before == nullptr && after == nullptr))
	{
		return nullptr;
	}

	auto* cval = dyn_cast<Constant>(val);
	if (constExpr)
	{
		assert(cval);
	}

	auto& ctx = type->getContext();
	Value* conv = nullptr;

	if (val->getType() == type)
	{
		conv = val;
	}
	else if (val->getType()->isPointerTy() && type->isPointerTy())
	{
		if (constExpr)
		{
			if (val->getType()->getPointerAddressSpace() == type->getPointerAddressSpace())
				conv = ConstantExpr::getBitCast(cval, type);
			else
				conv = ConstantExpr::getAddrSpaceCast(cval, type);
		}
		else
		{
			if (val->getType()->getPointerAddressSpace() == type->getPointerAddressSpace())
			{
				auto* i = new BitCastInst(val, type, "");
				conv = insertBeforeAfter(i, before, after);
			}
			else
			{
				auto* i = new AddrSpaceCastInst(val, type, "");
				conv = insertBeforeAfter(i, before, after);
			}
		}
	}
	else if (val->getType()->isPointerTy() && type->isIntegerTy())
	{
		if (constExpr)
		{
			conv = ConstantExpr::getPtrToInt(cval, type);
		}
		else
		{
			auto* i = new PtrToIntInst(val, type, "");
			conv = insertBeforeAfter(i, before, after);
		}
	}
	else if (val->getType()->isIntegerTy() && type->isPointerTy())
	{
		if (constExpr)
		{
			conv = ConstantExpr::getIntToPtr(cval, type);
		}
		else
		{
			auto* i = new IntToPtrInst(val, type, "");
			conv = insertBeforeAfter(i, before, after);
		}
	}
	else if (val->getType()->isIntegerTy() && type->isIntegerTy())
	{
		if (constExpr)
		{
			conv = ConstantExpr::getIntegerCast(cval, type, true);
		}
		else
		{
			auto* i = CastInst::CreateIntegerCast(val, type, true, "");
			conv = insertBeforeAfter(i, before, after);
		}
	}
	else if (val->getType()->isIntegerTy() && type->isFloatingPointTy())
	{
		auto* toInt = Type::getIntNTy(ctx, type->getPrimitiveSizeInBits());
		auto* szConv = convertToType(val, toInt, before, after, constExpr);

		if (constExpr)
		{
			conv = ConstantExpr::getBitCast(cast<Constant>(szConv), type);
		}
		else
		{
			auto* i = new BitCastInst(szConv, type, "");
			auto* a = val == szConv ? after : cast<Instruction>(szConv);
			conv = insertBeforeAfter(i, before, a);
		}
	}
	else if (val->getType()->isPointerTy() && type->isFloatingPointTy())
	{
		auto* toInt = Type::getIntNTy(ctx, type->getPrimitiveSizeInBits());
		auto* intConv = convertToType(val, toInt, before, after, constExpr);
		auto* a = dyn_cast<Instruction>(intConv);
		conv = convertToType(intConv, type, before, a, constExpr);
	}
	else if (val->getType()->isFloatingPointTy() && type->isIntegerTy())
	{
		Type* ft = nullptr;
		IntegerType* intT = cast<IntegerType>(type);
		switch (intT->getBitWidth())
		{
			case 16: ft = Type::getHalfTy(ctx); break;
			case 32: ft = Type::getFloatTy(ctx); break;
			case 64: ft = Type::getDoubleTy(ctx); break;
			case 80: ft = Type::getX86_FP80Ty(ctx); break;
			default:
			{
				auto* fpConv = convertToType(
						val,
						Type::getInt32Ty(ctx),
						before,
						after,
						constExpr);
				auto* a = dyn_cast<Instruction>(fpConv);
				conv = convertToType(fpConv, intT, before, a, constExpr);
				return conv;
			}
		}

		if (val->getType() != ft)
		{
			auto* fpConv = convertToType(val, ft, before, after, constExpr);
			auto* a = dyn_cast<Instruction>(fpConv);
			conv = convertToType(fpConv, intT, before, a, constExpr);
		}
		else
		{
			if (constExpr)
			{
				conv = ConstantExpr::getBitCast(cval, intT);
			}
			else
			{
				auto* i = new BitCastInst(val, intT, "");
				conv = insertBeforeAfter(i, before, after);
			}
		}
	}
	else if (val->getType()->isFloatingPointTy() && type->isPointerTy())
	{
		auto* toInt = Type::getIntNTy(
				ctx,
				val->getType()->getPrimitiveSizeInBits());
		auto* intConv = convertToType(val, toInt, before, after, constExpr);
		auto* a = dyn_cast<Instruction>(intConv);
		conv = convertToType(intConv, type, before, a, constExpr);
	}
	else if (val->getType()->isFloatingPointTy() && type->isFloatingPointTy())
	{
		if (constExpr)
		{
			conv = ConstantExpr::getFPCast(cval, type);
		}
		else
		{
			auto* i = CastInst::CreateFPCast(val, type, "");
			conv = insertBeforeAfter(i, before, after);
		}
	}
	// TODO: this is too late, it would be the best if loads/stores that
	// load/store entire aggregate types were not created at all.
	// Such complex load/stores are not possible at ASM level.
	// Something like util function createSafe{Load,Store}() that would
	// check if loaded/stored value is not aggregate and if it is, it would
	// do the same this as here.
	//
	else if (isa<LoadInst>(val) && val->getType()->isAggregateType() && !constExpr)
	{
		auto* l = cast<LoadInst>(val);
		auto* c = cast<Instruction>(convertToType(
				l->getPointerOperand(),
				PointerType::get(type, 0),
				before,
				after,
				constExpr));
		auto* nl = new LoadInst(c->getType(), c, "", c->getNextNonDebugInstruction());
		// nl->insertAfter(c);
		conv = nl;
	}
	else if (val->getType()->isAggregateType())
	{
		std::vector<unsigned> idxs = { 0 };
		Value* toSimple = nullptr;
		if (constExpr)
		{
			toSimple = ConstantExpr::getExtractValue(
					cval,
					ArrayRef<unsigned>(idxs));
		}
		else
		{
			auto* i = ExtractValueInst::Create(
					val,
					ArrayRef<unsigned>(idxs),
					"");
			toSimple = insertBeforeAfter(i, before, after);
		}
		auto* a = dyn_cast<Instruction>(toSimple);
		conv = convertToType(toSimple, type, before, a, constExpr);
	}
	// else if (val->getType()->isVoidTy())
	// {
	// 	auto* module = before ? before->getModule() : after->getModule();
	// 	auto* config = ConfigProvider::getConfig(module);
	// 	auto* dummy = config->getGlobalDummy();
	// 	conv = convertToType(dummy, type, before, after, constExpr);
	// }
	else
	{
		errs() << "\nconvertValueToType(): unhandled type conversion\n";
		errs() << *val << "\n";
		errs() << *val->getType() << "\n";
		errs() << *type << "\n\n";
		assert(false);
		conv = nullptr;
	}

	return conv;
}

Value* IrModifier::convertValueToType(Value* val, Type* type, Instruction* before)
{
	return convertToType(val, type, before, nullptr, false);
}

void _eraseUnusedInstructionRecursive(
		const std::unordered_set<llvm::Value*>& workset)
{
	std::list<llvm::Instruction*> toEraseList;
	std::unordered_set<llvm::Value*> toEraseSet;
	std::list<llvm::Value*> worklist(workset.begin(), workset.end());

	for (auto it = worklist.begin(); it != worklist.end(); ++it)
	{
		if (auto* i = dyn_cast<llvm::Instruction>(*it))
		{
			// Do not erase instructions with side effects.
			//
			if (auto* call = llvm::dyn_cast<CallInst>(i))
			{
				auto* cf = call->getCalledFunction();
				if (!(cf && cf->isIntrinsic()))
				{
					continue;
				}
			}

			bool erase = i->user_empty();
			if (!erase)
			{
				erase = true;
				for (auto* u : i->users())
				{
					if (toEraseSet.count(u) == 0)
					{
						erase = false;
						break;
					}
				}
			}

			if (erase && toEraseSet.count(i) == 0)
			{
				toEraseList.push_back(i);
				toEraseSet.insert(i);
				for (auto* op : i->operand_values())
				{
					worklist.push_back(op);
				}
			}
		}
	}

	for (auto* i : toEraseList)
	{
		i->eraseFromParent();
	}
}

/**
 * Erase @a insn if it is an unused instruction, and also all its operands
 * that are also unused instructions.
 */
void IrModifier::eraseUnusedInstructionRecursive(llvm::Value* insn)
{
	_eraseUnusedInstructionRecursive({insn});
}

void IrModifier::eraseUnusedInstructionsRecursive(
		std::unordered_set<llvm::Value*>& insns)
{
	_eraseUnusedInstructionRecursive(insns);
}

} // namespace bin2llvmir
} // namespace retdec
