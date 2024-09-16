#pragma once
#include <vector>
#include <cstdint>
#include <string>
#include "CPU6809.h"
#include "DeviceROM.h"
#include "FD502.h"
#include "EmuDisk.h"
#include "olcPixelGameEngine.h"

constexpr uint8_t INT_CLEAR = 0;
constexpr uint8_t INT_SRC_PIA_VSYNC = 1;
constexpr uint8_t INT_SRC_PIA_HSYNC = 2;
constexpr uint8_t INT_SRC_GIME_TIMER = 3;
constexpr uint8_t INT_SRC_GIME_HBORD = 4;
constexpr uint8_t INT_SRC_GIME_VBORD = 5;
constexpr uint8_t INT_SRC_GIME_SERIAL = 6;
constexpr uint8_t INT_SRC_GIME_KEYBOARD = 7;
constexpr uint8_t INT_SRC_GIME_CART = 8;

// GIME master clock frequency in Hz
constexpr float gimeMasterClock_NTSC = 28636360.0F;
constexpr double gimeFrameTime_NTSC = gimeMasterClock_NTSC / 60;
constexpr float gimeMasterClock_PAL = 2847500;
constexpr double gimeFrameTime_PAL = gimeMasterClock_PAL / 60;
constexpr double cpuClockFreq_NTSC = gimeMasterClock_NTSC / 4;
constexpr double cpuFrameTime_NTSC = cpuClockFreq_NTSC / 60;

constexpr int gimeTimerOffset = 2;						// 1986 GIMEs process an additional 2 counts of the timer from what the user sets, 1987 revision is only 1 instead of 2.

// GIME Register Definitions
constexpr uint8_t GIME_ROM_16_SPLIT_0	= 0b00000000;
constexpr uint8_t GIME_ROM_16_SPLIT_1	= 0b00000001;
constexpr uint8_t GIME_ROM_32_INTERNAL	= 0b00000010;
constexpr uint8_t GIME_ROM_32_EXTERNAL	= 0b00000011;

constexpr uint8_t GIME_TEXT_UNDERLINE	= 0b01000000;
constexpr uint8_t GIME_TEXT_BLINKING	= 0b10000000;

constexpr uint8_t GIME_GFX_COLORS_2		= 0b00000000;
constexpr uint8_t GIME_GFX_COLORS_4		= 0b00000001;
constexpr uint8_t GIME_GFX_COLORS_16	= 0b00000010;
constexpr uint8_t GIME_GFX_COLORS_UNDEF = 0b00000011;

constexpr uint8_t GIME_INT_MASK_TIMER	 = 0b00100000;
constexpr uint8_t GIME_INT_MASK_HBORDER  = 0b00010000;
constexpr uint8_t GIME_INT_MASK_VBORDER  = 0b00001000;
constexpr uint8_t GIME_INT_MASK_SERIAL	 = 0b00000100;
constexpr uint8_t GIME_INT_MASK_KEYBOARD = 0b00000010;
constexpr uint8_t GIME_INT_MASK_CART	 = 0b00000001;

constexpr uint8_t PIA_CTRL_MUX_MASK			= 0b00001000;
constexpr uint8_t PIA_CTRL_DIR_MASK			= 0b00000100;
constexpr uint8_t PIA_CTRL_IRQEDGE_MASK		= 0b00000010;
constexpr uint8_t PIA_CTRL_IRQENABLE_MASK	= 0b00000001;

constexpr uint8_t COL7 = 0x80;
constexpr uint8_t COL6 = 0x40;
constexpr uint8_t COL5 = 0x20;
constexpr uint8_t COL4 = 0x10;
constexpr uint8_t COL3 = 0x08;
constexpr uint8_t COL2 = 0x04;
constexpr uint8_t COL1 = 0x02;
constexpr uint8_t COL0 = 0x01;
constexpr uint8_t ROW7 = 0x80;
constexpr uint8_t ROW6 = 0x40;
constexpr uint8_t ROW5 = 0x20;
constexpr uint8_t ROW4 = 0x10;
constexpr uint8_t ROW3 = 0x08;
constexpr uint8_t ROW2 = 0x04;
constexpr uint8_t ROW1 = 0x02;
constexpr uint8_t ROW0 = 0x01;

