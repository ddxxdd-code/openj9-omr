/*******************************************************************************
 * Copyright (c) 2018, 2022 IBM Corp. and others
 *
 * This program and the accompanying materials are made available under
 * the terms of the Eclipse Public License 2.0 which accompanies this
 * distribution and is available at http://eclipse.org/legal/epl-2.0
 * or the Apache License, Version 2.0 which accompanies this distribution
 * and is available at https://www.apache.org/licenses/LICENSE-2.0.
 *
 * This Source Code may also be made available under the following Secondary
 * Licenses when the conditions for such availability set forth in the
 * Eclipse Public License, v. 2.0 are satisfied: GNU General Public License,
 * version 2 with the GNU Classpath Exception [1] and GNU General Public
 * License, version 2 with the OpenJDK Assembly Exception [2].
 *
 * [1] https://www.gnu.org/software/classpath/license.html
 * [2] http://openjdk.java.net/legal/assembly-exception.html
 *
 * SPDX-License-Identifier: EPL-2.0 OR Apache-2.0 OR GPL-2.0 WITH Classpath-exception-2.0 OR LicenseRef-GPL-2.0 WITH Assembly-exception
 *******************************************************************************/

#ifndef GENERATE_INSTRUCTIONS_INCL
#define GENERATE_INSTRUCTIONS_INCL

#include <stddef.h>
#include <stdint.h>
#include "codegen/ARM64ConditionCode.hpp"
#include "codegen/ARM64Instruction.hpp"
#include "codegen/ARM64ShiftCode.hpp"
#include "codegen/InstOpCode.hpp"
#include "codegen/Instruction.hpp"
#include "env/jittypes.h"
#include "runtime/Runtime.hpp"

namespace TR { class CodeGenerator; }
namespace TR { class LabelSymbol; }
namespace TR { class MemoryReference; }
namespace TR { class Node; }
namespace TR { class Register; }
namespace TR { class RegisterDependencyConditions; }
namespace TR { class Snippet; }
namespace TR { class SymbolReference; }


/*
 * @brief Generates simple instruction
 * @param[in] cg : CodeGenerator
 * @param[in] op : instruction opcode
 * @param[in] node : node
 * @param[in] preced : preceding instruction
 * @return generated instruction
 */
TR::Instruction *generateInstruction(
                   TR::CodeGenerator *cg,
                   TR::InstOpCode::Mnemonic op,
                   TR::Node *node,
                   TR::Instruction *preced = NULL);

/*
 * @brief Generates imm instruction
 * @param[in] cg : CodeGenerator
 * @param[in] op : instruction opcode
 * @param[in] node : node
 * @param[in] imm : immediate value
 * @param[in] preced : preceding instruction
 * @return generated instruction
 */
TR::ARM64ImmInstruction *generateImmInstruction(
                   TR::CodeGenerator *cg,
                   TR::InstOpCode::Mnemonic op,
                   TR::Node *node,
                   uint32_t imm,
                   TR::Instruction *preced = NULL);

/*
 * @brief Generates relocatable imm instruction
 * @param[in] cg : CodeGenerator
 * @param[in] op : instruction opcode
 * @param[in] node : node
 * @param[in] imm : immediate value
 * @param[in] relocationKind : relocation kind
 * @param[in] preced : preceding instruction
 * @return generated instruction
 */
TR::Instruction *generateRelocatableImmInstruction(
                   TR::CodeGenerator *cg,
                   TR::InstOpCode::Mnemonic op,
                   TR::Node *node,
                   uintptr_t imm,
                   TR_ExternalRelocationTargetKind relocationKind,
                   TR::Instruction *preced = NULL);

/*
 * @brief Generates relocatable imm instruction
 * @param[in] cg : CodeGenerator
 * @param[in] op : instruction opcode
 * @param[in] node : node
 * @param[in] imm : immediate value
 * @param[in] relocationKind : relocation kind
 * @param[in] sr : symbol reference
 * @param[in] preced : preceding instruction
 * @return generated instruction
 */
TR::Instruction *generateRelocatableImmInstruction(
                   TR::CodeGenerator *cg,
                   TR::InstOpCode::Mnemonic op,
                   TR::Node *node,
                   uintptr_t imm,
                   TR_ExternalRelocationTargetKind relocationKind,
                   TR::SymbolReference *sr,
                   TR::Instruction *preced = NULL);

/*
 * @brief Generates imm sym instruction
 * @param[in] cg : CodeGenerator
 * @param[in] op : instruction opcode
 * @param[in] node : node
 * @param[in] imm : immediate value
 * @param[in] cond : register dependency condition
 * @param[in] sr : symbol reference
 * @param[in] s : call snippet
 * @param[in] preced : preceding instruction
 * @return generated instruction
 */
TR::Instruction *generateImmSymInstruction(
                   TR::CodeGenerator *cg,
                   TR::InstOpCode::Mnemonic op,
                   TR::Node *node,
                   uintptr_t imm,
                   TR::RegisterDependencyConditions *cond,
                   TR::SymbolReference *sr,
                   TR::Snippet *s,
                   TR::Instruction *preced = NULL);

/*
 * @brief Generates label instruction
 * @param[in] cg : CodeGenerator
 * @param[in] op : instruction opcode
 * @param[in] node : node
 * @param[in] sym : label symbol
 * @param[in] preced : preceding instruction
 * @return generated instruction
 */
TR::Instruction *generateLabelInstruction(
                   TR::CodeGenerator *cg,
                   TR::InstOpCode::Mnemonic op,
                   TR::Node *node,
                   TR::LabelSymbol *sym,
                   TR::Instruction *preced = NULL);

