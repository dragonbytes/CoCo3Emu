#pragma once
//#include "doublebyte.h"

class GimeBus;

// Constants for internally referencing the different addressing modes
constexpr int ADDR_INHERENT = 0;
constexpr int ADDR_IMMEDIATE_BYTE = 1;
constexpr int ADDR_IMMEDIATE_WORD = 2;
constexpr int ADDR_DIRECT = 3;
constexpr int ADDR_INDEXED_BYTE = 4;
constexpr int ADDR_INDEXED_WORD = 5;
constexpr int ADDR_INDEXED_DWORD = 6;
constexpr int ADDR_EXTENDED = 7;
constexpr int ADDR_RELATIVE_BYTE = 8;
constexpr int ADDR_RELATIVE_WORD = 9;

// Constants that define 6809 CPU interrupt vectors
constexpr uint16_t VECTOR_RESET = 0xFFFE;
constexpr uint16_t VECTOR_NMI = 0xFFFC;
constexpr uint16_t VECTOR_SWI = 0xFFFA;
constexpr uint16_t VECTOR_IRQ = 0xFFF8;
constexpr uint16_t VECTOR_FIRQ = 0xFFF6;
constexpr uint16_t VECTOR_SWI2 = 0xFFF4;
constexpr uint16_t VECTOR_SWI3 = 0xFFF2;

constexpr uint8_t INT_SWI3 = 0;
constexpr uint8_t INT_SWI2 = 1;
constexpr uint8_t INT_FIRQ = 2;
constexpr uint8_t INT_IRQ = 3;
constexpr uint8_t INT_SWI = 4;
constexpr uint8_t INT_NMI = 5;
constexpr uint8_t INT_RESET = 6;

// Constants that define bit positions for registers within the Postbyte when pushing/pulling from a stack
constexpr uint8_t MASK_PC = 0x80;
constexpr uint8_t MASK_SU = 0x40;
constexpr uint8_t MASK_Y = 0x20;
constexpr uint8_t MASK_X = 0x10;
constexpr uint8_t MASK_DP = 0x08;
constexpr uint8_t MASK_B = 0x04;
constexpr uint8_t MASK_A = 0x02;
constexpr uint8_t MASK_CC = 0x01;

constexpr char indexRegName[4] = { 'X', 'Y', 'U', 'S' };

// Note: Some compilers may not handle unions of `	`	single-bits correctly
union ConditionCodeRegister
{
	struct
	{
		bool C : 1;
		bool V : 1;
		bool Z : 1;
		bool N : 1;
		bool I : 1;
		bool H : 1;
		bool F : 1;
		bool E : 1;
	};
	uint8_t Byte;
};

union doubleAccumulator
{
	struct
	{
		// Because PC's use little-endian, B comes before A in a 16-bit word
		uint8_t B;
		uint8_t A;
	};
	uint16_t D;
};

struct registersStruct
{
	ConditionCodeRegister CC;
	doubleAccumulator Acc;
	uint8_t DP;
	uint16_t X;
	uint16_t Y;
	uint16_t U;
	uint16_t S;
	uint16_t PC;
};

class Cpu6809
{
public:
	Cpu6809();

public:
	uint8_t assertedInterrupts[7];

	void ConnectBus(GimeBus* busPtr) { gimeBus = busPtr; }
	void cpuClockTick();

private:
	uint8_t readByteAtCurPC();
	uint16_t readWordAtCurPC();
	void printDebugMsgs();

	void ABX(int),   ADCA(int), ADCB(int), ADDA(int), ADDB(int), ADDD(int), ANDA(int), ANDB(int);
	void ANDCC(int), ASL(int),	ASRA(int), ASRB(int), ASR(int),	 BITA(int), BITB(int), CLRA(int);
	void CLRB(int),	 CLR(int),  CMPA(int), CMPB(int), CMPD(int), CMPS(int), CMPU(int), CMPX(int);
	void CMPY(int),  COMA(int), COMB(int), COM(int),  CWAI(int), DAA(int),  DECA(int), DECB(int);
	void DEC(int),   EORA(int), EORB(int), EXG(int),  INCA(int), INCB(int), INC(int),  JMP(int);
	void JSR(int),   LDA(int),  LDB(int),  LDD(int),  LDS(int),  LDU(int),  LDX(int),  LDY(int);
	void LEAS(int),  LEAU(int), LEAX(int), LEAY(int), LSLA(int), LSLB(int), LSL(int),  LSRA(int);
	void LSRB(int),  LSR(int),  MUL(int),  NEGA(int), NEGB(int), NEG(int),  NOP(int),  ORA(int);
	void ORB(int),   ORCC(int), PSHS(int), PSHU(int), PULS(int), PULU(int), ROLA(int), ROLB(int);
	void ROL(int),   RORA(int), RORB(int), ROR(int),  RTI(int),  RTS(int),  SBCA(int), SBCB(int);
	void SEX(int),   STA(int),  STB(int),  STD(int),  STS(int),  STU(int),  STX(int),  STY(int);
	void SUBA(int),  SUBB(int), SUBD(int), SWI(int),  SWI2(int), SWI3(int), SYNC(int), TFR(int);
	void TSTA(int),  TSTB(int), TST(int);

