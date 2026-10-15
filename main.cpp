#define main worklet1_main
#include "worklet1.cpp"
#undef main

#include "worklet2.h"

#include <algorithm>
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <map>

// ---------------------------------------------------------------------------
// Adapter: Worklet 1 IR (strings) -> Worklet 2 input (enums + integers)
// ---------------------------------------------------------------------------
namespace {

struct LowerError {
    string message;
    int    line;     // 0 if not tied to a source line
};

bool lookupOp(const string& m, w2::Op& op) {
    if (m == "MOV")  { op = w2::Op::MOV;  return true; }
    if (m == "ADD")  { op = w2::Op::ADD;  return true; }
    if (m == "SUB")  { op = w2::Op::SUB;  return true; }
    if (m == "CMP")  { op = w2::Op::CMP;  return true; }
    if (m == "INC")  { op = w2::Op::INC;  return true; }
    if (m == "DEC")  { op = w2::Op::DEC;  return true; }
    if (m == "PUSH") { op = w2::Op::PUSH; return true; }
    if (m == "POP")  { op = w2::Op::POP;  return true; }
    if (m == "JMP")  { op = w2::Op::JMP;  return true; }
    if (m == "CALL") { op = w2::Op::CALL; return true; }
    if (m == "JE")   { op = w2::Op::JE;   return true; }
    if (m == "JNE")  { op = w2::Op::JNE;  return true; }
    if (m == "NOP")  { op = w2::Op::NOP;  return true; }
    if (m == "RET")  { op = w2::Op::RET;  return true; }
    // ---- added instruction set ----
    if (m == "AND")    { op = w2::Op::AND;    return true; }
    if (m == "OR")     { op = w2::Op::OR;     return true; }
    if (m == "XOR")    { op = w2::Op::XOR;    return true; }
    if (m == "TEST")   { op = w2::Op::TEST;   return true; }
    if (m == "XCHG")   { op = w2::Op::XCHG;   return true; }
    if (m == "IMUL")   { op = w2::Op::IMUL;   return true; }
    if (m == "SHL")    { op = w2::Op::SHL;    return true; }
    if (m == "SHR")    { op = w2::Op::SHR;    return true; }
    if (m == "SAR")    { op = w2::Op::SAR;    return true; }
    if (m == "NOT")    { op = w2::Op::NOT;    return true; }
    if (m == "NEG")    { op = w2::Op::NEG;    return true; }
    if (m == "MUL")    { op = w2::Op::MUL;    return true; }
    if (m == "DIV")    { op = w2::Op::DIV;    return true; }
    if (m == "IDIV")   { op = w2::Op::IDIV;   return true; }
    if (m == "INT")    { op = w2::Op::INT;    return true; }
    if (m == "JL")     { op = w2::Op::JL;     return true; }
    if (m == "JGE")    { op = w2::Op::JGE;    return true; }
    if (m == "JLE")    { op = w2::Op::JLE;    return true; }
    if (m == "JG")     { op = w2::Op::JG;     return true; }
    if (m == "JB")     { op = w2::Op::JB;     return true; }
    if (m == "JAE")    { op = w2::Op::JAE;    return true; }
    if (m == "JBE")    { op = w2::Op::JBE;    return true; }
    if (m == "JA")     { op = w2::Op::JA;     return true; }
    if (m == "JS")     { op = w2::Op::JS;     return true; }
    if (m == "JNS")    { op = w2::Op::JNS;    return true; }
    if (m == "JZ")     { op = w2::Op::JE;     return true; }   // alias of JE
    if (m == "JNZ")    { op = w2::Op::JNE;    return true; }   // alias of JNE
    if (m == "JC")     { op = w2::Op::JB;     return true; }
    if (m == "JNC")    { op = w2::Op::JAE;    return true; }
    if (m == "JNAE")   { op = w2::Op::JB;     return true; }
    if (m == "JNB")    { op = w2::Op::JAE;    return true; }
    if (m == "JNLE")   { op = w2::Op::JG;     return true; }
    if (m == "JNL")    { op = w2::Op::JGE;    return true; }
    if (m == "JNGE")   { op = w2::Op::JL;     return true; }
    if (m == "JNG")    { op = w2::Op::JLE;    return true; }
    if (m == "JNA")    { op = w2::Op::JBE;    return true; }
    if (m == "JNBE")   { op = w2::Op::JA;     return true; }
    if (m == "LEA")    { op = w2::Op::LEA;    return true; }
    if (m == "HLT")    { op = w2::Op::HLT;    return true; }
    if (m == "CLC")    { op = w2::Op::CLC;    return true; }
    if (m == "STC")    { op = w2::Op::STC;    return true; }
    if (m == "CLD")    { op = w2::Op::CLD;    return true; }
    if (m == "STD")    { op = w2::Op::STD;    return true; }
    if (m == "CDQ")    { op = w2::Op::CDQ;    return true; }
    if (m == "LEAVE")  { op = w2::Op::LEAVE;  return true; }
    if (m == "PUSHAD") { op = w2::Op::PUSHAD; return true; }
    if (m == "POPAD")  { op = w2::Op::POPAD;  return true; }
    if (m == "PUSHFD") { op = w2::Op::PUSHFD; return true; }
    if (m == "POPFD")  { op = w2::Op::POPFD;  return true; }
    // ---- 8/16-bit extension ----
    if (m == "SAL")    { op = w2::Op::SHL;    return true; }   // SAL == SHL
    if (m == "ROL")    { op = w2::Op::ROL;    return true; }
    if (m == "ROR")    { op = w2::Op::ROR;    return true; }
    if (m == "RCL")    { op = w2::Op::RCL;    return true; }
    if (m == "RCR")    { op = w2::Op::RCR;    return true; }
    if (m == "CMC")    { op = w2::Op::CMC;    return true; }
    if (m == "CWDE")   { op = w2::Op::CWDE;   return true; }
    if (m == "CBW")    { op = w2::Op::CBW;    return true; }
    if (m == "CWD")    { op = w2::Op::CWD;    return true; }
    if (m == "PUSHA")  { op = w2::Op::PUSHA;  return true; }
    if (m == "POPA")   { op = w2::Op::POPA;   return true; }
    if (m == "PUSHF")  { op = w2::Op::PUSHF;  return true; }
    if (m == "POPF")   { op = w2::Op::POPF;   return true; }
    if (m == "LODSB")  { op = w2::Op::LODSB;  return true; }
    if (m == "LODSW")  { op = w2::Op::LODSW;  return true; }
    if (m == "LODSD")  { op = w2::Op::LODSD;  return true; }
    if (m == "STOSB")  { op = w2::Op::STOSB;  return true; }
    if (m == "STOSW")  { op = w2::Op::STOSW;  return true; }
    if (m == "STOSD")  { op = w2::Op::STOSD;  return true; }
    if (m == "JO")     { op = w2::Op::JO;     return true; }
    if (m == "JNO")    { op = w2::Op::JNO;    return true; }
    if (m == "JP")     { op = w2::Op::JP;     return true; }
    if (m == "JPE")    { op = w2::Op::JP;     return true; }
    if (m == "JNP")    { op = w2::Op::JNP;    return true; }
    if (m == "JPO")    { op = w2::Op::JNP;    return true; }
    // ---- carry arithmetic, remaining string ops, flag / control, short loops ----
    if (m == "ADC")    { op = w2::Op::ADC;    return true; }
    if (m == "SBB")    { op = w2::Op::SBB;    return true; }
    if (m == "MOVSB")  { op = w2::Op::MOVSB;  return true; }
    if (m == "MOVSW")  { op = w2::Op::MOVSW;  return true; }
    if (m == "MOVSD")  { op = w2::Op::MOVSD;  return true; }
    if (m == "CMPSB")  { op = w2::Op::CMPSB;  return true; }
    if (m == "CMPSW")  { op = w2::Op::CMPSW;  return true; }
    if (m == "CMPSD")  { op = w2::Op::CMPSD;  return true; }
    if (m == "SCASB")  { op = w2::Op::SCASB;  return true; }
    if (m == "SCASW")  { op = w2::Op::SCASW;  return true; }
    if (m == "SCASD")  { op = w2::Op::SCASD;  return true; }
    if (m == "LAHF")   { op = w2::Op::LAHF;   return true; }
    if (m == "SAHF")   { op = w2::Op::SAHF;   return true; }
    if (m == "CLI")    { op = w2::Op::CLI;    return true; }
    if (m == "STI")    { op = w2::Op::STI;    return true; }
    if (m == "WAIT")   { op = w2::Op::WAIT;   return true; }
    if (m == "LOOP")   { op = w2::Op::LOOP;   return true; }
    if (m == "LOOPE")  { op = w2::Op::LOOPE;  return true; }
    if (m == "LOOPZ")  { op = w2::Op::LOOPE;  return true; }   // alias of LOOPE
    if (m == "LOOPNE") { op = w2::Op::LOOPNE; return true; }
    if (m == "LOOPNZ") { op = w2::Op::LOOPNE; return true; }   // alias of LOOPNE
    if (m == "JECXZ")  { op = w2::Op::JECXZ;  return true; }
    return false;   // LOAD / STORE / LODS / STOS are handled separately
}

bool lookupReg(const string& name, w2::Reg& r) {
    if (name == "EAX") { r = w2::Reg::EAX; return true; }
    if (name == "ECX") { r = w2::Reg::ECX; return true; }
    if (name == "EDX") { r = w2::Reg::EDX; return true; }
    if (name == "EBX") { r = w2::Reg::EBX; return true; }
    if (name == "ESP") { r = w2::Reg::ESP; return true; }
    if (name == "EBP") { r = w2::Reg::EBP; return true; }
    if (name == "ESI") { r = w2::Reg::ESI; return true; }
    if (name == "EDI") { r = w2::Reg::EDI; return true; }
    return false;
}

// Any general register (32-, 16- or 8-bit) -> encoding number + width in bits.
// Segment registers are not supported.
bool lookupRegSized(const string& name, w2::Reg& r, uint8_t& bits) {
    if (lookupReg(name, r)) { bits = 32; return true; }
    static const char* r16[8] = { "AX", "CX", "DX", "BX", "SP", "BP", "SI", "DI" };
    static const char* r8[8]  = { "AL", "CL", "DL", "BL", "AH", "CH", "DH", "BH" };
    for (int i = 0; i < 8; ++i) {
        if (name == r16[i]) { r = static_cast<w2::Reg>(i); bits = 16; return true; }
        if (name == r8[i])  { r = static_cast<w2::Reg>(i); bits = 8;  return true; }
    }
    return false;
}

// Worklet 1's memory-expression parser keeps no sign for registers/labels, lets a second scaled
// register overwrite the first, and lets a second label overwrite the first. Each of those
// would silently produce the WRONG address, so re-inspect the raw "[...]" text and refuse them.
// (Worklet 1 itself is untouched; this only looks at the text it already stored in Operand::value.)
bool memExprProblem(const string& raw, string& why) {
    const size_t lb = raw.find('['), rb = raw.rfind(']');
    if (lb == string::npos || rb == string::npos || rb < lb) return false;
    string s;
    for (size_t k = lb + 1; k < rb; ++k) if (!isspace(static_cast<unsigned char>(raw[k]))) s += raw[k];

    int regs = 0, scaled = 0, labels = 0;
    size_t i = 0;
    char sign = '+';
    if (!s.empty() && (s[0] == '+' || s[0] == '-')) { sign = s[0]; i = 1; }
    while (i <= s.size()) {
        size_t j = i;
        while (j < s.size() && s[j] != '+' && s[j] != '-') ++j;
        const string term = s.substr(i, j - i);
        if (!term.empty()) {
            const size_t star = term.find('*');
            const string name = toUpper(star == string::npos ? term : term.substr(0, star));
            const string rhs  = (star == string::npos) ? string() : toUpper(term.substr(star + 1));
            w2::Reg r; uint8_t b;
            const bool isReg = lookupRegSized(name, r, b) || isSegmentReg(name);
            const bool rhsReg = !rhs.empty() && (lookupRegSized(rhs, r, b) || isSegmentReg(rhs));
            if (star != string::npos && (isReg || rhsReg)) {            // ECX*4 or 4*ECX
                ++regs; ++scaled;
                if (sign == '-') { why = "a scaled index register cannot be subtracted"; return true; }
            } else if (isReg) {
                ++regs;
                if (sign == '-') { why = "a register cannot be subtracted in a memory operand"; return true; }
            } else if (star == string::npos && (isalpha(static_cast<unsigned char>(term[0])) || term[0] == '_')) {
                ++labels;
                if (sign == '-') { why = "a label cannot be subtracted in a memory operand"; return true; }
            }
        }
        if (j >= s.size()) break;
        sign = s[j];
        i = j + 1;
    }
    if (scaled > 1) { why = "only one scaled index register is allowed in a memory operand"; return true; }
    if (regs   > 2) { why = "at most two registers (base + index) are allowed in a memory operand"; return true; }
    if (labels > 1) { why = "only one label is allowed in a memory operand"; return true; }
    return false;
}

struct Lowering {
    const Worklet1Output&          ir;
    map<unsigned, w2::LabelId>     idByAddr;   // address -> dense label ID
    LowerError                     err;