/*
 * @brief Generates label instruction with register dependency
 * @param[in] cg : CodeGenerator
 * @param[in] op : instruction opcode
 * @param[in] node : node
 * @param[in] sym : label symbol
 * @param[in] cond : register dependency condition
 * @param[in] preced : preceding instruction
 * @return generated instruction
 */
TR::Instruction *generateLabelInstruction(
                   TR::CodeGenerator *cg,
                   TR::InstOpCode::Mnemonic op,
                   TR::Node *node,
                   TR::LabelSymbol *sym,
                   TR::RegisterDependencyConditions *cond,
                   TR::Instruction *preced = NULL);

/*
 * @brief Generates conditional branch instruction
 * @param[in] cg : CodeGenerator
 * @param[in] op : instruction opcode
 * @param[in] node : node
 * @param[in] sym : label symbol
 * @param[in] cc : branch condition code
 * @param[in] preced : preceding instruction
 * @return generated instruction
 */
TR::Instruction *generateConditionalBranchInstruction(
                   TR::CodeGenerator *cg,
                   TR::InstOpCode::Mnemonic op,
                   TR::Node *node,
                   TR::LabelSymbol *sym,
                   TR::ARM64ConditionCode cc,
                   TR::Instruction *preced = NULL);

/*
 * @brief Generates conditional branch instruction with register dependency
 * @param[in] cg : CodeGenerator
 * @param[in] op : instruction opcode
 * @param[in] node : node
 * @param[in] sym : label symbol
 * @param[in] cc : branch condition code
 * @param[in] cond : register dependency condition
 * @param[in] preced : preceding instruction
 * @return generated instruction
 */
TR::Instruction *generateConditionalBranchInstruction(
                   TR::CodeGenerator *cg,
                   TR::InstOpCode::Mnemonic op,
                   TR::Node *node,
                   TR::LabelSymbol *sym,
                   TR::ARM64ConditionCode cc,
                   TR::RegisterDependencyConditions *cond,
                   TR::Instruction *preced = NULL);

/*
 * @brief Generates compare and branch instruction
 * @param[in] cg : CodeGenerator
 * @param[in] op : instruction opcode
 * @param[in] node : node
 * @param[in] sreg : source register
 * @param[in] sym : label symbol
 * @param[in] preced : preceding instruction
 * @return generated instruction
 */
TR::Instruction *generateCompareBranchInstruction(
                   TR::CodeGenerator *cg,
                   TR::InstOpCode::Mnemonic op,
                   TR::Node *node,
                   TR::Register *sreg,
                   TR::LabelSymbol *sym,
                   TR::Instruction *preced = NULL);

/*
 * @brief Generates compare and branch instruction
 * @param[in] cg : CodeGenerator
 * @param[in] op : instruction opcode
 * @param[in] node : node
 * @param[in] sreg : source register
 * @param[in] sym : label symbol
 * @param[in] cond : register dependency condition
 * @param[in] preced : preceding instruction
 * @return generated instruction
 */
TR::Instruction *generateCompareBranchInstruction(
                   TR::CodeGenerator *cg,
                   TR::InstOpCode::Mnemonic op,
                   TR::Node *node,
                   TR::Register *sreg,
                   TR::LabelSymbol *sym,
                   TR::RegisterDependencyConditions *cond,
                   TR::Instruction *preced = NULL);

/*
 * @brief Generates test bit and branch instruction
 * @param[in] cg : CodeGenerator
 * @param[in] op : instruction opcode
 * @param[in] node : node
 * @param[in] sreg : source register
 * @param[in] bitpos : bit position
 * @param[in] sym : label symbol
 * @param[in] cond : register dependency condition
 * @param[in] preced : preceding instruction
 * @return generated instruction
 */
TR::Instruction *generateTestBitBranchInstruction(
                   TR::CodeGenerator *cg,
                   TR::InstOpCode::Mnemonic op,
                   TR::Node *node,
                   TR::Register *sreg,
                   uint32_t bitpos,
                   TR::LabelSymbol *sym,
                   TR::RegisterDependencyConditions *cond,
                   TR::Instruction *preced = NULL);

/*
 * @brief Generates test bit and branch instruction
 * @param[in] cg : CodeGenerator
 * @param[in] op : instruction opcode
 * @param[in] node : node
 * @param[in] sreg : source register
 * @param[in] bitpos : bit position
 * @param[in] sym : label symbol
 * @param[in] preced : preceding instruction
 * @return generated instruction
 */
TR::Instruction *generateTestBitBranchInstruction(
                   TR::CodeGenerator *cg,
                   TR::InstOpCode::Mnemonic op,
                   TR::Node *node,
                   TR::Register *sreg,
                   uint32_t bitpos,
                   TR::LabelSymbol *sym,
                   TR::Instruction *preced = NULL);

/*
 * @brief Generates branch-to-register instruction
 * @param[in] cg : CodeGenerator
 * @param[in] op : instruction opcode
 * @param[in] node : node
 * @param[in] treg : target register
 * @param[in] preced : preceding instruction
 * @return generated instruction
 */
TR::Instruction *generateRegBranchInstruction(
                   TR::CodeGenerator *cg,
                   TR::InstOpCode::Mnemonic op,
                   TR::Node *node,
                   TR::Register *treg,
                   TR::Instruction *preced = NULL);