	void BCC(int),  BCS(int), BEQ(int), BGE(int), BGT(int), BHI(int), BLE(int);
	void BLS(int),	BLT(int), BMI(int), BNE(int), BPL(int), BRA(int), BRN(int), BSR(int);
	void BVC(int),	BVS(int), LBEQ(int),LBGE(int),LBGT(int),LBHI(int),LBCC(int);
	void LBLE(int), LBCS(int),LBLS(int),LBLT(int),LBMI(int),LBNE(int),LBPL(int),LBRA(int);
	void LBRN(int), LBSR(int),LBVC(int),LBVS(int);

	int addrModeInherent();
	int addrModeImmediateByte();
	int addrModeImmediateWord();
	int addrModeImmediateRegs();
	int addrModeImmediateStack();
	int addrModeDirect();
	int addrModeExtended();
	int addrModeIndexed();
	int addrModeRelativeByte();
	int addrModeRelativeWord();
	int invalidOpCode();
	/*
	char* asmAddrModeInherent();
	char* asmAddrModeImmediateByte();
	char* asmAddrModeImmediateWord();
	char* asmAddrModeImmediateRegs();
	char* asmAddrModeImmediateStack();
	char* asmAddrModeDirect();
	char* asmAddrModeExtended();
	char* asmAddrModeIndexed();
	char* asmAddrModeRelativeByte();
	char* asmAddrModeRelativeWord();
	char* asmInvalidOpCode();
	*/

	uint8_t getParamByte(int);
	uint16_t getParamWord(int);
	void pushToStack(uint16_t*);
	void pullFromStack(uint16_t*);

	GimeBus* gimeBus = nullptr;
	registersStruct cpuReg;
	bool waitingForNextOp, cpuHaltFlag, interruptInProgress, debugStepMode;
	unsigned int nextOpWaitCounter;
	uint16_t effectiveAddr, cpuCyclesTotal;
	uint8_t curOpCode, operandByte, paramByte, registerByte, resultByte, invalidRegByte = 0xFF;
	uint16_t operandWord, registerWord, paramWord, resultWord, debuggerRegPC, invalidRegWord = 0xFFFF;
	uint32_t resultDWord;
	int curOpCodeCycleCount, curAddrMode;
	char curOpAddressString[16];
	std::string curOpMnemonic;
	unsigned int instructionTotalCounter = 0;
	FILE* traceListFile = nullptr;
	std::string interRegNames[16] = { "D", "X", "Y", "U", "S", "PC", "W", "V", "A", "B", "CC", "DP", "UNDEFINED", "UNDEFINED", "E", "F" };
	
	bool debuggerStepEnabled = false;

	uint16_t* indexedRegOrder[4] = { &cpuReg.X, &cpuReg.Y, &cpuReg.U, &cpuReg.S };
	uint16_t* interRegWordOrder[8] = { &cpuReg.Acc.D, &cpuReg.X, &cpuReg.Y, &cpuReg.U, &cpuReg.S, &cpuReg.PC, &invalidRegWord, &invalidRegWord };
	uint8_t* interRegByteOrder[8] = { &cpuReg.Acc.A, &cpuReg.Acc.B, &cpuReg.CC.Byte, &cpuReg.DP, &invalidRegByte, &invalidRegByte, &invalidRegByte, &invalidRegByte };

	struct instructionsTable
	{
		std::string mnemonicName;
		int opBaseCycles;
		int (Cpu6809::*addrModePtr)(void) = nullptr;
		//char* (Cpu6809::*disasmAddrModePtr)(void) = nullptr;
		void (Cpu6809::*execOpPtr)(int) = nullptr;
	};

	using c = Cpu6809;
	#define INH &Cpu6809::addrModeInherent
	#define IM1 &Cpu6809::addrModeImmediateByte
	#define IM2 &Cpu6809::addrModeImmediateWord
	#define IMR &Cpu6809::addrModeImmediateRegs
	#define IMS &Cpu6809::addrModeImmediateStack
	#define DIR &Cpu6809::addrModeDirect
	#define EXT &Cpu6809::addrModeExtended
	#define IDX &Cpu6809::addrModeIndexed
	#define RL1 &Cpu6809::addrModeRelativeByte
	#define RL2 &Cpu6809::addrModeRelativeWord
	#define ERR &Cpu6809::invalidOpCode
	#define NUL nullptr

	/*
	#define ASMINH &Cpu6809::asmAddrModeInherent
	#define ASMIM1 &Cpu6809::asmAddrModeImmediateByte
	#define ASMIM2 &Cpu6809::asmAddrModeImmediateWord
	#define ASMIMR &Cpu6809::asmAddrModeImmediateRegs
	#define ASMIMS &Cpu6809::asmAddrModeImmediateStack
	#define ASMDIR &Cpu6809::asmAddrModeDirect
	#define ASMEXT &Cpu6809::asmAddrModeExtended
	#define ASMIDX &Cpu6809::asmAddrModeIndexed
	#define ASMRL1 &Cpu6809::asmAddrModeRelativeByte
	#define ASMRL2 &Cpu6809::asmAddrModeRelativeWord
	#define ASMERR &Cpu6809::asmInvalidOpCode
	*/