constexpr uint8_t JOYSTICK_DEVICE_KEYBOARD		= 0;
constexpr uint8_t JOYSTICK_DEVICE_MOUSE			= 1;
constexpr uint8_t JOYSTICK_DEVICE_CONTROLLER	= 2;

constexpr uint8_t JOYSTICK_PORT_RIGHT	 = 0;
constexpr uint8_t JOYSTICK_PORT_LEFT	 = 1;

struct piaRegisters
{
	uint8_t controlReg;
	uint8_t dataDirReg;
	uint8_t dataReg;
};

struct piaDevice
{
	piaRegisters SideA;
	piaRegisters SideB;
};

class CoCoEmuPGE;

struct keyMatrix
{
	olc::Key key;
	uint8_t cocoKeyMask;
};

struct olcKeyMatrix
{
	olc::Key keyID;
	bool keyDownState;
};

class GimeBus
{
	public:
		GimeBus();

	public:
		FD502 diskController;
		EmuDisk emuDiskDriver;
		Cpu6809 cpu;
		DeviceROM* romCoCo3;
		DeviceROM* romExternal;
		//serialib serial;

		std::vector<uint8_t> physicalRAM;
		//std::vector<olc::Pixel> offscreenBuffer;
		uint16_t ramTotalSizeKB, curResolutionWidth;
		uint8_t curResolutionHeight;
		unsigned int masterBusCycleCounter;
		unsigned int scanlineCounter, dotCounter;	// 1024 GIME cycles for Active Display Area, 1484 GIME cycles for start of left border to end of right border, 1820 GIME cycles for EVERYTHING together, 336 GIME cycles of horizontal blanking period
		int cpuClockDivisor;
		uint32_t videoStartAddr, ramSizeMask;
		uint8_t gimePaletteRegs[16];
		bool gimeBlinkStateOn;
		piaDevice devPIA0, devPIA1;
		std::string statusBarText;

		struct joystickStruct
		{
			bool isAttached;
			bool buttonDown1;
			bool buttonDown2;
			uint8_t deviceType;
			uint8_t xAxis;
			uint8_t yAxis;
		};

		joystickStruct joystickDevice[2];

		struct vdgConfig
		{
			bool gfxModeEnabled;
			bool fontIsExternal;
			uint8_t colorSetSelect;
			uint8_t colorDepth;
			uint8_t bytesPerRow;

		} vdgVideoConfig;

		// GIME-specific internal registers
		struct GimeInit0
		{
			bool cocoCompatMode;
			bool mmuEnabled;
			bool chipIRQEnabled;			
			bool chipFIRQEnabled;				
			bool constSecondaryVectors;
			bool scsEnabled;
			uint8_t romMapControl;
		} gimeRegInit0;

		struct GimeInit1
		{
			bool memoryType;		// False = 64K chips, True = 256K chips
			bool timerSourceFast;		// False = 63.695 usec, True = 279.365 nsec
			uint8_t mmuTaskSelect;	// 0 = 0xFFA0-0xFFA7, 1 = 0xFFA8-0xFFAF
		} gimeRegInit1;

		// Both IRQ and FIRQ registers have the same bit definitions and behavoirs
		uint8_t gimeRegIRQtypes;	// Bit 5 = Enable Timer Interrupt
		uint8_t gimeRegFIRQtypes;	// Bit 4 = Enable Horizontal Border Interrupt
									// Bit 3 = Enable Vertical Border Interrupt
									// Bit 2 = Enable Serial Data Interrupt
									// Bit 1 = Enable Keyboard Interrupt
									// Bit 0 = Enable CART Interrupt
		uint8_t gimeIRQstatus, gimeFIRQstatus;