/*
 * @brief Generates branch-to-register instruction with register dependency
 * @param[in] cg : CodeGenerator
 * @param[in] op : instruction opcode
 * @param[in] node : node
 * @param[in] treg : target register
 * @param[in] cond : register dependency condition
 * @param[in] preced : preceding instruction
 * @return generated instruction
 */
TR::Instruction *generateRegBranchInstruction(
                   TR::CodeGenerator *cg,
                   TR::InstOpCode::Mnemonic op,
                   TR::Node *node,
                   TR::Register *treg,
                   TR::RegisterDependencyConditions *cond,
                   TR::Instruction *preced = NULL);

/*
 * @brief Generates admin instruction
 * @param[in] cg : CodeGenerator
 * @param[in] op : instruction opcode
 * @param[in] node : node
 * @param[in] fenceNode : fence node
 * @param[in] preced : preceding instruction
 * @return generated instruction
 */
TR::Instruction *generateAdminInstruction(
                   TR::CodeGenerator *cg,
                   TR::InstOpCode::Mnemonic op,
                   TR::Node *node,
                   TR::Node *fenceNode = NULL,
                   TR::Instruction *preced = NULL);

/*
 * @brief Generates admin instruction with register dependency
 * @param[in] cg : CodeGenerator
 * @param[in] op : instruction opcode
 * @param[in] node : node
 * @param[in] cond : register dependency condition
 * @param[in] fenceNode : fence node
 * @param[in] preced : preceding instruction
 * @return generated instruction
 */
TR::Instruction *generateAdminInstruction(
                   TR::CodeGenerator *cg,
                   TR::InstOpCode::Mnemonic op,
                   TR::Node *node,
                   TR::RegisterDependencyConditions *cond,
                   TR::Node *fenceNode = NULL,
                   TR::Instruction *preced = NULL);

/*
 * @brief Generates trg instruction
 * @param[in] cg : CodeGenerator
 * @param[in] op : instruction opcode
 * @param[in] node : node
 * @param[in] treg : target register
 * @param[in] preced : preceding instruction
 * @return generated instruction
 */
TR::Instruction *generateTrgInstruction(
                   TR::CodeGenerator *cg,
                   TR::InstOpCode::Mnemonic op,
                   TR::Node *node,
                   TR::Register *treg,
                   TR::Instruction *preced = NULL);

/*
 * @brief Generates imm-to-trg instruction
 * @param[in] cg : CodeGenerator
 * @param[in] op : instruction opcode
 * @param[in] node : node
 * @param[in] treg : target register
 * @param[in] imm : immediate value
 * @param[in] preced : preceding instruction
 * @return generated instruction
 */
TR::Instruction *generateTrg1ImmInstruction(
                   TR::CodeGenerator *cg,
                   TR::InstOpCode::Mnemonic op,
                   TR::Node *node,
                   TR::Register *treg,
                   uint32_t imm,
                   TR::Instruction *preced = NULL);

/*
 * @brief Generates shifted imm--to-trg instruction
 * @param[in] cg : CodeGenerator
 * @param[in] op : instruction opcode
 * @param[in] node : node
 * @param[in] treg : target register
 * @param[in] imm : immediate value
 * @param[in] shiftAmount : shift amount
 * @param[in] preced : preceding instruction
 * @return generated instruction
 */
TR::Instruction *generateTrg1ImmShiftedInstruction(
                   TR::CodeGenerator *cg,
                   TR::InstOpCode::Mnemonic op,
                   TR::Node *node,
                   TR::Register *treg,
                   uint32_t imm,
                   uint32_t shiftAmount,
                   TR::Instruction *preced = NULL);

/*
 * @brief Generates imm-to-trg label instruction
 * @param[in] cg : CodeGenerator
 * @param[in] op : instruction opcode
 * @param[in] node : node
 * @param[in] treg : target register
 * @param[in] imm : immediate value
 * @param[in] sym : symbol
 * @param[in] preced : preceding instruction
 * @return generated instruction
 */
TR::Instruction *generateTrg1ImmSymInstruction(
                   TR::CodeGenerator *cg,
                   TR::InstOpCode::Mnemonic op,
                   TR::Node *node,
                   TR::Register *treg,
                   uint32_t imm,
                   TR::Symbol *sym,
                   TR::Instruction *preced = NULL);

/*
 * @brief Generates src1-to-trg instruction
 * @param[in] cg : CodeGenerator
 * @param[in] op : instruction opcode
 * @param[in] node : node
 * @param[in] treg : target register
 * @param[in] s1reg : source register
 * @param[in] preced : preceding instruction
 * @return generated instruction
 */
TR::Instruction *generateTrg1Src1Instruction(
                   TR::CodeGenerator *cg,
                   TR::InstOpCode::Mnemonic op,
                   TR::Node *node,
                   TR::Register *treg,
                   TR::Register *s1reg,
                   TR::Instruction *preced = NULL);

/*
 * @brief Generates src1-and-imm-to-trg instruction
 * @param[in] cg : CodeGenerator
 * @param[in] op : instruction opcode
 * @param[in] node : node
 * @param[in] treg : target register
 * @param[in] s1reg : source register
 * @param[in] imm : immediate value
 * @param[in] preced : preceding instruction
 * @return generated instruction
 */
TR::Instruction *generateTrg1Src1ImmInstruction(
                   TR::CodeGenerator *cg,
                   TR::InstOpCode::Mnemonic op,
                   TR::Node *node,
                   TR::Register *treg,
                   TR::Register *s1reg,
                   uint32_t imm,
                   TR::Instruction *preced = NULL);

