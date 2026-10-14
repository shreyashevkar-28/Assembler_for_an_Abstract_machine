// #define main worklet1_main
// #include "worklet1.cpp"
#undef main

#include "worklet2.h"

#include <algorithm>
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
    return false;   // LOAD / STORE are handled separately
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