// worklet2.cpp -- x86-32 encoder. Consumes only enums/integers; no text handling.
#include "worklet2.h"

#include <fstream>

namespace w2 {
namespace {

const size_t kUndefined = static_cast<size_t>(-1);

struct Fixup {
    size_t  pos;        // offset of the 32-bit field in the output
    LabelId label;
    size_t  instr;      // for error reporting
    bool    absolute;   // true: baseAddress + target + addend;  false: rel32 (target - end of field)
    int32_t addend;     // constant added to an absolute fixup ("[label+4]"); 0 for rel32
    bool    rel8;       // true: 1-byte relative field (LOOP / JECXZ); range-checked at patch time
};

// ---- byte emission (little-endian, independent of host endianness) --------

inline void put8(std::vector<uint8_t>& o, uint32_t v) {
    o.push_back(static_cast<uint8_t>(v));
}
inline void put16(std::vector<uint8_t>& o, uint32_t v) {
    o.push_back(static_cast<uint8_t>(v));
    o.push_back(static_cast<uint8_t>(v >> 8));
}
inline void put32(std::vector<uint8_t>& o, uint32_t v) {
    o.push_back(static_cast<uint8_t>(v));
    o.push_back(static_cast<uint8_t>(v >> 8));
    o.push_back(static_cast<uint8_t>(v >> 16));
    o.push_back(static_cast<uint8_t>(v >> 24));
}
inline void patch32(std::vector<uint8_t>& o, size_t pos, uint32_t v) {
    o[pos + 0] = static_cast<uint8_t>(v);
    o[pos + 1] = static_cast<uint8_t>(v >> 8);
    o[pos + 2] = static_cast<uint8_t>(v >> 16);
    o[pos + 3] = static_cast<uint8_t>(v >> 24);
}

// ---- operand shape predicates ----------------------------------------------

inline uint8_t code(Reg r)                    { return static_cast<uint8_t>(r); }
inline bool isGpr(Reg r)                      { return code(r) < 8; }
inline bool isReg(const Operand& o)           { return o.kind == OperandKind::REG && isGpr(o.reg); }
inline bool isMem(const Operand& o)           { return o.kind == OperandKind::MEM || o.kind == OperandKind::MEMLABEL; }
inline bool isImm(const Operand& o)           { return o.kind == OperandKind::IMM; }
inline bool isRM(const Operand& o)            { return isReg(o) || isMem(o); }
inline bool fitsInt8(int32_t v)               { return v >= -128 && v <= 127; }

// ---- operand size (8 / 16 / 32 bit) ------------------------------------------
// Register operands carry their width in Operand::size (0 = 32).  Memory operands
// carry the access width, or 0 when unspecified.
inline unsigned regBits(const Operand& o)     { return o.size ? o.size : 32u; }

// Works out the data size of an instruction from its (up to two) operands.
// A register operand fixes the size; a sized memory operand must agree with it;
// with neither, the size is 32 (the original behaviour).
Status resolveSize(const Operand* a, const Operand* b, unsigned& bits) {
    unsigned regSz = 0, memSz = 0;
    const Operand* ops[2] = { a, b };
    for (int i = 0; i < 2; ++i) {
        const Operand* o = ops[i];
        if (!o) continue;
        if (isReg(*o)) {
            const unsigned s = regBits(*o);
            if (s != 8 && s != 16 && s != 32) return Status::BAD_OPERAND_SIZE;
            if (regSz && regSz != s)          return Status::BAD_OPERAND_SIZE;
            regSz = s;
        } else if (isMem(*o)) {
            const unsigned s = o->size;
            if (s == 0) continue;
            if (s != 8 && s != 16 && s != 32) return Status::BAD_OPERAND_SIZE;
            memSz = s;
        }
    }
    if (regSz && memSz && regSz != memSz) return Status::BAD_OPERAND_SIZE;
    bits = regSz ? regSz : (memSz ? memSz : 32u);
    return Status::OK;
}

// Immediate must be representable in the operand size (signed or unsigned reading).
inline bool immFits(unsigned bits, int32_t v) {
    if (bits == 8)  return v >= -128   && v <= 255;
    if (bits == 16) return v >= -32768 && v <= 65535;
    return true;
}
inline void putImm(std::vector<uint8_t>& o, unsigned bits, int32_t v) {
    if (bits == 8)       put8(o, static_cast<uint32_t>(v));
    else if (bits == 16) put16(o, static_cast<uint32_t>(v));
    else                 put32(o, static_cast<uint32_t>(v));
}
// 0x66 operand-size prefix for 16-bit operations.
inline void putPrefix(std::vector<uint8_t>& o, unsigned bits) {
    if (bits == 16) put8(o, 0x66);
}
// For every opcode pair used here, the 8-bit form is the 16/32-bit form minus one
// (88/89, 8A/8B, 84/85, 86/87, 00/01, F6/F7, FE/FF, C6/C7, C0/C1, D0/D1, D2/D3, A8/A9).
inline uint8_t opc(unsigned bits, uint8_t wordForm) {
    return static_cast<uint8_t>(bits == 8 ? wordForm - 1 : wordForm);
}

// ---- ModRM / SIB / displacement ---------------------------------------------
// Emits the addressing bytes for `rm` (a REG, MEM or MEMLABEL operand).
// `regField` goes in ModRM.reg: either the other register operand, or an
// opcode /digit.
Status emitModRM(std::vector<uint8_t>& out, uint32_t regField, const Operand& rm,
                 std::vector<Fixup>& fixups, size_t idx) {
    if (rm.kind == OperandKind::REG) {                      // mod = 11
        put8(out, 0xC0 | (regField << 3) | code(rm.reg));
        return Status::OK;
    }

    if (rm.kind == OperandKind::MEMLABEL) {                 // [abs32 of label]
        put8(out, (regField << 3) | 0x05);                  // mod=00, rm=101
        Fixup f = { out.size(), rm.label, idx, true, 0, false };
        fixups.push_back(f);
        put32(out, 0);                                      // patched later
        return Status::OK;
    }

    const MemOperand& m = rm.mem;
    const bool hasBase  = m.base  != Reg::NONE;
    const bool hasIndex = m.index != Reg::NONE;
    if ((hasBase && !isGpr(m.base)) || (hasIndex && !isGpr(m.index))) return Status::BAD_OPERANDS;

    uint32_t scaleBits = 0;
    if (hasIndex) {
        if (m.index == Reg::ESP) return Status::BAD_INDEX_REG;
        switch (m.scale) {
            case 1: scaleBits = 0; break;
            case 2: scaleBits = 1; break;
            case 4: scaleBits = 2; break;
            case 8: scaleBits = 3; break;
            default: return Status::BAD_SCALE;
        }
    }

    if (m.hasLabel) {                                       // [label + base + index*scale + disp]
        // The label is a 32-bit absolute field, so the displacement is always disp32.
        // Worklet 1 sizes every symbolic operand that way (4 bytes, plus SIB if needed).
        const bool sib = hasIndex || (hasBase && m.base == Reg::ESP);
        if (!hasBase && !hasIndex) {                        // [label+disp]   mod=00 rm=101
            put8(out, (regField << 3) | 0x05);
        } else if (!hasBase) {                              // [label+index*scale]  SIB, base=101
            put8(out, (regField << 3) | 0x04);
            put8(out, (scaleBits << 6) | (code(m.index) << 3) | 0x05);
        } else {                                            // mod=10 (disp32)
            put8(out, (2u << 6) | (regField << 3) | (sib ? 4u : code(m.base)));
            if (sib)
                put8(out, (scaleBits << 6) | ((hasIndex ? code(m.index) : 4u) << 3) | code(m.base));
        }
        Fixup f = { out.size(), m.label, idx, true, m.disp, false };
        fixups.push_back(f);
        put32(out, 0);                                      // patched later
        return Status::OK;
    }

    if (!hasBase && !hasIndex) {                            // [disp32]
        put8(out, (regField << 3) | 0x05);
        put32(out, static_cast<uint32_t>(m.disp));
        return Status::OK;
    }

    if (!hasBase) {                                         // [index*scale + disp32]
        put8(out, (regField << 3) | 0x04);                  // rm=100 -> SIB follows
        put8(out, (scaleBits << 6) | (code(m.index) << 3) | 0x05);  // base=101 -> none
        put32(out, static_cast<uint32_t>(m.disp));
        return Status::OK;
    }

    // Base present. EBP as base can't use mod=00 (that slot means "no base"),
    // so it always carries an explicit displacement, even a zero one.
    uint32_t mod;
    if (m.disp == 0 && m.base != Reg::EBP) mod = 0;
    else if (fitsInt8(m.disp))             mod = 1;
    else                                   mod = 2;

    // ESP as base (or any index) requires a SIB byte.
    const bool needSib = hasIndex || m.base == Reg::ESP;

    put8(out, (mod << 6) | (regField << 3) | (needSib ? 4u : code(m.base)));
    if (needSib) {
        put8(out, (scaleBits << 6) | ((hasIndex ? code(m.index) : 4u) << 3) | code(m.base));
    }
    if (mod == 1)      put8(out, static_cast<uint32_t>(m.disp));
    else if (mod == 2) put32(out, static_cast<uint32_t>(m.disp));
    return Status::OK;
}

// ---- opcode table for the ALU group (ADD/OR/AND/SUB/XOR/CMP) -----------------
// rmR = "op r/m32, r32", rRm = "op r32, r/m32", ext = /digit for 81 and 83.
// The short "op EAX, imm32" opcode is always rmR + 4.
bool aluForm(Op op, uint8_t& rmR, uint8_t& rRm, uint8_t& ext) {
    switch (op) {
        case Op::ADD: rmR = 0x01; rRm = 0x03; ext = 0; return true;
        case Op::OR:  rmR = 0x09; rRm = 0x0B; ext = 1; return true;
        case Op::ADC: rmR = 0x11; rRm = 0x13; ext = 2; return true;
        case Op::SBB: rmR = 0x19; rRm = 0x1B; ext = 3; return true;
        case Op::AND: rmR = 0x21; rRm = 0x23; ext = 4; return true;
        case Op::SUB: rmR = 0x29; rRm = 0x2B; ext = 5; return true;
        case Op::XOR: rmR = 0x31; rRm = 0x33; ext = 6; return true;
        case Op::CMP: rmR = 0x39; rRm = 0x3B; ext = 7; return true;
        default:      return false;
    }
}

// ---- tables for the added instruction set -------------------------------------
// Opcode of every added instruction that is exactly one byte long.
uint8_t singleByteOpcode(Op op) {
    switch (op) {
        case Op::HLT:    return 0xF4;
        case Op::CLC:    return 0xF8;
        case Op::STC:    return 0xF9;
        case Op::CLD:    return 0xFC;
        case Op::STD:    return 0xFD;
        case Op::CDQ:    return 0x99;
        case Op::LEAVE:  return 0xC9;
        case Op::PUSHAD: return 0x60;
        case Op::POPAD:  return 0x61;
        case Op::PUSHFD: return 0x9C;
        case Op::POPFD:  return 0x9D;
        case Op::CMC:    return 0xF5;
        case Op::CWDE:   return 0x98;
        case Op::LODSB:  return 0xAC;
        case Op::LODSD:  return 0xAD;
        case Op::STOSB:  return 0xAA;
        case Op::STOSD:  return 0xAB;
        case Op::MOVSB:  return 0xA4;
        case Op::MOVSD:  return 0xA5;
        case Op::CMPSB:  return 0xA6;
        case Op::CMPSD:  return 0xA7;
        case Op::SCASB:  return 0xAE;
        case Op::SCASD:  return 0xAF;
        case Op::LAHF:   return 0x9F;
        case Op::SAHF:   return 0x9E;
        case Op::CLI:    return 0xFA;
        case Op::STI:    return 0xFB;
        case Op::WAIT:   return 0x9B;
        default:         return 0x90;   // not reachable: callers pre-filter
    }
}

// Zero-operand instructions whose 16-bit form is the 32-bit opcode behind a 0x66 prefix.
uint8_t prefixedOpcode(Op op) {
    switch (op) {
        case Op::CBW:   return 0x98;
        case Op::CWD:   return 0x99;
        case Op::PUSHA: return 0x60;
        case Op::POPA:  return 0x61;
        case Op::PUSHF: return 0x9C;
        case Op::POPF:  return 0x9D;
        case Op::LODSW: return 0xAD;
        case Op::MOVSW: return 0xA5;
        case Op::CMPSW: return 0xA7;
        case Op::SCASW: return 0xAF;
        default:        return 0xAB;    // STOSW
    }
}

// /digit of the F7 group (NOT/NEG/MUL/DIV/IDIV r/m32).
uint8_t unaryDigit(Op op) {
    switch (op) {
        case Op::NOT:  return 2;
        case Op::NEG:  return 3;
        case Op::MUL:  return 4;
        case Op::DIV:  return 6;
        default:       return 7;        // IDIV
    }
}

// /digit of the C0/C1/D0-D3 group (shift / rotate r/m, imm8 or CL).
uint8_t shiftDigit(Op op) {
    switch (op) {
        case Op::ROL: return 0;
        case Op::ROR: return 1;
        case Op::RCL: return 2;
        case Op::RCR: return 3;
        case Op::SHL: return 4;
        case Op::SHR: return 5;
        default:      return 7;         // SAR
    }
}

// Second opcode byte of a near conditional jump (0F 8x rel32).
uint8_t jccOpcode(Op op) {
    switch (op) {
        case Op::JO:  return 0x80;
        case Op::JNO: return 0x81;
        case Op::JB:  return 0x82;
        case Op::JAE: return 0x83;
        case Op::JBE: return 0x86;
        case Op::JA:  return 0x87;
        case Op::JS:  return 0x88;
        case Op::JNS: return 0x89;
        case Op::JP:  return 0x8A;
        case Op::JNP: return 0x8B;
        case Op::JL:  return 0x8C;
        case Op::JGE: return 0x8D;
        case Op::JLE: return 0x8E;
        default:      return 0x8F;      // JG
    }
}