/*
 * @brief Generates src2-to-trg instruction
 * @param[in] cg : CodeGenerator
 * @param[in] op : instruction opcode
 * @param[in] node : node
 * @param[in] treg : target register
 * @param[in] s1reg : source register 1
 * @param[in] s2reg : source register 2
 * @param[in] preced : preceding instruction
 * @return generated instruction
 */
TR::Instruction *generateTrg1Src2Instruction(
                   TR::CodeGenerator *cg,
                   TR::InstOpCode::Mnemonic op,
                   TR::Node *node,
                   TR::Register *treg,
                   TR::Register *s1reg,
                   TR::Register *s2reg,
                   TR::Instruction *preced = NULL);

/*
 * @brief Generates src2-to-trg instruction (Conditional register)
 * @param[in] cg : CodeGenerator
 * @param[in] op : instruction opcode
 * @param[in] node : node
 * @param[in] treg : target register
 * @param[in] s1reg : source register 1
 * @param[in] s2reg : source register 2
 * @param[in] cc : branch condition code
 * @param[in] preced : preceding instruction
 * @return generated instruction
 */
TR::Instruction *generateCondTrg1Src2Instruction(
                   TR::CodeGenerator *cg,
                   TR::InstOpCode::Mnemonic op,
                   TR::Node *node,
                   TR::Register *treg,
                   TR::Register *s1reg,
                   TR::Register *s2reg,
                   TR::ARM64ConditionCode cc,
                   TR::Instruction *preced = NULL);

/*
 * @brief Generates src2-to-trg instruction (Conditional register)
 * @param[in] cg : CodeGenerator
 * @param[in] op : instruction opcode
 * @param[in] node : node
 * @param[in] treg : target register
 * @param[in] s1reg : source register 1
 * @param[in] s2reg : source register 2
 * @param[in] cc : branch condition code
 * @param[in] cond : Register Dependency Condition
 * @param[in] preced : preceding instruction
 * @return generated instruction
 */
TR::Instruction *generateCondTrg1Src2Instruction(
                   TR::CodeGenerator *cg,
                   TR::InstOpCode::Mnemonic op,
                   TR::Node *node,
                   TR::Register *treg,
                   TR::Register *s1reg,
                   TR::Register *s2reg,
                   TR::ARM64ConditionCode cc,
                   TR::RegisterDependencyConditions *cond,
                   TR::Instruction *preced = NULL);

/*
 * @brief Generates src2-to-trg instruction (shifted register)
 * @param[in] cg : CodeGenerator
 * @param[in] op : instruction opcode
 * @param[in] node : node
 * @param[in] treg : target register
 * @param[in] s1reg : source register 1
 * @param[in] s2reg : source register 2
 * @param[in] shiftType : shift type
 * @param[in] shiftAmount : shift amount
 * @param[in] preced : preceding instruction
 * @return generated instruction
 */
TR::Instruction *generateTrg1Src2ShiftedInstruction(
                   TR::CodeGenerator *cg,
                   TR::InstOpCode::Mnemonic op,
                   TR::Node *node,
                   TR::Register *treg,
                   TR::Register *s1reg,
                   TR::Register *s2reg,
                   TR::ARM64ShiftCode shiftType,
                   uint32_t shiftAmount,
                   TR::Instruction *preced = NULL);

/*
 * @brief Generates src2-to-trg instruction (extended register)
 * @param[in] cg : CodeGenerator
 * @param[in] op : instruction opcode
 * @param[in] node : node
 * @param[in] treg : target register
 * @param[in] s1reg : source register 1
 * @param[in] s2reg : source register 2
 * @param[in] extendType : extend type
 * @param[in] shiftAmount : shift amount
 * @param[in] preced : preceding instruction
 * @return generated instruction
 */
TR::Instruction *generateTrg1Src2ExtendedInstruction(
                   TR::CodeGenerator *cg,
                   TR::InstOpCode::Mnemonic op,
                   TR::Node *node,
                   TR::Register *treg,
                   TR::Register *s1reg,
                   TR::Register *s2reg,
                   TR::ARM64ExtendCode extendType,
                   uint32_t shiftAmount,
                   TR::Instruction *preced = NULL);

/*
 * @brief Generates src2-to-trg indexed element instruction
 * @param[in] cg : CodeGenerator
 * @param[in] op : instruction opcode
 * @param[in] node : node
 * @param[in] treg : target register
 * @param[in] s1reg : source register 1
 * @param[in] s2reg : source register 2
 * @param[in] index : index of element in s2reg
 * @param[in] preced : preceding instruction
 * @return generated instruction
 */
TR::Instruction *generateTrg1Src2IndexedElementInstruction(
                   TR::CodeGenerator *cg,
                   TR::InstOpCode::Mnemonic op,
                   TR::Node *node,
                   TR::Register *treg,
                   TR::Register *s1reg,
                   TR::Register *s2reg,
                   uint32_t index,
                   TR::Instruction *preced = NULL);

/*
 * @brief Generates src3-to-trg instruction
 * @param[in] cg : CodeGenerator
 * @param[in] op : instruction opcode
 * @param[in] node : node
 * @param[in] treg : target register
 * @param[in] s1reg : source register 1
 * @param[in] s2reg : source register 2
 * @param[in] s3reg : source register 3
 * @param[in] preced : preceding instruction
 * @return generated instruction
 */
TR::Instruction *generateTrg1Src3Instruction(
                   TR::CodeGenerator *cg,
                   TR::InstOpCode::Mnemonic op,
                   TR::Node *node,
                   TR::Register *treg,
                   TR::Register *s1reg,
                   TR::Register *s2reg,
                   TR::Register *s3reg,
                   TR::Instruction *preced = NULL);