	void extendedOpCodePtrs(uint16_t);
	instructionsTable extendedOpCodeObjects;
	/*
	instructionsTable mainOpCodeLookup[256] =
	{	{"NEG",6,DIR,ASMDIR,&c::NEG},	{"???",0,ERR,ASMERR,NUL},		{"???",0,ERR,ASMERR,NUL},		{"COM",6,DIR,ASMDIR,&c::COM},	{"LSR",6,DIR,ASMDIR,&c::LSR},		{"???",0,ERR,ASMERR,NUL},		{"ROR",6,DIR,ASMDIR,&c::ROR},	{"ASR",6,DIR,ASMDIR,&c::ASR},
		{"LSL",6,DIR,ASMDIR,&c::LSL},	{"ROL",6,DIR,ASMDIR,&c::ROL},	{"DEC",6,DIR,ASMDIR,&c::DEC},	{"???",0,ERR,ASMERR,NUL},		{"INC",6,DIR,ASMDIR,&c::INC},		{"TST",6,DIR,ASMDIR,&c::TST},	{"JMP",3,DIR,ASMDIR,&c::JMP},	{"CLR",6,DIR,ASMDIR,&c::CLR},
		{"PAGE2",0,ERR,ASMERR,NUL},		{"PAGE3",0,ERR,ASMERR,NUL},		{"NOP",2,INH,ASMINH,&c::NOP},	{"SYNC",4,INH,ASMINH,&c::SYNC},	{"???",0,ERR,ASMERR,NUL},			{"???",0,ERR,ASMERR,NUL},		{"LBRA",5,RL2,ASMRL2,&c::LBRA},	{"LBSR",9,RL2,ASMRL2,&c::LBSR},
		{"???",0,ERR,ASMERR,NUL},		{"DAA",2,INH,ASMINH,&c::DAA},	{"ORCC",3,IM1,ASMIM1,&c::ORCC},	{"???",0,ERR,ASMERR,NUL},		{"ANDCC",3,IM1,ASMIM1,&c::ANDCC},	{"SEX",2,INH,ASMINH,&c::SEX},	{"EXG",8,IMR,ASMIMR,&c::EXG},	{"TFR",6,IMR,ASMIMR,&c::TFR},
		{"BRA",3,RL1,ASMRL1,&c::BRA},	{"BRN",3,RL1,ASMRL1,&c::BRN},	{"BHI",3,RL1,ASMRL1,&c::BHI},	{"BLS",3,RL1,ASMRL1,&c::BLS},	{"BCC",3,RL1,ASMRL1,&c::BCC},		{"BCS",3,RL1,ASMRL1,&c::BCS},	{"BNE",3,RL1,ASMRL1,&c::BNE},	{"BEQ",3,RL1,ASMRL1,&c::BEQ},
		{"BVC",3,RL1,ASMRL1,&c::BVC},	{"BVS",3,RL1,ASMRL1,&c::BVS},	{"BPL",3,RL1,ASMRL1,&c::BPL},	{"BMI",3,RL1,ASMRL1,&c::BMI},	{"BGE",3,RL1,ASMRL1,&c::BGE},		{"BLT",3,RL1,ASMRL1,&c::BLT},	{"BGT",3,RL1,ASMRL1,&c::BGT},	{"BLE",3,RL1,ASMRL1,&c::BLE},
		{"LEAX",4,IDX,ASMIDX,&c::LEAX},	{"LEAY",4,IDX,ASMIDX,&c::LEAY},	{"LEAS",4,IDX,ASMIDX,&c::LEAS},	{"LEAU",4,IDX,ASMIDX,&c::LEAU},	{"PSHS",5,IMS,ASMIMS,&c::PSHS},		{"PULS",5,IMS,ASMIMS,&c::PULS},	{"PSHU",5,IMS,ASMIMS,&c::PSHU},	{"PULU",5,IMS,ASMIMS,&c::PULU},
		{"???",0,ERR,ASMERR,NUL},		{"RTS",5,INH,ASMINH,&c::RTS},	{"ABX",3,INH,ASMINH,&c::ABX},	{"RTI",6,INH,ASMINH,&c::RTI},	{"CWAI",22,IM1,ASMIM1,&c::CWAI},	{"MUL",11,INH,ASMINH,&c::MUL},	{"???",0,ERR,ASMERR,NUL},		{"SWI",19,INH,ASMINH,&c::SWI},
		{"NEGA",2,INH,ASMINH,&c::NEGA},	{"???",0,ERR,ASMERR,NUL},		{"???",0,ERR,ASMERR,NUL},		{"COMA",2,INH,ASMINH,&c::COMA},	{"LSRA",2,INH,ASMINH,&c::LSRA},		{"???",0,ERR,ASMERR,NUL},		{"RORA",2,INH,ASMINH,&c::RORA},	{"ASRA",2,INH,ASMINH,&c::ASRA},
		{"LSLA",2,INH,ASMINH,&c::LSLA},	{"ROLA",2,INH,ASMINH,&c::ROLA},	{"DECA",2,INH,ASMINH,&c::DECA},	{"???",0,ERR,ASMERR,NUL},		{"INCA",2,INH,ASMINH,&c::INCA},		{"TSTA",2,INH,ASMINH,&c::TSTA},	{"???",0,ERR,ASMERR,NUL},		{"CLRA",2,INH,ASMINH,&c::CLRA},
		{"NEGB",2,INH,ASMINH,&c::NEGB},	{"???",0,ERR,ASMERR,NUL},		{"???",0,ERR,ASMERR,NUL},		{"COMB",2,INH,ASMINH,&c::COMB},	{"LSRB",2,INH,ASMINH,&c::LSRB},		{"???",0,ERR,ASMERR,NUL},		{"RORB",2,INH,ASMINH,&c::RORB},	{"ASRB",2,INH,ASMINH,&c::ASRB},
		{"LSLB",2,INH,ASMINH,&c::LSLB},	{"ROLB",2,INH,ASMINH,&c::ROLB},	{"DECB",2,INH,ASMINH,&c::DECB},	{"???",0,ERR,ASMERR,NUL},		{"INCB",2,INH,ASMINH,&c::INCB},		{"TSTB",2,INH,ASMINH,&c::TSTB},	{"???",0,ERR,ASMERR,NUL},		{"CLRB",2,INH,ASMINH,&c::CLRB},
		{"NEG",6,IDX,ASMIDX,&c::NEG},	{"???",0,ERR,ASMERR,NUL},		{"???",0,ERR,ASMERR,NUL},		{"COM",6,IDX,ASMIDX,&c::COM},	{"LSR",6,IDX,ASMIDX,&c::LSR},		{"???",0,ERR,ASMERR,NUL},		{"ROR",6,IDX,ASMIDX,&c::ROR},	{"ASR",6,IDX,ASMIDX,&c::ASR},
		{"ASL",6,IDX,ASMIDX,&c::ASL},	{"ROL",6,IDX,ASMIDX,&c::ROL},	{"DEC",6,IDX,ASMIDX,&c::DEC},	{"???",0,ERR,ASMERR,NUL},		{"INC",6,IDX,ASMIDX,&c::INC},		{"TST",6,IDX,ASMIDX,&c::TST},	{"JMP",3,IDX,ASMIDX,&c::JMP},	{"CLR",6,IDX,ASMIDX,&c::CLR},
		{"NEG",7,EXT,ASMEXT,&c::NEG},	{"???",0,ERR,ASMERR,NUL},		{"???",0,ERR,ASMERR,NUL},		{"COM",7,EXT,ASMEXT,&c::COM},	{"LSR",7,EXT,ASMEXT,&c::LSR},		{"???",0,ERR,ASMERR,NUL},		{"ROR",7,EXT,ASMEXT,&c::ROR},	{"ASR",7,EXT,ASMEXT,&c::ASR},
		{"ASL",7,EXT,ASMEXT,&c::ASL},	{"ROL",7,EXT,ASMEXT,&c::ROL},	{"DEC",7,EXT,ASMEXT,&c::DEC},	{"???",0,ERR,ASMERR,NUL},		{"INC",7,EXT,ASMEXT,&c::INC},		{"TST",7,EXT,ASMEXT,&c::TST},	{"JMP",4,EXT,ASMEXT,&c::JMP},	{"CLR",7,EXT,ASMEXT,&c::CLR},
		{"SUBA",2,IM1,ASMIM1,&c::SUBA},	{"CMPA",2,IM1,ASMIM1,&c::CMPA},	{"SBCA",2,IM1,ASMIM1,&c::SBCA},	{"SUBD",4,IM2,ASMIM2,&c::SUBD},	{"ANDA",2,IM1,ASMIM1,&c::ANDA},		{"BITA",2,IM1,ASMIM1,&c::BITA},	{"LDA",2,IM1,ASMIM1,&c::LDA},	{"???",0,ERR,ASMERR,NUL},
		{"EORA",2,IM1,ASMIM1,&c::EORA},	{"ADCA",2,IM1,ASMIM1,&c::ADCA},	{"ORA",2,IM1,ASMIM1,&c::ORA},	{"ADDA",2,IM1,ASMIM1,&c::ADDA},	{"CMPX",4,IM2,ASMIM2,&c::CMPX},		{"BSR",7,RL1,ASMRL1,&c::BSR},	{"LDX",3,IM2,ASMIM2,&c::LDX},	{"???",0,ERR,ASMERR,NUL},
		{"SUBA",4,DIR,ASMDIR,&c::SUBA},	{"CMPA",4,DIR,ASMDIR,&c::CMPA},	{"SBCA",4,DIR,ASMDIR,&c::SBCA},	{"SUBD",6,DIR,ASMDIR,&c::SUBD},	{"ANDA",4,DIR,ASMDIR,&c::ANDA},		{"BITA",4,DIR,ASMDIR,&c::BITA},	{"LDA",4,DIR,ASMDIR,&c::LDA},	{"STA",4,DIR,ASMDIR,&c::STA},
		{"EORA",4,DIR,ASMDIR,&c::EORA},	{"ADCA",4,DIR,ASMDIR,&c::ADCA},	{"ORA",4,DIR,ASMDIR,&c::ORA},	{"ADDA",4,DIR,ASMDIR,&c::ADDA},	{"CMPX",6,DIR,ASMDIR,&c::CMPX},		{"JSR",7,DIR,ASMDIR,&c::JSR},	{"LDX",5,DIR,ASMDIR,&c::LDX},	{"STX",5,DIR,ASMDIR,&c::STX},
		{"SUBA",4,IDX,ASMIDX,&c::SUBA},	{"CMPA",4,IDX,ASMIDX,&c::CMPA},	{"SBCA",4,IDX,ASMIDX,&c::SBCA},	{"SUBD",6,IDX,ASMIDX,&c::SUBD},	{"ANDA",4,IDX,ASMIDX,&c::ANDA},		{"BITA",4,IDX,ASMIDX,&c::BITA},	{"LDA",4,IDX,ASMIDX,&c::LDA},	{"STA",4,IDX,ASMIDX,&c::STA},
		{"EORA",4,IDX,ASMIDX,&c::EORA},	{"ADCA",4,IDX,ASMIDX,&c::ADCA},	{"ORA",4,IDX,ASMIDX,&c::ORA},	{"ADDA",4,IDX,ASMIDX,&c::ADDA},	{"CMPX",6,IDX,ASMIDX,&c::CMPX},		{"JSR",7,IDX,ASMIDX,&c::JSR},	{"LDX",5,IDX,ASMIDX,&c::LDX},	{"STX",5,IDX,ASMIDX,&c::STX},
		{"SUBA",5,EXT,ASMEXT,&c::SUBA},	{"CMPA",5,EXT,ASMEXT,&c::CMPA},	{"SBCA",5,EXT,ASMEXT,&c::SBCA},	{"SUBD",7,EXT,ASMEXT,&c::SUBD},	{"ANDA",5,EXT,ASMEXT,&c::ANDA},		{"BITA",5,EXT,ASMEXT,&c::BITA},	{"LDA",5,EXT,ASMEXT,&c::LDA},	{"STA",5,EXT,ASMEXT,&c::STA},
		{"EORA",5,EXT,ASMEXT,&c::EORA},	{"ADCA",5,EXT,ASMEXT,&c::ADCA},	{"ORA",5,EXT,ASMEXT,&c::ORA},	{"ADDA",5,EXT,ASMEXT,&c::ADDA},	{"CMPX",7,EXT,ASMEXT,&c::CMPX},		{"JSR",8,EXT,ASMEXT,&c::JSR},	{"LDX",6,EXT,ASMEXT,&c::LDX},	{"STX",6,EXT,ASMEXT,&c::STX},
		{"SUBB",2,IM1,ASMIM1,&c::SUBB},	{"CMPB",2,IM1,ASMIM1,&c::CMPB},	{"SBCB",2,IM1,ASMIM1,&c::SBCB},	{"ADDD",4,IM2,ASMIM2,&c::ADDD},	{"ANDB",2,IM1,ASMIM1,&c::ANDB},		{"BITB",2,IM1,ASMIM1,&c::BITB},	{"LDB",2,IM1,ASMIM1,&c::LDB},	{"???",0,ERR,ASMERR,NUL},
		{"EORB",2,IM1,ASMIM1,&c::EORB},	{"ADCB",2,IM1,ASMIM1,&c::ADCB},	{"ORB",2,IM1,ASMIM1,&c::ORB},	{"ADDB",2,IM1,ASMIM1,&c::ADDB},	{"LDD",3,IM2,ASMIM2,&c::LDD},		{"???",0,ERR,ASMERR,NUL},		{"LDU",3,IM2,ASMIM2,&c::LDU},	{"???",0,ERR,ASMERR,NUL},
		{"SUBB",4,DIR,ASMDIR,&c::SUBB},	{"CMPB",4,DIR,ASMDIR,&c::CMPB},	{"SBCB",4,DIR,ASMDIR,&c::SBCB},	{"ADDD",6,DIR,ASMDIR,&c::ADDD},	{"ANDB",4,DIR,ASMDIR,&c::ANDB},		{"BITB",4,DIR,ASMDIR,&c::BITB},	{"LDB",4,DIR,ASMDIR,&c::LDB},	{"STB",4,DIR,ASMDIR,&c::STB},
		{"EORB",4,DIR,ASMDIR,&c::EORB},	{"ADCB",4,DIR,ASMDIR,&c::ADCB},	{"ORB",4,DIR,ASMDIR,&c::ORB},	{"ADDB",4,DIR,ASMDIR,&c::ADDB},	{"LDD",5,DIR,ASMDIR,&c::LDD},		{"STD",5,DIR,ASMDIR,&c::STD},	{"LDU",5,DIR,ASMDIR,&c::LDU},	{"STU",5,DIR,ASMDIR,&c::STU},
		{"SUBB",4,IDX,ASMIDX,&c::SUBB},	{"CMPB",4,IDX,ASMIDX,&c::CMPB},	{"SBCB",4,IDX,ASMIDX,&c::SBCB},	{"ADDD",6,IDX,ASMIDX,&c::ADDD},	{"ANDB",4,IDX,ASMIDX,&c::ANDB},		{"BITB",4,IDX,ASMIDX,&c::BITB},	{"LDB",4,IDX,ASMIDX,&c::LDB},	{"STB",4,IDX,ASMIDX,&c::STB},
		{"EORB",4,IDX,ASMIDX,&c::EORB},	{"ADCB",4,IDX,ASMIDX,&c::ADCB},	{"ORB",4,IDX,ASMIDX,&c::ORB},	{"ADDB",4,IDX,ASMIDX,&c::ADDB},	{"LDD",5,IDX,ASMIDX,&c::LDD},		{"STD",5,IDX,ASMIDX,&c::STD},	{"LDU",5,IDX,ASMIDX,&c::LDU},	{"STU",5,IDX,ASMIDX,&c::STU},
		{"SUBB",5,EXT,ASMEXT,&c::SUBB},	{"CMPB",5,EXT,ASMEXT,&c::CMPB},	{"SBCB",5,EXT,ASMEXT,&c::SBCB},	{"ADDD",7,EXT,ASMEXT,&c::ADDD},	{"ANDB",5,EXT,ASMEXT,&c::ANDB},		{"BITB",5,EXT,ASMEXT,&c::BITB},	{"LDB",5,EXT,ASMEXT,&c::LDB},	{"STB",5,EXT,ASMEXT,&c::STB},
		{"EORB",5,EXT,ASMEXT,&c::EORB},	{"ADCB",5,EXT,ASMEXT,&c::ADCB},	{"ORB",5,EXT,ASMEXT,&c::ORB},	{"ADDB",5,EXT,ASMEXT,&c::ADDB},	{"LDD",6,EXT,ASMEXT,&c::LDD},		{"STD",6,EXT,ASMEXT,&c::STD},	{"LDU",6,EXT,ASMEXT,&c::LDU},	{"STU",6,EXT,ASMEXT,&c::STU}
	};
	*/
	
