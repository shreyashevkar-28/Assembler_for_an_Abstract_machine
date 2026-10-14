#pragma once

#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <vector>

namespace w2 {

// ---- Strongly typed vocabulary ------------------------------------------

// Numbered exactly as the hardware encodes them in ModRM/SIB fields.
enum class Reg : uint8_t {
    EAX = 0, ECX = 1, EDX = 2, EBX = 3, ESP = 4, EBP = 5, ESI = 6, EDI = 7,
    NONE = 0xFF
};

enum class Op : uint8_t {
    MOV, ADD, SUB, AND, OR, XOR, CMP,   // two operands
    PUSH, POP, INC, DEC,                // one register operand
    JMP, CALL, JE, JNE,                 // one LABEL operand (rel32, near form)
    RET, NOP,                           // no operands

    // ---- added instruction set -------------------------------------------
    TEST, XCHG, IMUL,                   // two operands (IMUL = r32, r/m32 form)
    SHL, SHR, SAR,                      // r/m32, imm8 (count 0..31)
    NOT, NEG, MUL, DIV, IDIV,           // one register operand (F7 group)
    INT,                                // one imm8 operand
    JL, JGE, JLE, JG,                   // signed conditional jumps  (rel32, near)
    JB, JAE, JBE, JA,                   // unsigned conditional jumps (rel32, near)
    JS, JNS,                            // sign-flag conditional jumps (rel32, near)
    HLT, CLC, STC, CLD, STD, CDQ,       // no operands, single byte
    LEAVE, PUSHAD, POPAD, PUSHFD, POPFD, // no operands, single byte
    LEA                                 // LEA r32, m
};

// MEMLABEL = absolute memory operand "[label]"; encoded as [disp32] where
// disp32 = Program::baseAddress + offset of the label.
enum class OperandKind : uint8_t { NONE, REG, IMM, MEM, LABEL, MEMLABEL };

typedef uint32_t LabelId;
constexpr LabelId NO_LABEL = 0xFFFFFFFFu;

// [base + index*scale + disp].  Absent parts are Reg::NONE; with neither
// base nor index, `disp` is an absolute address.
struct MemOperand {
    Reg     base;
    Reg     index;   // ESP is not encodable as an index
    uint8_t scale;   // 1, 2, 4 or 8 (ignored when index == NONE)
    int32_t disp;
};

struct Operand {
    OperandKind kind;
    union {
        Reg        reg;     // kind == REG
        int32_t    imm;     // kind == IMM
        LabelId    label;   // kind == LABEL or MEMLABEL
        MemOperand mem;     // kind == MEM
    };
};

struct Instruction {
    Op       op;
    uint8_t  operandCount;   // 0..2
    Operand  operands[2];    // operands[0] is the destination (Intel order)
    LabelId  defLabel;       // label defined at this instruction, or NO_LABEL
};

static_assert(std::is_pod<Instruction>::value, "Instruction must stay plain-old-data");

struct Program {
    const Instruction* instrs;
    size_t             count;
    uint32_t           labelCount;    // valid label IDs are [0, labelCount)
    uint32_t           baseAddress;   // added to label offsets for absolute ([label]) operands
    LabelId            endLabel;      // label located after the last instruction, or NO_LABEL
};

static_assert(std::is_pod<Program>::value, "Program must stay plain-old-data");