/*
 * @brief Generates src3-to-trg instruction with register dependency
 * @param[in] cg : CodeGenerator
 * @param[in] op : instruction opcode
 * @param[in] node : node
 * @param[in] treg : target register
 * @param[in] s1reg : source register 1
 * @param[in] s2reg : source register 2
 * @param[in] s3reg : source register 3
 * @param[in] cond : register dependency condition
 * @param[in] preced : preceding instruction
 * @return generated instruction
 */
TR::Instruction *generateTrg1Src3Instruction(
                   TR::CodeGenerator *cg,
                   TR::InstOpCode::Mnemonic op,
                   TR::Node *node,
                   TR::Register *treg,
                   TR::Register *s1reg,
                   TR::Register *s2reg,
                   TR::Register *s3reg,
                   TR::RegisterDependencyConditions *cond,
                   TR::Instruction *preced = NULL);

/*
 * @brief Generates mem-to-trg instruction
 * @param[in] cg : CodeGenerator
 * @param[in] op : instruction opcode
 * @param[in] node : node
 * @param[in] treg : target register
 * @param[in] mr : memory reference
 * @param[in] preced : preceding instruction
 * @return generated instruction
 */
TR::Instruction *generateTrg1MemInstruction(
                   TR::CodeGenerator *cg,
                   TR::InstOpCode::Mnemonic op,
                   TR::Node *node,
                   TR::Register *treg,
                   TR::MemoryReference *mr,
                   TR::Instruction *preced = NULL);

/*
 * @brief Generates mem-imm instruction
 * @param[in] cg : CodeGenerator
 * @param[in] op : instruction opcode
 * @param[in] node : node
 * @param[in] mr : memory reference
 * @param[in] imm : immediate
 * @param[in] preced : preceding instruction
 * @return generated instruction
 */
TR::Instruction *generateMemImmInstruction(
                   TR::CodeGenerator *cg,
                   TR::InstOpCode::Mnemonic op,
                   TR::Node *node,
                   TR::MemoryReference *mr,
                   uint32_t imm,
                   TR::Instruction *preced = NULL);

/*
 * @brief Generates src-to-mem instruction
 * @param[in] cg : CodeGenerator
 * @param[in] op : instruction opcode
 * @param[in] node : node
 * @param[in] mr : memory reference
 * @param[in] sreg : source register
 * @param[in] preced : preceding instruction
 * @return generated instruction
 */
TR::Instruction *generateMemSrc1Instruction(
                   TR::CodeGenerator *cg,
                   TR::InstOpCode::Mnemonic op,
                   TR::Node *node,
                   TR::MemoryReference *mr,
                   TR::Register *sreg,
                   TR::Instruction *preced = NULL);

/*
 * @brief Generates src2-to-mem instruction
 * @param[in] cg : CodeGenerator
 * @param[in] op : instruction opcode
 * @param[in] node : node
 * @param[in] mr : memory reference
 * @param[in] s1reg : source register 1
 * @param[in] s2reg : source register 2
 * @param[in] preced : preceding instruction
 * @return generated instruction
 */
TR::Instruction *generateMemSrc2Instruction(
                   TR::CodeGenerator *cg,
                   TR::InstOpCode::Mnemonic op,
                   TR::Node *node,
                   TR::MemoryReference *mr,
                   TR::Register *s1reg,
                   TR::Register *s2reg,
                   TR::Instruction *preced = NULL);

/*
 * @brief Generates "store exclusive" instruction
 * @param[in] cg : CodeGenerator
 * @param[in] op : instruction opcode
 * @param[in] node : node
 * @param[in] treg : target register
 * @param[in] mr : memory reference
 * @param[in] sreg : source register
 * @param[in] preced : preceding instruction
 * @return generated instruction
 */
TR::Instruction *generateTrg1MemSrc1Instruction(
                   TR::CodeGenerator *cg,
                   TR::InstOpCode::Mnemonic op,
                   TR::Node *node,
                   TR::Register *treg,
                   TR::MemoryReference *mr,
                   TR::Register *sreg,
                   TR::Instruction *preced = NULL);

/*
 * @brief Generates src1 instruction
 * @param[in] cg : CodeGenerator
 * @param[in] op : instruction opcode
 * @param[in] node : node
 * @param[in] s1reg : source register
 * @param[in] preced : preceding instruction
 * @return generated instruction
 */
TR::Instruction *generateSrc1Instruction(
                   TR::CodeGenerator *cg,
                   TR::InstOpCode::Mnemonic op,
                   TR::Node *node,
                   TR::Register *s1reg,
                   TR::Instruction *preced = NULL);

/*
 * @brief Generates src2 instruction
 * @param[in] cg : CodeGenerator
 * @param[in] op : instruction opcode
 * @param[in] node : node
 * @param[in] s1reg : source register 1
 * @param[in] s2reg : source register 2
 * @param[in] preced : preceding instruction
 * @return generated instruction
 */
TR::Instruction *generateSrc2Instruction(
                   TR::CodeGenerator *cg,
                   TR::InstOpCode::Mnemonic op,
                   TR::Node *node,
                   TR::Register *s1reg,
                   TR::Register *s2reg,
                   TR::Instruction *preced = NULL);