	instructionsTable mainOpCodeLookup[256] =
	{	{"NEG  ",6,DIR,&c::NEG},	{"???  ",0,ERR,NUL},		{"???  ",0,ERR,NUL},		{"COM  ",6,DIR,&c::COM},	{"LSR  ",6,DIR,&c::LSR},	{"???  ",0,ERR,NUL},		{"ROR  ",6,DIR,&c::ROR},	{"ASR  ",6,DIR,&c::ASR},
		{"LSL  ",6,DIR,&c::LSL},	{"ROL  ",6,DIR,&c::ROL},	{"DEC  ",6,DIR,&c::DEC},	{"???  ",0,ERR,NUL},		{"INC  ",6,DIR,&c::INC},	{"TST  ",6,DIR,&c::TST},	{"JMP  ",3,DIR,&c::JMP},	{"CLR  ",6,DIR,&c::CLR},
		{"PAGE2",0,ERR,NUL},		{"PAGE3",0,ERR,NUL},		{"NOP  ",2,INH,&c::NOP},	{"SYNC ",4,INH,&c::SYNC},	{"???  ",0,ERR,NUL},		{"???  ",0,ERR,NUL},		{"LBRA ",5,RL2,&c::LBRA},	{"LBSR ",9,RL2,&c::LBSR},
		{"???  ",0,ERR,NUL},		{"DAA  ",2,INH,&c::DAA},	{"ORCC ",3,IM1,&c::ORCC},	{"???  ",0,ERR,NUL},		{"ANDCC",3,IM1,&c::ANDCC},	{"SEX  ",2,INH,&c::SEX},	{"EXG  ",8,IMR,&c::EXG},	{"TFR  ",6,IMR,&c::TFR},
		{"BRA  ",3,RL1,&c::BRA},	{"BRN  ",3,RL1,&c::BRN},	{"BHI  ",3,RL1,&c::BHI},	{"BLS  ",3,RL1,&c::BLS},	{"BCC  ",3,RL1,&c::BCC},	{"BCS  ",3,RL1,&c::BCS},	{"BNE  ",3,RL1,&c::BNE},	{"BEQ  ",3,RL1,&c::BEQ},
		{"BVC  ",3,RL1,&c::BVC},	{"BVS  ",3,RL1,&c::BVS},	{"BPL  ",3,RL1,&c::BPL},	{"BMI  ",3,RL1,&c::BMI},	{"BGE  ",3,RL1,&c::BGE},	{"BLT  ",3,RL1,&c::BLT},	{"BGT  ",3,RL1,&c::BGT},	{"BLE  ",3,RL1,&c::BLE},
		{"LEAX ",4,IDX,&c::LEAX},	{"LEAY ",4,IDX,&c::LEAY},	{"LEAS ",4,IDX,&c::LEAS},	{"LEAU ",4,IDX,&c::LEAU},	{"PSHS ",5,IMS,&c::PSHS},	{"PULS ",5,IMS,&c::PULS},	{"PSHU ",5,IMS,&c::PSHU},	{"PULU ",5,IMS,&c::PULU},
		{"???  ",0,ERR,NUL},		{"RTS  ",5,INH,&c::RTS},	{"ABX  ",3,INH,&c::ABX},	{"RTI  ",6,INH,&c::RTI},	{"CWAI ",22,IM1,&c::CWAI},	{"MUL  ",11,INH,&c::MUL},	{"???  ",0,ERR,NUL},		{"SWI  ",19,INH,&c::SWI},
		{"NEGA ",2,INH,&c::NEGA},	{"???  ",0,ERR,NUL},		{"???  ",0,ERR,NUL},		{"COMA ",2,INH,&c::COMA},	{"LSRA ",2,INH,&c::LSRA},	{"???  ",0,ERR,NUL},		{"RORA ",2,INH,&c::RORA},	{"ASRA ",2,INH,&c::ASRA},
		{"LSLA ",2,INH,&c::LSLA},	{"ROLA ",2,INH,&c::ROLA},	{"DECA ",2,INH,&c::DECA},	{"???  ",0,ERR,NUL},		{"INCA ",2,INH,&c::INCA},	{"TSTA ",2,INH,&c::TSTA},	{"???  ",0,ERR,NUL},		{"CLRA ",2,INH,&c::CLRA},
		{"NEGB ",2,INH,&c::NEGB},	{"???  ",0,ERR,NUL},		{"???  ",0,ERR,NUL},		{"COMB ",2,INH,&c::COMB},	{"LSRB ",2,INH,&c::LSRB},	{"???  ",0,ERR,NUL},		{"RORB ",2,INH,&c::RORB},	{"ASRB ",2,INH,&c::ASRB},
		{"LSLB ",2,INH,&c::LSLB},	{"ROLB ",2,INH,&c::ROLB},	{"DECB ",2,INH,&c::DECB},	{"???  ",0,ERR,NUL},		{"INCB ",2,INH,&c::INCB},	{"TSTB ",2,INH,&c::TSTB},	{"???  ",0,ERR,NUL},		{"CLRB ",2,INH,&c::CLRB},
		{"NEG  ",6,IDX,&c::NEG},	{"???  ",0,ERR,NUL},		{"???  ",0,ERR,NUL},		{"COM  ",6,IDX,&c::COM},	{"LSR  ",6,IDX,&c::LSR},	{"???  ",0,ERR,NUL},		{"ROR  ",6,IDX,&c::ROR},	{"ASR  ",6,IDX,&c::ASR},
		{"ASL  ",6,IDX,&c::ASL},	{"ROL  ",6,IDX,&c::ROL},	{"DEC  ",6,IDX,&c::DEC},	{"???  ",0,ERR,NUL},		{"INC  ",6,IDX,&c::INC},	{"TST  ",6,IDX,&c::TST},	{"JMP  ",3,IDX,&c::JMP},	{"CLR  ",6,IDX,&c::CLR},
		{"NEG  ",7,EXT,&c::NEG},	{"???  ",0,ERR,NUL},		{"???  ",0,ERR,NUL},		{"COM  ",7,EXT,&c::COM},	{"LSR  ",7,EXT,&c::LSR},	{"???  ",0,ERR,NUL},		{"ROR  ",7,EXT,&c::ROR},	{"ASR  ",7,EXT,&c::ASR},
		{"ASL  ",7,EXT,&c::ASL},	{"ROL  ",7,EXT,&c::ROL},	{"DEC  ",7,EXT,&c::DEC},	{"???  ",0,ERR,NUL},		{"INC  ",7,EXT,&c::INC},	{"TST  ",7,EXT,&c::TST},	{"JMP  ",4,EXT,&c::JMP},	{"CLR  ",7,EXT,&c::CLR},
		{"SUBA ",2,IM1,&c::SUBA},	{"CMPA ",2,IM1,&c::CMPA},	{"SBCA ",2,IM1,&c::SBCA},	{"SUBD ",4,IM2,&c::SUBD},	{"ANDA ",2,IM1,&c::ANDA},	{"BITA ",2,IM1,&c::BITA},	{"LDA  ",2,IM1,&c::LDA},	{"???  ",0,ERR,NUL},
		{"EORA ",2,IM1,&c::EORA},	{"ADCA ",2,IM1,&c::ADCA},	{"ORA  ",2,IM1,&c::ORA},	{"ADDA ",2,IM1,&c::ADDA},	{"CMPX ",4,IM2,&c::CMPX},	{"BSR  ",7,RL1,&c::BSR},	{"LDX  ",3,IM2,&c::LDX},	{"???  ",0,ERR,NUL},
		{"SUBA ",4,DIR,&c::SUBA},	{"CMPA ",4,DIR,&c::CMPA},	{"SBCA ",4,DIR,&c::SBCA},	{"SUBD ",6,DIR,&c::SUBD},	{"ANDA ",4,DIR,&c::ANDA},	{"BITA ",4,DIR,&c::BITA},	{"LDA  ",4,DIR,&c::LDA},	{"STA  ",4,DIR,&c::STA},
		{"EORA ",4,DIR,&c::EORA},	{"ADCA ",4,DIR,&c::ADCA},	{"ORA  ",4,DIR,&c::ORA},	{"ADDA ",4,DIR,&c::ADDA},	{"CMPX ",6,DIR,&c::CMPX},	{"JSR  ",7,DIR,&c::JSR},	{"LDX  ",5,DIR,&c::LDX},	{"STX  ",5,DIR,&c::STX},
		{"SUBA ",4,IDX,&c::SUBA},	{"CMPA ",4,IDX,&c::CMPA},	{"SBCA ",4,IDX,&c::SBCA},	{"SUBD ",6,IDX,&c::SUBD},	{"ANDA ",4,IDX,&c::ANDA},	{"BITA ",4,IDX,&c::BITA},	{"LDA  ",4,IDX,&c::LDA},	{"STA  ",4,IDX,&c::STA},
		{"EORA ",4,IDX,&c::EORA},	{"ADCA ",4,IDX,&c::ADCA},	{"ORA  ",4,IDX,&c::ORA},	{"ADDA ",4,IDX,&c::ADDA},	{"CMPX ",6,IDX,&c::CMPX},	{"JSR  ",7,IDX,&c::JSR},	{"LDX  ",5,IDX,&c::LDX},	{"STX  ",5,IDX,&c::STX},
		{"SUBA ",5,EXT,&c::SUBA},	{"CMPA ",5,EXT,&c::CMPA},	{"SBCA ",5,EXT,&c::SBCA},	{"SUBD ",7,EXT,&c::SUBD},	{"ANDA ",5,EXT,&c::ANDA},	{"BITA ",5,EXT,&c::BITA},	{"LDA  ",5,EXT,&c::LDA},	{"STA  ",5,EXT,&c::STA},
		{"EORA ",5,EXT,&c::EORA},	{"ADCA ",5,EXT,&c::ADCA},	{"ORA  ",5,EXT,&c::ORA},	{"ADDA ",5,EXT,&c::ADDA},	{"CMPX ",7,EXT,&c::CMPX},	{"JSR  ",8,EXT,&c::JSR},	{"LDX  ",6,EXT,&c::LDX},	{"STX  ",6,EXT,&c::STX},
		{"SUBB ",2,IM1,&c::SUBB},	{"CMPB ",2,IM1,&c::CMPB},	{"SBCB ",2,IM1,&c::SBCB},	{"ADDD ",4,IM2,&c::ADDD},	{"ANDB ",2,IM1,&c::ANDB},	{"BITB ",2,IM1,&c::BITB},	{"LDB  ",2,IM1,&c::LDB},	{"???  ",0,ERR,NUL},
		{"EORB ",2,IM1,&c::EORB},	{"ADCB ",2,IM1,&c::ADCB},	{"ORB  ",2,IM1,&c::ORB},	{"ADDB ",2,IM1,&c::ADDB},	{"LDD  ",3,IM2,&c::LDD},	{"???  ",0,ERR,NUL},		{"LDU  ",3,IM2,&c::LDU},	{"???  ",0,ERR,NUL},
		{"SUBB ",4,DIR,&c::SUBB},	{"CMPB ",4,DIR,&c::CMPB},	{"SBCB ",4,DIR,&c::SBCB},	{"ADDD ",6,DIR,&c::ADDD},	{"ANDB ",4,DIR,&c::ANDB},	{"BITB ",4,DIR,&c::BITB},	{"LDB  ",4,DIR,&c::LDB},	{"STB  ",4,DIR,&c::STB},
		{"EORB ",4,DIR,&c::EORB},	{"ADCB ",4,DIR,&c::ADCB},	{"ORB  ",4,DIR,&c::ORB},	{"ADDB ",4,DIR,&c::ADDB},	{"LDD  ",5,DIR,&c::LDD},	{"STD  ",5,DIR,&c::STD},	{"LDU  ",5,DIR,&c::LDU},	{"STU  ",5,DIR,&c::STU},
		{"SUBB ",4,IDX,&c::SUBB},	{"CMPB ",4,IDX,&c::CMPB},	{"SBCB ",4,IDX,&c::SBCB},	{"ADDD ",6,IDX,&c::ADDD},	{"ANDB ",4,IDX,&c::ANDB},	{"BITB ",4,IDX,&c::BITB},	{"LDB  ",4,IDX,&c::LDB},	{"STB  ",4,IDX,&c::STB},
		{"EORB ",4,IDX,&c::EORB},	{"ADCB ",4,IDX,&c::ADCB},	{"ORB  ",4,IDX,&c::ORB},	{"ADDB ",4,IDX,&c::ADDB},	{"LDD  ",5,IDX,&c::LDD},	{"STD  ",5,IDX,&c::STD},	{"LDU  ",5,IDX,&c::LDU},	{"STU  ",5,IDX,&c::STU},
		{"SUBB ",5,EXT,&c::SUBB},	{"CMPB ",5,EXT,&c::CMPB},	{"SBCB ",5,EXT,&c::SBCB},	{"ADDD ",7,EXT,&c::ADDD},	{"ANDB ",5,EXT,&c::ANDB},	{"BITB ",5,EXT,&c::BITB},	{"LDB  ",5,EXT,&c::LDB},	{"STB  ",5,EXT,&c::STB},
		{"EORB ",5,EXT,&c::EORB},	{"ADCB ",5,EXT,&c::ADCB},	{"ORB  ",5,EXT,&c::ORB},	{"ADDB ",5,EXT,&c::ADDB},	{"LDD  ",6,EXT,&c::LDD},	{"STD  ",6,EXT,&c::STD},	{"LDU  ",6,EXT,&c::LDU},	{"STU  ",6,EXT,&c::STU}
	};
};

