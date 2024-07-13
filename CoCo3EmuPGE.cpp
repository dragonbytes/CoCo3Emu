#define OLC_PGE_APPLICATION
#include "olcPixelGameEngine.h"
#include "CoCo3EmuPGE.h"
//#include "DeviceROM.h"
#include <chrono>

CoCoEmuPGE::CoCoEmuPGE()
{
	// Name your application
	sAppName = "Example";
	gimeBus.ConnectBusGime(this);
}

bool CoCoEmuPGE::OnUserCreate()
{
	// Load a CoCo .BIN file into RAM (for testing purposes)
	/*
	FILE* cocoBinFile = nullptr;
	fopen_s(&cocoBinFile, "Z:\\Documents\\Coco_Code\\coco3emu\\coco3_rom.bin", "rb");

	uint8_t headerBuffer[5];
	uint16_t dataBlockSize, blockLoadAddress;

	if (cocoBinFile)
	{
		fread(headerBuffer, 1, 5, cocoBinFile);

		while (headerBuffer[0] == 0x00)
		{
			dataBlockSize = (headerBuffer[1] * 256) + headerBuffer[2];
			blockLoadAddress = (headerBuffer[3] * 256) + headerBuffer[4];
			printf("Size = $%04X, Load Address = $%04X\n", dataBlockSize, blockLoadAddress);
			fread(&gimeBus.physicalRAM[blockLoadAddress], 1, dataBlockSize, cocoBinFile);
			fread(headerBuffer, 1, 5, cocoBinFile);
		}

		gimeBus.cpu.interruptSignal(INTERRUPT_RESET);		// This "resets" the CPU to it's initial state and enables it's "clock"
	}
	*/

	//DeviceROM* coco3rom = loadRomFile("Z:\\Documents\\Coco_Code\\coco3emu\\coco3_rom.bin");
	std::string romFilePathCoCo3 = "Z:\\Documents\\Coco_Code\\coco3emu\\coco3.rom";
	gimeBus.romCoCo3 = loadFileROM(romFilePathCoCo3.c_str());

	if (gimeBus.romCoCo3 == nullptr)
	{
		printf("Could not load CoCo 3 ROM file at %s\n", romFilePathCoCo3.c_str());
		return true;
	}

	printf("Loaded $%04X bytes from CoCo 3 ROM file.\n", gimeBus.romCoCo3->readRomSize);
	gimeBus.cpu.assertedInterrupts[INT_RESET] = RESET_INT_SRC;		// This "resets" the CPU to it's initial state and enables it's "clock"

	SetDrawTarget(offscreenBuffer);

	return true;
}