/*
 * @brief Generates ASR instruction
 * @param[in] cg : CodeGenerator
 * @param[in] node : node
 * @param[in] treg : target register
 * @param[in] sreg : source register
 * @param[in] shiftAmount : shift amount
 * @param[in] is64bit : true when it is 64-bit operation
 * @param[in] preced : preceding instruction
 * @return generated instruction
 */
TR::Instruction *generateArithmeticShiftRightImmInstruction(
                   TR::CodeGenerator *cg,
                   TR::Node *node,
                   TR::Register *treg,
                   TR::Register *sreg,
                   uint32_t shiftAmount,
                   bool is64bit = true,
                   TR::Instruction *preced = NULL);

/*
 * @brief Generates LSR instruction
 * @param[in] cg : CodeGenerator
 * @param[in] node : node
 * @param[in] treg : target register
 * @param[in] sreg : source register
 * @param[in] shiftAmount : shift amount
 * @param[in] is64bit : true when it is 64-bit operation
 * @param[in] preced : preceding instruction
 * @return generated instruction
 */
TR::Instruction *generateLogicalShiftRightImmInstruction(
                   TR::CodeGenerator *cg,
                   TR::Node *node,
                   TR::Register *treg,
                   TR::Register *sreg,
                   uint32_t shiftAmount,
                   bool is64bit = true,
                   TR::Instruction *preced = NULL);

/*
 * @brief Generates LSL instruction
 * @param[in] cg : CodeGenerator
 * @param[in] node : node
 * @param[in] treg : target register
 * @param[in] sreg : source register
 * @param[in] shiftAmount : shift amount
 * @param[in] is64bit : true when it is 64-bit operation
 * @param[in] preced : preceding instruction
 * @return generated instruction
 */
TR::Instruction *generateLogicalShiftLeftImmInstruction(
                   TR::CodeGenerator *cg,
                   TR::Node *node,
                   TR::Register *treg,
                   TR::Register *sreg,
                   uint32_t shiftAmount,
                   bool is64bit = true,
                   TR::Instruction *preced = NULL);

/*
 * @brief Generates logical immediate instruction
 * @param[in] cg : CodeGenerator
 * @param[in] op : instruction opcode
 * @param[in] node : node
 * @param[in] treg : target register
 * @param[in] s1reg : source register
 * @param[in] N : N bit (bit 22) value
 * @param[in] imms : immediate value
 * @param[in] preced : preceding instruction
 * @return generated instruction
 */
TR::Instruction *generateLogicalImmInstruction(
                   TR::CodeGenerator *cg,
                   TR::InstOpCode::Mnemonic op,
                   TR::Node *node,
                   TR::Register *treg,
                   TR::Register *s1reg,
                   bool N,
                   uint32_t imm,
                   TR::Instruction *preced = NULL);

/*
 * @brief Generates CMP (immediate) instruction
 * @param[in] cg : CodeGenerator
 * @param[in] node : node
 * @param[in] sreg : source register
 * @param[in] imm : immediate value
 * @param[in] is64bit : true when it is 64-bit operation
 * @param[in] preced : preceding instruction
 * @return generated instruction
 */
TR::Instruction *generateCompareImmInstruction(
                  TR::CodeGenerator *cg,
                  TR::Node *node,
                  TR::Register *sreg,
                  int32_t imm,
                  bool is64bit = false,
                  TR::Instruction *preced = NULL);

/*
 * @brief Generates CMP (register) instruction
 * @param[in] cg : CodeGenerator
 * @param[in] node : node
 * @param[in] s1reg : source register 1
 * @param[in] s2reg : source register 2
 * @param[in] is64bit : true when it is 64-bit operation
 * @param[in] preced : preceding instruction
 * @return generated instruction
 */
TR::Instruction *generateCompareInstruction(
                  TR::CodeGenerator *cg,
                  TR::Node *node,
                  TR::Register *s1reg,
                  TR::Register *s2reg,
                  bool is64bit = false,
                  TR::Instruction *preced = NULL);

/*
 * @brief Generates TST (immediate) instruction
 * @param[in] cg : CodeGenerator
 * @param[in] node : node
 * @param[in] sreg : source register
 * @param[in] imm : immediate value
 * @param[in] is64bit : true when it is 64-bit operation
 * @param[in] preced : preceding instruction
 * @return generated instruction
 */
TR::Instruction *generateTestImmInstruction(
                  TR::CodeGenerator *cg,
                  TR::Node *node,
                  TR::Register *sreg,
                  int32_t imm,
                  bool is64bit = false,
                  TR::Instruction *preced = NULL);

/*
 * @brief Generates TST (register) instruction
 * @param[in] cg : CodeGenerator
 * @param[in] node : node
 * @param[in] s1reg : source register 1
 * @param[in] s2reg : source register 2
 * @param[in] is64bit : true when it is 64-bit operation
 * @param[in] preced : preceding instruction
 * @return generated instruction
 */
TR::Instruction *generateTestInstruction(
                  TR::CodeGenerator *cg,
                  TR::Node *node,
                  TR::Register *s1reg,
                  TR::Register *s2reg,
                  bool is64bit = false,
                  TR::Instruction *preced = NULL);

/*
 * @brief Generates MOV (register) instruction
 * @param[in] cg : CodeGenerator
 * @param[in] node : node
 * @param[in] treg : target register
 * @param[in] sreg : source register
 * @param[in] is64bit : true when it is 64-bit operation
 * @param[in] preced : preceding instruction
 * @return generated instruction
 */
TR::Instruction *generateMovInstruction(
                  TR::CodeGenerator *cg,
                  TR::Node *node,
                  TR::Register *treg,
                  TR::Register *sreg,
                  bool is64bit = true,
                  TR::Instruction *preced = NULL);