    explicit Lowering(const Worklet1Output& o) : ir(o) { err.line = 0; }

    bool fail(const string& msg, int line) { err.message = msg; err.line = line; return false; }

    // Access width of a memory operand (8/16/32; 0 = unspecified, 64 is rejected by Worklet 2).
    static uint8_t memBits(const ::Operand& in) { return static_cast<uint8_t>(in.size); }

    // Registers inside [...] must be 32-bit: Worklet 1 sizes memory operands without a
    // 0x67 address-size prefix, so 16-bit addressing ([BX+SI]) cannot be encoded to match.
    static string addrRegError(const char* role, const string& name) {
        w2::Reg r; uint8_t bits;
        if (lookupRegSized(name, r, bits))
            return string("16/8-bit register '") + name + "' cannot be used as a memory " + role +
                   " register (use the 32-bit register)";
        return string("unknown ") + role + " register '" + name + "'";
    }

    // Worklet 1 label NAME -> Worklet 2 label ID (via the label's address).
    bool labelId(const string& name, int line, w2::LabelId& id) {
        unordered_map<string, Symbol>::const_iterator s = ir.symbolTable.find(name);
        if (s == ir.symbolTable.end()) return fail("undefined symbol '" + name + "'", line);
        id = idByAddr[s->second.address];
        return true;
    }