		union
		{
			struct
			{
				// Because PC's use little-endian, LSB comes before MSB in a 16-bit word
				uint8_t LSB;
				uint8_t MSB;
			};
			uint16_t word;
		} gimeRegTimer;
		
		uint16_t gimeTimerCounter;

		struct GimeVMODE
		{
			bool gfxOrTextMode;			// BP	(False = Text modes, True = Graphics modes)
			bool cmpColorPhaseInvert;	// BPI
			bool monoCompositeOut;		// MOCH
			bool video50hz;				// H50	(False = 60 Hz video, True = 50 Hz video)
			uint8_t LPR;				// Lines Per Row
		} gimeRegVMode;

		struct GimeVRES
		{
			uint8_t LPF;				// 00 = 192 scanlines, 01 = 200 scanlines, 10 = zero/infinite scanlines (undefined), 11 = 225 scanlines
			uint8_t HRES;				// 000 = 16 bytes/row, 001 = 20 bytes/row, 010 = 32 bytes/row, 011 = 40 bytes/row, 100 = 64 bytes/row, 101 = 80 bytes/row, 110 = 128 bytes/row, 111 = 160 bytes/row
			uint8_t CRES;				// Gfx modes: 00 = 2 colors, 01 = 4 colors, 10 = 16 colors, 11 = Undefined / Text modes: x0 = No color attributes, x1 = color attributes enabled

		} gimeRegVRES;

		uint8_t textCharsPerRow[8] = { 32, 40, 32, 40, 64, 80, 64, 80 };
		uint8_t gfxBytesPerRow[8] = { 16, 20, 32, 40, 64, 80, 128, 160 };
		uint8_t gimeTopBorderEnd[4] = { 25, 21, 0, 9 };				// Values are zero-based VISIBLE scanline numbers. Array index should be LPF value from GIME $FF99 VRES register
		uint8_t gimeBottomBorderStart[4] = { 217, 221, 0, 234 };	// Values are zero-based VISIBLE scanline numbers. Array index should be LPF value from GIME $FF99 VRES register
		uint8_t gimeLinesPerRow[8] = { 1, 1, 2, 8, 9, 10, 11, 0 };	// 0 = Infinite lines and should be handled with care in the code
		uint8_t gimeUnderlineOffset[4] = { 8, 9, 9, 10 };
		uint8_t gimeRegBorder;
		uint16_t gimeHorizontalResolutions[4] = { 512, 640, 512, 640 };
		uint8_t gimeVerticalResolutions[4] = { 192, 200, 0, 225 };