/*
 * @brief Generates MVN (register) instruction
 * @param[in] cg : CodeGenerator
 * @param[in] node : node
 * @param[in] treg : target register
 * @param[in] sreg : source register
 * @param[in] is64bit : true when it is 64-bit operation
 * @param[in] preced : preceding instruction
 * @return generated instruction
 */
TR::Instruction *generateMvnInstruction(
                  TR::CodeGenerator *cg,
                  TR::Node *node,
                  TR::Register *treg,
                  TR::Register *sreg,
                  bool is64bit = true,
                  TR::Instruction *preced = NULL);

/*
 * @brief Generates NEG (register) instruction
 * @param[in] cg : CodeGenerator
 * @param[in] node : node
 * @param[in] treg : target register
 * @param[in] sreg : source register
 * @param[in] is64bit : true when it is 64-bit operation
 * @param[in] preced : preceding instruction
 * @return generated instruction
 */
TR::Instruction *generateNegInstruction(
                  TR::CodeGenerator *cg,
                  TR::Node *node,
                  TR::Register *treg,
                  TR::Register *sreg,
                  bool is64bit = false,
                  TR::Instruction *preced = NULL);

/*
 * @brief Generates MOV (bitmask immediate) instruction
 * @param[in] cg : CodeGenerator
 * @param[in] node : node
 * @param[in] treg : target register
 * @param[in] N : N bit (bit 22) value
 * @param[in] imm : immediate value
 * @param[in] is64bit : true when it is 64-bit operation
 * @param[in] preced : preceding instruction
 * @return generated instruction
 */
TR::Instruction *generateMovBitMaskInstruction(
                  TR::CodeGenerator *cg,
                  TR::Node *node,
                  TR::Register *treg,
                  bool N,
                  uint32_t imm,
                  bool is64bit = true,
                  TR::Instruction *preced = NULL);

/*
 * @brief Generates MUL (register) instruction
 * @param[in] cg : CodeGenerator
 * @param[in] node : node
 * @param[in] treg : target register
 * @param[in] s1reg : source register 1
 * @param[in] s2reg : source register 2
 * @param[in] preced : preceding instruction
 * @return generated instruction
 */
TR::Instruction *generateMulInstruction(
                  TR::CodeGenerator *cg,
                  TR::Node *node,
                  TR::Register *treg,
                  TR::Register *s1reg,
                  TR::Register *s2reg,
                  TR::Instruction *preced = NULL);

/*
 * @brief Generates MUL (register) instruction
 * @param[in] cg : CodeGenerator
 * @param[in] node : node
 * @param[in] treg : target register
 * @param[in] s1reg : source register 1
 * @param[in] s2reg : source register 2
 * @param[in] is64bit : true when it is 64-bit operation
 * @param[in] preced : preceding instruction
 * @return generated instruction
 */
TR::Instruction *generateMulInstruction(
                  TR::CodeGenerator *cg,
                  TR::Node *node,
                  TR::Register *treg,
                  TR::Register *s1reg,
                  TR::Register *s2reg,
                  bool is64bit,
                  TR::Instruction *preced = NULL);

/*
 * @brief Generates CSET instruction
 * @param[in] cg : CodeGenerator
 * @param[in] node : node
 * @param[in] treg : target register
 * @param[in] cc : branch condition code
 * @param[in] preced : preceding instruction
 * @return generated instruction
 */
TR::Instruction *generateCSetInstruction(
                  TR::CodeGenerator *cg,
                  TR::Node *node,
                  TR::Register *treg,
                  TR::ARM64ConditionCode cc,
                  TR::Instruction *preced = NULL);

/*
 * @brief Generates data synchronization instruction
 * @param[in] cg : CodeGenerator
 * @param[in] op : instruction opcode
 * @param[in] node : node
 * @param[in] imm : immediate value
 * @param[in] preced : preceding instruction
 * @return generated instruction
 */
TR::ARM64SynchronizationInstruction *generateSynchronizationInstruction(
                  TR::CodeGenerator *cg,
                  TR::InstOpCode::Mnemonic op,
                  TR::Node *node,
                  uint32_t imm,
                  TR::Instruction *preced = NULL);

/*
 * @brief Generates exception generating instruction
 * @param[in] cg : CodeGenerator
 * @param[in] op : instruction opcode
 * @param[in] node : node
 * @param[in] imm : immediate value
 * @param[in] preced : preceding instruction
 * @return generated instruction
 */
TR::ARM64ExceptionInstruction *generateExceptionInstruction(
                  TR::CodeGenerator *cg,
                  TR::InstOpCode::Mnemonic op,
                  TR::Node *node,
                  uint32_t imm,
                  TR::Instruction *preced = NULL);

/**
 * @brief Generates ubfx instruction
 *
 * @details Generates ubfx instruction which copies a bitfield of <width> bits
 *          starting from bit position <lsb> in the source register to
 *          the least significant bits of the target register.
 *          The bits above the bitfield in the target register is set to 0.
 *
 * @param[in] cg      : CodeGenerator
 * @param[in] node    : node
 * @param[in] treg    : target register
 * @param[in] sreg    : source register
 * @param[in] lsb     : the lsb to be copied in the source register
 * @param[in] width   : the bitfield width to copy
 * @param[in] is64bit : true if 64bit
 * @param[in] preced  : preceding instruction
 * @return generated instruction
 */