    // Worklet 1 operand (REG / IMM / SYM / MEM) -> Worklet 2 operand.
    bool operand(const ::Operand& in, int line, bool symIsMemory, w2::Operand& out) {
        if (in.kind == "REG") {
            w2::Reg r; uint8_t bits;
            if (!lookupRegSized(in.value, r, bits)) return fail("unknown register '" + in.value + "'", line);
            out = w2::regOp(r, bits);
            return true;
        }
        if (in.kind == "IMM") {
            // Worklet 1 keeps 0..0xFFFFFFFF and -2^31..-1; reinterpret as 32-bit two's complement.
            out = w2::immOp(static_cast<int32_t>(static_cast<uint32_t>(in.number)));
            return true;
        }
        if (in.kind == "SYM") {
            w2::LabelId id;
            if (!labelId(in.value, line, id)) return false;
            out = symIsMemory ? w2::labelMemOp(id) : w2::labelOp(id);
            return true;
        }
        if (in.kind == "MEM") {
            string why;
            if (memExprProblem(in.value, why)) return fail(why + " ('" + in.value + "')", line);
            // Worklet 1 accumulates the displacement in 64 bits; it must fit 32 bits.
            if (in.displacement < -2147483648LL || in.displacement > 4294967295LL)
                return fail("memory displacement out of 32-bit range in '" + in.value + "'", line);

            const bool hasSym = !in.symbol.empty();
            const int32_t disp = static_cast<int32_t>(static_cast<uint32_t>(in.displacement));

            // [label]  -- the original, unchanged path
            if (hasSym && in.baseReg.empty() && in.indexReg.empty() && in.displacement == 0) {
                w2::LabelId id;
                if (!labelId(in.symbol, line, id)) return false;
                out = w2::labelMemOp(id, memBits(in));
                return true;
            }

            w2::Reg base = w2::Reg::NONE;
            w2::Reg index = w2::Reg::NONE;
            if (!in.baseReg.empty()) {
                if (!lookupReg(in.baseReg, base)) return fail(addrRegError("base", in.baseReg), line);
            }
            if (!in.indexReg.empty()) {
                if (!lookupReg(in.indexReg, index)) return fail(addrRegError("index", in.indexReg), line);
            }
            uint8_t sc = static_cast<uint8_t>(in.scale ? in.scale : 1);

            // ESP cannot be an index, but "[eax+esp]" == "[esp+eax]" (scale 1): swap, as real
            // assemblers do. Skipped when the swap would change the size Worklet 1 already
            // fixed (EBP as base needs a displacement byte; ESP alone without a base).
            if (index == w2::Reg::ESP && sc == 1 && base != w2::Reg::NONE &&
                base != w2::Reg::ESP && (base != w2::Reg::EBP || hasSym)) {
                index = base;
                base  = w2::Reg::ESP;
            }

            if (hasSym) {                                   // [label + base + index*scale + disp]
                w2::LabelId id;
                if (!labelId(in.symbol, line, id)) return false;
                out = w2::labelMemOpEx(id, base, index, sc, disp, memBits(in));
                return true;
            }
            out = w2::memOp(base, index, sc, disp, memBits(in));
            return true;
        }
        return fail("unknown operand kind '" + in.kind + "'", line);
    }