		// GIME 64-color palette definitions for both Composite and RGB
		olc::Pixel gimePaletteDefs[2][64] = {
			{	// First set of values is for Composite display output
			{0, 0, 0},		{0, 0, 85},		{0, 85, 0},		{0, 85, 85},	{85, 0, 0},		{85, 0, 85},	{85, 85, 0},	{85, 85, 85},
			{0, 0, 170},	{0, 0, 255},	{0, 85, 170},	{0, 85, 255},	{85, 0, 170},	{85, 0, 255},	{85, 85, 170},	{85, 85, 255},
			{0, 170, 0},	{0, 170, 85},	{0, 255, 0},	{0, 255, 85},	{85, 170, 0},	{85, 170, 85},	{85, 255, 0},	{85, 255, 85},
			{0, 170, 170},	{0, 170, 255},	{0, 255, 170},	{0, 255, 255},	{85, 170, 170},	{85, 170, 255},	{85, 255, 170},	{85, 255, 255},
			{170, 0, 0},	{170, 0, 85},	{170, 85, 0},	{170, 85, 85},	{255, 0, 0},	{255, 0, 85},	{255, 85, 0},	{255, 85, 85},
			{170, 0, 170},	{170, 0, 255},	{170, 85, 170},	{170, 85, 255},	{255, 0, 170},	{255, 0, 255},	{255, 85, 170},	{255, 85, 255},
			{170, 170, 0},	{170, 170, 85},	{170, 255, 0},	{170, 255, 85},	{255, 170, 0},	{255, 170, 85},	{255, 255, 0},	{255, 255, 85},
			{170, 170, 170},{170, 170, 255},{170, 255, 170},{170, 255, 255},{255, 170, 170},{255, 170, 255},{255, 255, 170},{255, 255, 255}
			},
			{	// Second set of values for RGB display output
			{0, 0, 0},		{0, 0, 85},		{0, 85, 0},		{0, 85, 85},	{85, 0, 0},		{85, 0, 85},	{85, 85, 0},	{85, 85, 85},
			{0, 0, 170},	{0, 0, 255},	{0, 85, 170},	{0, 85, 255},	{85, 0, 170},	{85, 0, 255},	{85, 85, 170},	{85, 85, 255},
			{0, 170, 0},	{0, 170, 85},	{0, 255, 0},	{0, 255, 85},	{85, 170, 0},	{85, 170, 85},	{85, 255, 0},	{85, 255, 85},
			{0, 170, 170},	{0, 170, 255},	{0, 255, 170},	{0, 255, 255},	{85, 170, 170},	{85, 170, 255},	{85, 255, 170},	{85, 255, 255},
			{170, 0, 0},	{170, 0, 85},	{170, 85, 0},	{170, 85, 85},	{255, 0, 0},	{255, 0, 85},	{255, 85, 0},	{255, 85, 85},
			{170, 0, 170},	{170, 0, 255},	{170, 85, 170},	{170, 85, 255},	{255, 0, 170},	{255, 0, 255},	{255, 85, 170},	{255, 85, 255},
			{170, 170, 0},	{170, 170, 85},	{170, 255, 0},	{170, 255, 85},	{255, 170, 0},	{255, 170, 85},	{255, 255, 0},	{255, 255, 85},
			{170, 170, 170},{170, 170, 255},{170, 255, 170},{170, 255, 255},{255, 170, 170},{255, 170, 255},{255, 255, 170},{255, 255, 255}
			}
		};

		// CoCo 1/2 compatible VDG color palette defintions
		olc::Pixel vdgBorderPaletteDefs[2] = { {0, 255, 0}, {255, 255, 255} };

		olcKeyMatrix hostKeyMatrix[56] = {
			{olc::G, false},		{olc::O, false}, {olc::W, false}, {olc::SPACE, false},	{olc::K7, false}, {olc::OEM_2, false},	{olc::SHIFT, false},
			{olc::F, false},		{olc::N, false}, {olc::V, false}, {olc::RIGHT, false},	{olc::K6, false}, {olc::PERIOD, false},	{olc::F2, false},
			{olc::E, false},		{olc::M, false}, {olc::U, false}, {olc::LEFT, false},	{olc::K5, false}, {olc::EQUALS, false},	{olc::F1, false},
			{olc::D, false},		{olc::L, false}, {olc::T, false}, {olc::DOWN, false},	{olc::K4, false}, {olc::COMMA, false},	{olc::CTRL, false},
			{olc::C, false},		{olc::K, false}, {olc::S, false}, {olc::UP, false},		{olc::K3, false}, {olc::OEM_1, false},	{olc::INS, false},
			{olc::B, false},		{olc::J, false}, {olc::R, false}, {olc::Z, false},		{olc::K2, false}, {olc::MINUS, false},	{olc::ESCAPE, false},
			{olc::A, false},		{olc::I, false}, {olc::Q, false}, {olc::Y, false},		{olc::K1, false}, {olc::K9, false},		{olc::HOME, false},
			{olc::OEM_4, false},	{olc::H, false}, {olc::P, false}, {olc::X, false},		{olc::K0, false}, {olc::K8, false},		{olc::ENTER, false}
		};