bool CoCoEmuPGE::OnUserUpdate(float fElapsedTime)
{
	//using namespace std::chrono;
	
	// Called once per frame, draws random coloured pixels
	uint16_t scanlineIndex;
	
	totalElapsedTime += fElapsedTime;
	//gimeCycleCounter = residualGimeCycleCount;
	elapsedGimeClockCycles = (gimeMasterClock_NTSC * fElapsedTime) + residualCycles;

	//auto start = high_resolution_clock::now();

	while (elapsedGimeClockCycles >= 1)
	{
		gimeBus.gimeBusClockTick();

		if (((unsigned int)elapsedGimeClockCycles % 20) == 0)
			printf("elapsedGimeClockCycles = %u\n", (unsigned int)elapsedGimeClockCycles);

		if (gimeBus.dotCounter == 42)
		{
			if ((gimeBus.curScanline >= 13) && (gimeBus.curScanline < 38))
			{
				// We are in the VDG top border region which is 25 scanlines in height. Just draw a black scanline for them
				scanlineIndex = gimeBus.curScanline - 13;
				DrawLine(0, scanlineIndex, 319, scanlineIndex, olc::BLACK);
			}
			else if ((gimeBus.curScanline >= 38) && (gimeBus.curScanline < 230))
			{
				scanlineIndex = gimeBus.curScanline - 13;
				// Draw left-hand border first
				DrawLine(0, scanlineIndex, 31, scanlineIndex, olc::BLACK);
				// Now draw the "Active Display Area" where text is which is 192 scanlines in height and start at visible scanline 25
				rowPixelCounter = 32;
				vdgScreenRamPtr = (((gimeBus.curScanline - 38) / 12) * vdgColumnsPerRow) + gimeBus.videoStartAddr;
				vdgCharLineOffset = (gimeBus.curScanline - 38) % 12;

				for (int y = 0; y < 32; y++)
				{
					curVDGchar = gimeBus.physicalRAM[vdgScreenRamPtr];
					if (curVDGchar & 0x80)
					{
						// If here, we are rendering a semi-graphics mode 4 character
						vdgFontDataIndex = ((curVDGchar & 0x0F) * 12) + vdgCharLineOffset;
						fontDataByte = semigraphics4_fontdata8x12[vdgFontDataIndex];
						uint8_t semiGfxColor = gimeBus.gimePaletteRegs[(curVDGchar & 0x7F) >> 4];		// Mask off semi-graphics flag bit 7, and shift color bits 6-4 over to get our color value
						for (int x = 0; x < 8; x++)
						{
							fontPixel = (fontDataByte & 0x80) ? gimePaletteRGB[semiGfxColor] : olc::BLACK;
							Draw(rowPixelCounter, scanlineIndex, fontPixel);
							fontDataByte <<= 1;
							rowPixelCounter++;
						}
					}
					else
					{
						// If here, we are rendering an ASCII character
						vdgFontDataIndex = ((curVDGchar & 0x3F) * 12) + vdgCharLineOffset;		// Mask off bits 6 and 7 to get character index. 12 bytes per character bitmap entry
						fontDataByte = lowres_font[vdgFontDataIndex];
						if (!(curVDGchar & 0b01000000))		// Check the "inverted" color bit
							fontDataByte = ~fontDataByte;	// Invert the character's bitmap
						for (int x = 0; x < 8; x++)
						{
							fontPixel = (fontDataByte & 0x80) ? olc::BLACK : olc::GREEN;
							Draw(rowPixelCounter, scanlineIndex, fontPixel);
							fontDataByte <<= 1;
							rowPixelCounter++;
						}
					}
					vdgScreenRamPtr++;
				}
				// Finally draw right-hand border
				DrawLine(rowPixelCounter, scanlineIndex, 319, scanlineIndex, olc::BLACK);
			}
			else if ((gimeBus.curScanline >= 230) && (gimeBus.curScanline < 255))
				DrawLine(0, gimeBus.curScanline - 13, 319, gimeBus.curScanline - 13, olc::BLACK);
			//else if (gimeBus.curScanline == 2);
		}
		else if (gimeBus.dotCounter == 455)
		{
			gimeBus.dotCounter = 0;
			gimeBus.curScanline++;
			if (gimeBus.curScanline == 262)
			{
				gimeBus.curScanline = 0;
				SetDrawTarget(nullptr);
				DrawSprite(0, 0, offscreenBuffer);
				SetDrawTarget(offscreenBuffer);
			}
		}

		/*
		if (gimeCycleCounter >= gimeFrameTime_NTSC)
		{
			//FillRect(0, 0, 320, 240, olc::BLACK);
			Clear(olc::BLACK);
			FillRect(32, 24, 256, 192, olc::GREEN);

			for (y = 0; y < 16; y++)
			{
				for (x = 0; x < 32; x++)
					if (gimeBus.physicalRAM[vdgVideoStartAddr + x] >= 0x60)
						vdgTextLine[x] = gimeBus.physicalRAM[vdgVideoStartAddr + x] - 0x40;
					else
						vdgTextLine[x] = gimeBus.physicalRAM[vdgVideoStartAddr + x];
				DrawString(32, 24 + (y * 8), vdgTextLine, olc::BLACK, 1);
				vdgVideoStartAddr += 32;
			}
			gimeCycleCounter = 0;
			//printf("Frame Rendered\n");

			fpsFrameCounter++;
			if (totalElapsedTime > 1.0)
			{
				printf("CoCo FPS = %f\n", fpsFrameCounter);
				fpsFrameCounter = 0;
				totalElapsedTime = 0;

				//printf("elapsedGimeClockCycles = %f, Average Elapsed Cycles = %f\n", elapsedGimeClockCycles, (averageElapsedCycles/averageElapsedCyclesCount));
			}
		} */
		elapsedGimeClockCycles--;
	}
	//auto end = high_resolution_clock::now();
	//duration<double> duration = end - start;
	//printf("fElapsedTime = %f, timer = %f\n", fElapsedTime, duration.count());

	// correct any overshoot of our counter next time through the loop
	residualCycles = elapsedGimeClockCycles;
	//residualGimeCycleCount = gimeCycleCounter;

	for (int i = 0; i < 56; i++) 
		gimeBus.hostKeyMatrix[i].keyDownState = GetKey(gimeBus.hostKeyMatrix[i].keyID).bHeld;
	// Add one additional check for the host machine's "Backspace" key and make the CoCo "Left-arrow" true if either one is pressed
	gimeBus.hostKeyMatrix[17].keyDownState |= GetKey(olc::BACK).bHeld;

	return true;
}

DeviceROM* CoCoEmuPGE::loadFileROM(const char* romFilePath)
{
	FILE* romFile = nullptr;

	int romOpenResult = fopen_s(&romFile, romFilePath, "rb");
	if (romOpenResult)
		return nullptr;

	fseek(romFile, 0, SEEK_END);
	long romFileSize = ftell(romFile);
	fseek(romFile, 0, SEEK_SET);				// Return file pointer to beginning of file again

	DeviceROM* newRomFile = new DeviceROM(romFileSize);
	newRomFile->romData.resize(romFileSize);
	// Set size and initilize ROM data block
	for (int i = 0; i < romFileSize; i++)
		newRomFile->romData[i] = std::rand() % 256;

	newRomFile->readRomSize = fread(&newRomFile->romData[0], 1, romFileSize, romFile);
	fclose(romFile);

	return newRomFile;
}