    // LOAD/STORE use a register as an address ([reg]) or a label as an absolute address ([label]).
    bool addressOperand(const ::Operand& in, int line, w2::Operand& out) {
        if (in.kind == "REG") {
            w2::Reg r;
            if (!lookupReg(in.value, r)) return fail(addrRegError("address", in.value), line);
            out = w2::memOp(r, w2::Reg::NONE, 1, 0);
            return true;
        }
        return operand(in, line, true, out);
    }

    bool lower(vector<w2::Instruction>& code, w2::LabelId& endLabel) {
        const vector< ::Instruction >& src = ir.instructions;
        endLabel = w2::NO_LABEL;

        // 1. Dense label IDs. All names at one address share one ID, and IDs are
        //    assigned in address order so the result doesn't depend on hash order.
        for (unordered_map<string, Symbol>::const_iterator it = ir.symbolTable.begin();
             it != ir.symbolTable.end(); ++it)
            idByAddr[it->second.address] = 0;
        w2::LabelId next = 0;
        for (map<unsigned, w2::LabelId>::iterator it = idByAddr.begin(); it != idByAddr.end(); ++it)
            it->second = next++;

        // 2. Instructions.
        code.assign(src.size(), w2::Instruction());
        map<unsigned, size_t> instrAt;                      // address -> instruction index
        for (size_t i = 0; i < src.size(); ++i) {
            const ::Instruction& in = src[i];
            instrAt[in.address] = i;

            w2::Instruction& out = code[i];
            out = w2::Instruction();
            out.defLabel = w2::NO_LABEL;
            out.operandCount = static_cast<uint8_t>(in.operands.size());
            if (in.operands.size() > 2) return fail("more than 2 operands", in.line);

            if (in.mnemonic == "LOAD" || in.mnemonic == "STORE") {
                if (in.operands.size() != 2) return fail(in.mnemonic + " needs 2 operands", in.line);
                out.op = w2::Op::MOV;
                const bool isLoad = (in.mnemonic == "LOAD");
                if (isLoad) {      // LOAD reg, [reg|label]  ->  MOV reg, [..]
                    if (!operand(in.operands[0], in.line, false, out.operands[0])) return false;
                    if (!addressOperand(in.operands[1], in.line, out.operands[1])) return false;
                } else {           // STORE [reg|label], reg ->  MOV [..], reg
                    if (!addressOperand(in.operands[0], in.line, out.operands[0])) return false;
                    if (!operand(in.operands[1], in.line, false, out.operands[1])) return false;
                }
                continue;
            }

            if (in.mnemonic == "LODS" || in.mnemonic == "STOS") {
                // Size comes from the optional operand (AL/AX/EAX or BYTE/WORD/DWORD [..]); default dword.
                unsigned bits = 32;
                if (in.operands.size() == 1) {
                    const ::Operand& o = in.operands[0];
                    w2::Reg r; uint8_t rb;
                    if (o.kind == "REG") {
                        if (!lookupRegSized(o.value, r, rb))
                            return fail(in.mnemonic + ": unsupported register '" + o.value + "'", in.line);
                        bits = rb;
                    }
                    else if (o.kind == "MEM" && o.size) bits = static_cast<unsigned>(o.size);
                }
                const bool lods = (in.mnemonic == "LODS");
                if      (bits == 8)  out.op = lods ? w2::Op::LODSB : w2::Op::STOSB;
                else if (bits == 16) out.op = lods ? w2::Op::LODSW : w2::Op::STOSW;
                else if (bits == 32) out.op = lods ? w2::Op::LODSD : w2::Op::STOSD;
                else return fail(in.mnemonic + " operand size must be 8, 16 or 32 bits", in.line);
                out.operandCount = 0;      // the operand only selects the size
                continue;
            }

            if (!lookupOp(in.mnemonic, out.op))
                return fail("unsupported mnemonic '" + in.mnemonic + "'", in.line);
            for (size_t k = 0; k < in.operands.size(); ++k) {
                // LEA reg, label  ->  LEA reg, [label]  (the label is the memory operand whose address is wanted)
                const bool leaLabel = (in.mnemonic == "LEA" && k == 1);
                if (!operand(in.operands[k], in.line, leaLabel, out.operands[k])) return false;
            }
        }

        // 3. Attach label definitions to the instruction that starts at their address.
        for (map<unsigned, w2::LabelId>::const_iterator it = idByAddr.begin(); it != idByAddr.end(); ++it) {
            if (it->first == ir.finalLC) { endLabel = it->second; continue; }   // end-of-code label
            map<unsigned, size_t>::const_iterator at = instrAt.find(it->first);
            if (at == instrAt.end())
                return fail("label address does not coincide with an instruction start", 0);
            code[at->second].defLabel = it->second;
        }
        return true;
    }
};

// Worklet 1's Pass 1 fixed every address and length up front. If Worklet 2 emits
// something different, every later label/jump would be wrong, so check it.
bool crossCheck(const Worklet1Output& ir, const vector<size_t>& offs, size_t totalBytes) {
    bool ok = true;
    for (size_t i = 0; i < ir.instructions.size(); ++i) {
        const ::Instruction& in = ir.instructions[i];
        const size_t got = offs[i + 1] - offs[i];
        if (offs[i] != in.address || got != static_cast<size_t>(in.length)) {
            std::fprintf(stderr,
                "size mismatch at line %d (%s): Worklet 1 expected %d byte(s) at 0x%X, "
                "Worklet 2 emitted %lu byte(s) at 0x%lX\n",
                in.line, in.mnemonic.c_str(), in.length, in.address,
                static_cast<unsigned long>(got), static_cast<unsigned long>(offs[i]));
            ok = false;
        }
    }
    if (totalBytes != ir.finalLC) {
        std::fprintf(stderr, "total size mismatch: Worklet 1 says %u, Worklet 2 produced %lu\n",
                     ir.finalLC, static_cast<unsigned long>(totalBytes));
        ok = false;
    }
    return ok;
}

void printListing(const Worklet1Output& ir, const vector<size_t>& offs,
                  const vector<uint8_t>& bytes) {
    std::printf("LISTING\n-------------------------\n");
    for (size_t i = 0; i < ir.instructions.size(); ++i) {
        const ::Instruction& in = ir.instructions[i];
        std::string hex;
        char buf[4];
        for (size_t k = offs[i]; k < offs[i + 1]; ++k) {
            std::snprintf(buf, sizeof buf, "%02X ", bytes[k]);
            hex += buf;
        }
        std::string text = in.mnemonic;
        for (size_t k = 0; k < in.operands.size(); ++k)
            text += (k ? ", " : " ") + in.operands[k].value;
        std::printf("%08lX  %-21s %s\n", static_cast<unsigned long>(offs[i]), hex.c_str(), text.c_str());
    }
}

} // namespace