		void ConnectBusGime(CoCoEmuPGE* inputPtr) { mainPtr = inputPtr; }
		uint8_t readPhysicalByte(uint16_t);
		uint8_t writePhysicalByte(uint16_t, uint8_t);
		uint8_t readMemoryByte(uint16_t);
		uint16_t readMemoryWord(uint16_t);
		uint8_t writeMemoryByte(uint16_t,uint8_t);
		uint16_t writeMemoryWord(uint16_t,uint16_t);
		void SetRAMSize(int);
		void gimeBusClockTick();
		void updateVideoParams();

	private:
		CoCoEmuPGE* mainPtr = nullptr;
		const unsigned int vdgColumnsPerRow = 32;
		unsigned int floppySeekIntervalCounter, floppyAccessIntervalCounter;
		uint8_t vdgCharLineOffset, curVDGchar, fontDataByte, joystickPortIndex, joystickAxisValue, joystickCompareResult, returnByte, tempByte;
		uint16_t scanlineIndex, rowPixelCounter, vdgFontDataIndex;
		uint32_t vdgScreenRamPtr;
		olc::Pixel fontPixel, borderPixel;
		uint16_t diskByteReadCount;

		std::string strEmuInfoName = "COCO3EMU";
		uint8_t emuInfoVersionMajor = 0, emuInfoVersionMinor = 1;
		std::string strEmuInfoExtra = "WRITTEN BY TODD WALLACE";
		int emuInfoTextIndex;
		std::string* emuInfoStringPtr = &strEmuInfoName;

		struct GimeMMUBankReg
		{
			uint8_t bankNum;
			uint32_t mmuBlockAddr;
		};

		GimeMMUBankReg gimeMMUBankRegs[16] = 
		{	{0x38, 0x70000}, {0x39, 0x72000}, {0x3A, 0x74000}, {0x3B, 0x76000}, {0x3C, 0x78000}, {0x3D, 0x7A000}, {0x3E, 0x7C000}, {0x3F, 0x7E000}, 
			{0x38, 0x70000}, {0x39, 0x72000}, {0x3A, 0x74000}, {0x3B, 0x76000}, {0x3C, 0x78000}, {0x3D, 0x7A000}, {0x3E, 0x7C000}, {0x3F, 0x7E000}
		};

		bool gimeAllRamModeEnabled;
		uint8_t gimeVertOffsetMSB;
		uint8_t gimeVertOffsetLSB;

		struct
		{
			bool hven;
			uint8_t offsetAddress;
		} gimeHorizontalOffsetReg;

		uint8_t samPageSelectMasks[14] = { 0xFE, 0x01, 0xFD, 0x02, 0xFB, 0x04, 0xF7, 0x08, 0xEF, 0x10, 0xDF, 0x20, 0xBF, 0x40 };
		uint8_t samPageSelectReg;
		uint8_t samVideoDisplayReg;
		uint8_t samVideoDisplayMasks[6] = { 0b11111110, 0b00000001, 0b11111101, 0b00000010, 0b11111011, 0b00000100 };

		uint8_t readByteFromROM(uint16_t);
		uint8_t getCocoKey(uint8_t);
		uint8_t cocoKeyStrobeResult[7] = { 0xFE, 0xFD, 0xFB, 0xF7, 0xEF, 0xDF, 0xBF };

		void decrementGimeTimer();
		/*
	public:
		union SAMPageSelectRegs
		{
		public:
			struct SAMPageSelectBits
			{
				bool bitF0 : 1;
				bool bitF1 : 1;
				bool bitF2 : 1;
				bool bitF3 : 1;
				bool bitF4 : 1;
				bool bitF5 : 1;
				bool bitF6 : 1;
				bool bitF7 : 1;
			} bitArray;
			uint8_t pageSelectRegByte;
		} samPageSelectRegs;
		*/
};