TR::Instruction *generateUBFXInstruction(
                  TR::CodeGenerator *cg,
                  TR::Node *node,
                  TR::Register *treg,
                  TR::Register *sreg,
                  uint32_t lsb,
                  uint32_t width,
                  bool is64bit,
                  TR::Instruction *preced = NULL);

/**
 * @brief Generates ubfiz instruction
 *
 * @details Generates ubfiz instruction which copies a bitfield of <width> bits
 *          from the least significant bits of the source register to
 *          the bit position <lsb> of the target register.
 *          The bits above and below the bitfield in the target register is set to 0.
 *
 * @param[in] cg      : CodeGenerator
 * @param[in] node    : node
 * @param[in] treg    : target register
 * @param[in] sreg    : source register
 * @param[in] lsb     : the bit position of the target register
 * @param[in] width   : the bitfield width to copy
 * @param[in] is64bit : true if 64bit
 * @param[in] preced  : preceding instruction
 * @return generated instruction
 */
TR::Instruction *generateUBFIZInstruction(
                  TR::CodeGenerator *cg,
                  TR::Node *node,
                  TR::Register *treg,
                  TR::Register *sreg,
                  uint32_t lsb,
                  uint32_t width,
                  bool is64bit,
                  TR::Instruction *preced = NULL);

/**
 * @brief Generates vector shift left immediate instruction
 *
 * @param[in] cg          : CodeGenerator
 * @param[in] op          : opcode
 * @param[in] node        : node
 * @param[in] treg        : target register
 * @param[in] sreg        : source register
 * @param[in] shiftAmount : shift amount
 * @param[in] preced      : preceding instruction
 * @return generated instruction
 */
TR::Instruction *generateVectorShiftImmediateInstruction(
                  TR::CodeGenerator *cg,
                  TR::InstOpCode::Mnemonic op,
                  TR::Node *node,
                  TR::Register *treg,
                  TR::Register *sreg,
                  uint32_t shiftAmount,
                  TR::Instruction *preced = NULL);

/**
 * @brief Generates duplicate vector element instruction
 *
 * @param[in] cg          : CodeGenerator
 * @param[in] op          : opcode
 * @param[in] node        : node
 * @param[in] treg        : target register
 * @param[in] sreg        : source register
 * @param[in] srcIndex    : source element index
 * @param[in] preced      : preceding instruction
 * @return generated instruction
 */
TR::Instruction *generateVectorDupElementInstruction(
                  TR::CodeGenerator *cg,
                  TR::InstOpCode::Mnemonic op,
                  TR::Node *node,
                  TR::Register *treg,
                  TR::Register *sreg,
                  uint32_t srcIndex,
                  TR::Instruction *preced = NULL);

/**
 * @brief Generates signed or unsigned move vector element to general purpose register instruction
 *
 * @param[in] cg          : CodeGenerator
 * @param[in] op          : opcode
 * @param[in] node        : node
 * @param[in] treg        : target register
 * @param[in] sreg        : source register
 * @param[in] srcIndex    : source element index
 * @param[in] preced      : preceding instruction
 * @return generated instruction
 */
TR::Instruction *generateMovVectorElementToGPRInstruction(
                  TR::CodeGenerator *cg,
                  TR::InstOpCode::Mnemonic op,
                  TR::Node *node,
                  TR::Register *treg,
                  TR::Register *sreg,
                  uint32_t srcIndex,
                  TR::Instruction *preced = NULL);

/**
 * @brief Generates move general purpose register to vector element instruction
 *
 * @param[in] cg          : CodeGenerator
 * @param[in] op          : opcode
 * @param[in] node        : node
 * @param[in] treg        : target register
 * @param[in] sreg        : source register
 * @param[in] trgIndex    : target element index
 * @param[in] preced      : preceding instruction
 * @return generated instruction
 */
TR::Instruction *generateMovGPRToVectorElementInstruction(
                  TR::CodeGenerator *cg,
                  TR::InstOpCode::Mnemonic op,
                  TR::Node *node,
                  TR::Register *treg,
                  TR::Register *sreg,
                  uint32_t trgIndex,
                  TR::Instruction *preced = NULL);


/**
 * @brief Generates move vector element instruction
 *
 * @param[in] cg          : CodeGenerator
 * @param[in] op          : opcode
 * @param[in] node        : node
 * @param[in] treg        : target register
 * @param[in] sreg        : source register
 * @param[in] trgIndex    : target element index
 * @param[in] srcIndex    : source element index
 * @param[in] preced      : preceding instruction
 * @return generated instruction
 */
TR::Instruction *generateMovVectorElementInstruction(
                  TR::CodeGenerator *cg,
                  TR::InstOpCode::Mnemonic op,
                  TR::Node *node,
                  TR::Register *treg,
                  TR::Register *sreg,
                  uint32_t trgIndex,
                  uint32_t srcIndex,
                  TR::Instruction *preced = NULL);

#ifdef J9_PROJECT_SPECIFIC
/*
 * @brief Generates virtual guard nop instruction
 * @param[in] cg : CodeGenerator
 * @param[in] node : node
 * @param[in] site : virtual guard site
 * @param[in] cond : register dependency condition
 * @param[in] sym : label symbol
 * @param[in] preced : preceding instruction
 * @return generated instruction
 */
TR::Instruction *generateVirtualGuardNOPInstruction(TR::CodeGenerator *cg,  TR::Node *node, TR_VirtualGuardSite *site,
   TR::RegisterDependencyConditions *cond, TR::LabelSymbol *sym, TR::Instruction *preced = NULL);

#endif
#endif