// ---------------------------------------------------------------------------
int main(int argc, char** argv) {
    if (argc < 2) {
        std::fprintf(stderr, "Usage: %s <file.asm> [out.bin] [baseAddress]\n", argv[0]);
        return 1;
    }
    const char* outPath = (argc > 2) ? argv[2] : "out.bin";
    // Load address added to [label] operands (LOAD/STORE). Default 0 = same as Worklet 1's addresses.
    uint32_t base = 0u;
    if (argc > 3) {
        char* endp = 0;
        errno = 0;
        const unsigned long long v = std::strtoull(argv[3], &endp, 0);
        if (endp == argv[3] || *endp != '\0' || errno == ERANGE || v > 0xFFFFFFFFull ||
            argv[3][0] == '-') {
            std::fprintf(stderr, "invalid base address '%s' (expected a number 0..0xFFFFFFFF)\n", argv[3]);
            return 1;
        }
        base = static_cast<uint32_t>(v);
    }

    // ---- Worklet 1 (unchanged) ----
    vector<string> sourceLines;
    if (!readSourceFile(argv[1], sourceLines)) {
        std::fprintf(stderr, "could not open '%s'\n", argv[1]);
        return 1;
    }
    Worklet1Output ir = runWorklet1(sourceLines);
    if (!ir.ok) {
        printErrors(ir.errors);
        return 1;
    }

    // ---- Adapter: strings -> enums / integer label IDs ----
    Lowering lowering(ir);
    vector<w2::Instruction> code;
    w2::LabelId endLabel;
    if (!lowering.lower(code, endLabel)) {
        std::fprintf(stderr, "adapter error%s%.0d: %s\n",
                     lowering.err.line ? " at line " : "", lowering.err.line,
                     lowering.err.message.c_str());
        return 1;
    }
    const w2::Program prog = w2::makeProgram(code.empty() ? 0 : &code[0], code.size(),
                                             static_cast<uint32_t>(lowering.idByAddr.size()),
                                             base, endLabel);

    // ---- Worklet 2 ----
    vector<uint8_t> bytes;
    vector<size_t>  offs;
    w2::Result r = w2::encode(prog, bytes, &offs);
    if (r.status != w2::Status::OK) {
        const bool atEnd = r.instrIndex >= ir.instructions.size();
        std::fprintf(stderr, "encode failed: %s (%s",
                     w2::statusText(r.status), atEnd ? "end of code" : "line ");
        if (!atEnd) std::fprintf(stderr, "%d", ir.instructions[r.instrIndex].line);
        std::fprintf(stderr, ")\n");
        return 1;
    }
    if (!crossCheck(ir, offs, bytes.size())) return 1;

    // ---- Hand the .bin to Worklet 3 ----
    if (!w2::writeBinFile(outPath, bytes)) {
        std::fprintf(stderr, "could not write %s\n", outPath);
        return 1;
    }
    printListing(ir, offs, bytes);
    std::printf("\nwrote %lu bytes to %s\n", static_cast<unsigned long>(bytes.size()), outPath);
    return 0;
}
