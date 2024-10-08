#define OLC_PGE_APPLICATION
#include <chrono>
#include <iostream>
#include "olcPixelGameEngine.h"
#include "CoCo3EmuPGE.h"
#include "FontData.h"

#define OLC_PGEX_SOUND
#include "olcPGEX_Sound.h"

// These defines are just so I can use shorthand labels for these long object references
#define palRegs gimeBus.gimePaletteRegs
#define palDefs gimeBus.gimePaletteDefs

CoCoEmuPGE::CoCoEmuPGE()
{
	// Name your application
	sAppName = "CoCo3Emu";
	gimeBus.ConnectBusGime(this);

	gimeBus.joystickDevice[JOYSTICK_PORT_RIGHT].isAttached = true;
	gimeBus.joystickDevice[JOYSTICK_PORT_RIGHT].deviceType = JOYSTICK_DEVICE_MOUSE;
}

bool CoCoEmuPGE::OnUserCreate()
{
	// Init the sound extension/hardware
	std::function<float(int, float, float)> soundHandlerFunc = std::bind(&CoCoEmuPGE::SoundHandler, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
	olc::SOUND::InitialiseAudio(48000, 1, 8, 512);
	olc::SOUND::SetUserSynthFunction(soundHandlerFunc);

	uiLoadmHasExecAddr = false;
	ConsoleCaptureStdOut(true);

	// Look for CoCo 3 32k rom file called "coco3.rom" in execution directory, and if present, load it into the emulator
	std::string romFilePathCoCo3 = "C:\\Users\\TekTodd\\source\\repos\\CoCo3Emu\\coco3.rom";
	gimeBus.romCoCo3 = loadFileROM(romFilePathCoCo3.c_str());
	if (gimeBus.romCoCo3 == nullptr)
	{
		printf("Could not load CoCo 3 ROM file at %s\n", romFilePathCoCo3.c_str());
		return true;
	}
	printf("Loaded $%04X bytes from CoCo 3 ROM file.\n", gimeBus.romCoCo3->readRomSize);

	// Now try and locate external 16K disk rom of some kind such as "disk11.rom". If found, load that as well
	std::string romFilePathExtDisk = "C:\\Users\\TekTodd\\source\\repos\\CoCo3Emu\\disk11.rom";
	gimeBus.romExternal = loadFileROM(romFilePathExtDisk.c_str());
	if (gimeBus.romExternal == nullptr)
	{
		printf("Could not load Disk ROM v1.1 file at %s\n", romFilePathExtDisk.c_str());
		return true;
	}
	printf("Loaded $%04X bytes from Disk 1.1 ROM file.\n", gimeBus.romExternal->readRomSize);

	//gimeBus.diskController.fdcAttachNewDrive(0, 80, true, true);
	//printf("Attached 80 track double-sided drive to Drive 0\n");
	//gimeBus.diskController.fdcInsertDisk(0, "Z:\\Documents\\Coco_Code\\EOU\\V101_STD\\68EMU.dsk");
	//printf("Inserted disk into Drive 0\n");
	//gimeBus.emuDiskDriver.isEnabled = true;
	//gimeBus.emuDiskDriver.vhdMountDisk(0, "Z:\\Documents\\Coco_Code\\EOU\\V101_STD\\68SDC.vhd");

	// Attempt to load config file
	uint8_t configLoadStatus = loadConfigFile();
	if (configLoadStatus == CONFIG_ERROR_OPENING_FILE)
		printf("Error: Could not open config file.\n");
	else if (configLoadStatus == CONFIG_ERROR_INVALID_SYNTAX)
		printf("Error: Invalid syntax encountered in config file. Aborted.\n");
	else if (configLoadStatus == CONFIG_ERROR_MOUNTING_DISK)
		printf("Error: Could not mount disk image in config file.\n");
	else
		printf("Successfully loaded config file.\n");

	gimeBus.cpu.assertedInterrupts[INT_RESET] = INT_ASSERT_MASK_RESET;		// This "resets" the CPU to it's initial state and enables it's "clock"
	cmpOrRgb = 1;	// Set display palette set to RGB by default

	// Setup initial status bar state
	gimeBus.statusBarText = "Idle";
	FillRect(0, STATUSBAR_START_Y, 640, STATUSBAR_HEIGHT, olc::GREY);
	DrawString(STATUSBAR_TEXT_OFFSET_X, STATUSBAR_TEXT_OFFSET_Y, gimeBus.statusBarText, olc::BLACK);
	statusBarTimeout = true;

	return true;
}

bool CoCoEmuPGE::OnUserDestroy()
{
	olc::SOUND::DestroyAudio();
	return true;
}

bool CoCoEmuPGE::OnUserUpdate(float fElapsedTime)
{	
	// Called once per frame, draws random coloured pixels
	uint16_t scanlineIndex;
	
	statusElapsedTime += fElapsedTime;
	//gimeCycleCounter = residualGimeCycleCount;
	elapsedGimeClockCycles = (gimeMasterClock_NTSC * fElapsedTime) + residualCycles;

	/*
	averageElapsedGimeCycles += elapsedGimeClockCycles;
	averageElapsedGimeCyclesCounter++;
	if (averageElapsedGimeCyclesCounter >= 2000)
	{
		printf("Average elapsed GIME cycles = %f\n", (averageElapsedGimeCycles / 2000));
		averageElapsedGimeCyclesCounter = 0;
		averageElapsedGimeCycles = 0;
	}
	*/

	while (elapsedGimeClockCycles > 1.0f)
	{
		gimeBus.gimeBusClockTick();
		gimeAudioCounter--;
		if (gimeAudioCounter <= 0.0f)
		{
			//fifoAudioBuffer.push(gimeBus.devPIA1.SideA.dataReg >> 2);	// add a new coco audio sample to our Host audio buffer
			leftAudioBuffer.push(gimeBus.devPIA1.SideA.dataReg >> 2);
			//rightAudioBuffer.push(gimeBus.orch90dac.rightChannel);
			//fifoAudioBuffer.push(newStereoSample);
			if (!audioBufferReady && leftAudioBuffer.size() >= numSamplesToBuffer)
				audioBufferReady = true;
			gimeAudioCounter += gimeAudioCountInterval;
		}
		
		if (gimeBus.dotCounter == 168)
		{
			// If current scanline is in the "visible" range (including borders because you CAN see those), then call relevant renderer function
			if ((gimeBus.scanlineCounter >= 13) && (gimeBus.scanlineCounter < 255))
			{
				if (gimeBus.gimeRegInit0.cocoCompatMode)
					renderScanlineVDG(gimeBus.scanlineCounter);
				else
					renderScanlineGIME(gimeBus.scanlineCounter);
			}
		}
		elapsedGimeClockCycles--;
	} 
	// correct any overshoot of our counter next time through the loop
	residualCycles = elapsedGimeClockCycles;

	for (int i = 0; i < 56; i++) 
		gimeBus.hostKeyMatrix[i].keyDownState = GetKey(gimeBus.hostKeyMatrix[i].keyID).bHeld;
	// Add one additional check for the host machine's "Backspace" key and make the CoCo "Left-arrow" true if either one is pressed
	gimeBus.hostKeyMatrix[17].keyDownState |= GetKey(olc::BACK).bHeld;

	if (gimeBus.joystickDevice[JOYSTICK_PORT_RIGHT].isAttached && (gimeBus.joystickDevice[JOYSTICK_PORT_RIGHT].deviceType == JOYSTICK_DEVICE_KEYBOARD))
	{
		if (GetKey(olc::NP4).bHeld)
			gimeBus.joystickDevice[JOYSTICK_PORT_RIGHT].xAxis = 0;
		else if (GetKey(olc::NP6).bHeld)
			gimeBus.joystickDevice[JOYSTICK_PORT_RIGHT].xAxis = 63;
		else
			gimeBus.joystickDevice[JOYSTICK_PORT_RIGHT].xAxis = 31;

		if (GetKey(olc::NP8).bHeld)
			gimeBus.joystickDevice[JOYSTICK_PORT_RIGHT].yAxis = 0;
		else if (GetKey(olc::NP2).bHeld)
			gimeBus.joystickDevice[JOYSTICK_PORT_RIGHT].yAxis = 63;
		else
			gimeBus.joystickDevice[JOYSTICK_PORT_RIGHT].yAxis = 31;

		gimeBus.joystickDevice[JOYSTICK_PORT_RIGHT].buttonDown1 = GetKey(olc::F5).bHeld;
		gimeBus.joystickDevice[JOYSTICK_PORT_RIGHT].buttonDown2 = GetKey(olc::F6).bHeld;
	}
	else if (gimeBus.joystickDevice[JOYSTICK_PORT_RIGHT].isAttached && (gimeBus.joystickDevice[JOYSTICK_PORT_RIGHT].deviceType == JOYSTICK_DEVICE_MOUSE))
	{
		uint32_t mousePositionX = GetMouseX();
		uint32_t mousePositionY = GetMouseY();
		uint8_t sideBorderWidth = (640 - gimeBus.curResolutionWidth) / 2;
		uint8_t topBottomBorderWidth = (242 - gimeBus.curResolutionHeight) / 2;

		if ((mousePositionX >= sideBorderWidth) && (mousePositionX < (640 - sideBorderWidth)) && (mousePositionY >= topBottomBorderWidth) && (mousePositionY < (242 - topBottomBorderWidth)))
		{
			gimeBus.joystickDevice[JOYSTICK_PORT_RIGHT].xAxis = ((mousePositionX - sideBorderWidth) * 64) / gimeBus.curResolutionWidth;
			gimeBus.joystickDevice[JOYSTICK_PORT_RIGHT].yAxis = ((mousePositionY - topBottomBorderWidth) * 64) / gimeBus.curResolutionHeight;
			gimeBus.joystickDevice[JOYSTICK_PORT_RIGHT].buttonDown1 = GetMouse(0).bHeld;
			gimeBus.joystickDevice[JOYSTICK_PORT_RIGHT].buttonDown2 = GetMouse(1).bHeld;
		}

	}

	// Check for the "Setup/Command Mode" keystroke TAB
	if (GetKey(olc::TAB).bHeld)
		ConsoleShow(olc::ESCAPE);

	if (updateStatusBar())
	{
		statusElapsedTime = 0;
		statusBarTimeout = false;
	}
	else if (!statusBarTimeout && statusElapsedTime >= 6.0f)
	{
		statusElapsedTime = 0;
		statusBarTimeout = true;
		// Clear status bar and show "Idle" message
		gimeBus.statusBarText = "Idle";
		FillRect(0, STATUSBAR_START_Y, 640, STATUSBAR_HEIGHT, olc::GREY);
		DrawString(STATUSBAR_TEXT_OFFSET_X, STATUSBAR_TEXT_OFFSET_Y, gimeBus.statusBarText, olc::BLACK);
	}

	return true;
}

bool CoCoEmuPGE::OnConsoleCommand(const std::string& sText)
{
	std::string commandWord, filename;

	// Extract next word
	commandWord = stringToUpper(nextStringWord(sText));

	if (commandWord == "LOADM")
	{
		filename = nextStringWord(sText);
		if (!filename.empty())
			commandLOADM(filename);
		else
			std::cout << "Error: No filename was specified." << std::endl;
	}
	else if (commandWord == "EXEC")
		commandEXEC();
	else if (commandWord == "ATTACH")
	{
		std::string deviceName = stringToUpper(nextStringWord(sText));
		if (deviceName == "FLOPPY")
		{
			uint8_t driveNum = std::stoi(nextStringWord(sText));
			uint8_t totalTracks = std::stoi(nextStringWord(sText));
			uint8_t totalSides = std::stoi(nextStringWord(sText));
			gimeBus.diskController.fdcAttachNewDrive(driveNum, totalTracks, (totalSides == 2) ? true : false, true);
			std::cout << "Successfully attached " << std::to_string(totalTracks) << " track " << ((totalSides == 2) ? "double" : "single") << "-sided drive as Drive " << std::to_string(driveNum) << std::endl;
		}
		else if (deviceName == "EMUDISK")
		{
			if (gimeBus.emuDiskDriver.isEnabled)
				std::cout << "Error: EmuDisk Virtual HD controller is already enabled." << std::endl;
			else
			{
				gimeBus.emuDiskDriver.isEnabled = true;
				std::cout << "Successfully enabled the EmuDisk Virtual HD conroller." << std::endl;
			}
		}
		else if (deviceName == "JOYSTICK")
		{
			std::string portName = stringToUpper(nextStringWord(sText));
			std::string inputType = stringToUpper(nextStringWord(sText));
			uint8_t portIndexNum;
			if (portName == "RIGHT")
				portIndexNum = JOYSTICK_PORT_RIGHT;
			else if (portName == "LEFT")
				portIndexNum = JOYSTICK_PORT_LEFT;
			else
			{
				std::cout << "Error: Invalid parameter." << std::endl;
				return true;
			}

			if (inputType == "KEYBOARD")
				gimeBus.joystickDevice[portIndexNum].deviceType = JOYSTICK_DEVICE_KEYBOARD;
			else if (inputType == "MOUSE")
				gimeBus.joystickDevice[portIndexNum].deviceType = JOYSTICK_DEVICE_MOUSE;
			else if (inputType == "GAMEPAD")
				gimeBus.joystickDevice[portIndexNum].deviceType = JOYSTICK_DEVICE_GAMEPAD;
			else
			{
				std::cout << "Error: Invalid parameter." << std::endl;
				return true;
			}
			gimeBus.joystickDevice[portIndexNum].isAttached = true;
			std::cout << "Successfully attached " << inputType << " to " << portName << " joystick port." << std::endl;
		}
		else if (deviceName.empty())
		{
			// Since no subcommands were entered, just display current state/status of attached hardware

		}
		else
			std::cout << "Error: Invalid parameter." << std::endl;
	}
	else if (commandWord == "DETACH")
	{
		std::string deviceName = stringToUpper(nextStringWord(sText));
		if (deviceName == "FLOPPY")
		{
			uint8_t driveNum = std::stoi(nextStringWord(sText));
			if (!gimeBus.diskController.fdcDrive[driveNum].isDriveAttached)
				std::cout << "Error: No drive is attached to Drive " << std::to_string(driveNum) << std::endl;
			else
			{
				gimeBus.diskController.fdcDetachDrive(driveNum);
				std::cout << "Successfully detached Drive " << std::to_string(driveNum) << std::endl;
			}
		}
		else if (deviceName == "EMUDISK")
		{
			if (!gimeBus.emuDiskDriver.isEnabled)
				std::cout << "Error: EmuDisk Virtual HD controller is not attached." << std::endl;
			else
			{
				gimeBus.emuDiskDriver.isEnabled = false;
				std::cout << "Successfully disabled the EmuDisk Virtual HD conroller." << std::endl;
			}
		}
	}
	else if (commandWord == "INSERT")
	{
		uint8_t targetDriveNum = std::stoi(nextStringWord(sText));

		if (!gimeBus.diskController.fdcDrive[targetDriveNum].isDriveAttached)
			std::cout << "Error: No drive is attached to Drive " << std::to_string(targetDriveNum) << std::endl;
		else if (gimeBus.diskController.fdcDrive[targetDriveNum].isDiskInserted)
			std::cout << "Error: A disk is already inserted into Drive " << std::to_string(targetDriveNum) << ". Eject it before trying to inserting a new disk." << std::endl;
		else
		{
			std::string diskImageFilename = nextStringWord(sText);
			uint8_t mountResult = gimeBus.diskController.fdcInsertDisk(targetDriveNum, diskImageFilename);
			if (mountResult == FD502_OPERATION_COMPLETE)
				std::cout << "Successfully mounted " << diskImageFilename << " to Drive " << std::to_string(targetDriveNum) << std::endl;
			else
				std::cout << "Error: Disk image mount failed." << std::endl;
		}
	}
	else if (commandWord == "EJECT")
	{
		uint8_t targetDriveNum = std::stoi(nextStringWord(sText));
		uint8_t ejectStatus = gimeBus.diskController.fdcEjectDisk(targetDriveNum);

		switch (ejectStatus)
		{
		case FD502_OPERATION_COMPLETE:
			std::cout << "Successfully ejected disk from Drive " << std::to_string(targetDriveNum) << std::endl;
			break;
		case FD502_ERROR_DRIVE_NOT_ATTACHED:
			std::cout << "Error: No drive attached to Drive " << std::to_string(targetDriveNum) << std::endl;
			break;
		case FD502_ERROR_NO_DISK_INSERTED:
			std::cout << "Error: No disk is inserted in Drive " << std::to_string(targetDriveNum) << std::endl;
			break;
		}
	}
	else if (commandWord == "RESET")
		gimeBus.cpu.assertedInterrupts[INT_RESET] = INT_ASSERT_MASK_RESET;
	else if (commandWord == "SAVE")
	{
		std::ofstream configFile("coco3emu.conf");
		if (!configFile.is_open())
		{
			std::cout << "Error: Could not open config file for writing." << std::endl;
			return true;
		}
		
		bool anyAttached = (gimeBus.diskController.isConnected || gimeBus.emuDiskDriver.isEnabled);
		if (anyAttached)
			configFile << "[Attached]" << std::endl;
		if (gimeBus.diskController.isConnected)
			configFile << "Device = FD502" << std::endl;
		if (gimeBus.emuDiskDriver.isEnabled)
			configFile << "Device = EmuDisk" << std::endl;

		if (gimeBus.diskController.isConnected)
			for (int i = 0; i < 3; i++)
			{
				if (gimeBus.diskController.fdcDrive[i].isDriveAttached)
				{
					configFile << std::endl << "[Drive " << std::to_string(i) << "]" << std::endl;
					configFile << "PhysicalTracks = " << std::to_string(gimeBus.diskController.fdcDrive[i].totalCylinders) << std::endl;
					configFile << "Sides = " << (gimeBus.diskController.fdcDrive[i].isDoubleSided ? "2" : "1") << std::endl;
					configFile << "DataDensity = " << (gimeBus.diskController.fdcDrive[i].isDoubleDensity ? "Double" : "Single") << std::endl;
					if (gimeBus.diskController.fdcDrive[i].isDiskInserted)
						configFile << "ImageFilePath = " << gimeBus.diskController.fdcDrive[i].imgFilePathname << std::endl;
				}
			}

		if (gimeBus.emuDiskDriver.isEnabled && (gimeBus.emuDiskDriver.emuDiskDrive[0].imageMounted || gimeBus.emuDiskDriver.emuDiskDrive[1].imageMounted))
		{
			for (int i = 0; i < 2; i++)
				if (gimeBus.emuDiskDriver.emuDiskDrive[i].imageMounted)
				{
					configFile << std::endl << "[HDD " << std::to_string(i) << "]" << std::endl;
					configFile << "ImageFilePath = " << gimeBus.emuDiskDriver.emuDiskDrive[i].imgFilePathname << std::endl;
				}
		}

		for (int i = 0; i < 2; i++)
			if (gimeBus.joystickDevice[i].isAttached)
			{
				configFile << std::endl << "[Joystick]" << std::endl;
				configFile << "Port = " << gimeBus.joystickDevice[i].portName << std::endl;
				configFile << "DeviceType = " << gimeBus.joystickTypeName[gimeBus.joystickDevice[i].deviceType] << std::endl;
			}

		std::cout << "Successfully saved settings to config file." << std::endl;
		configFile.close();
	}
	else if (commandWord == "LOAD")
	{
		uint8_t configLoadStatus = loadConfigFile();
		if (configLoadStatus == CONFIG_ERROR_OPENING_FILE)
			std::cout << "Error: Could not open config file." << std::endl;
		else if (configLoadStatus == CONFIG_ERROR_INVALID_SYNTAX)
			std::cout << "Error: Invalid syntax encountered in config file. Aborted." << std::endl;
		else if (configLoadStatus == CONFIG_ERROR_MOUNTING_DISK)
			std::cout << "Error: Could not mount disk image in config file." << std::endl;
		else
			std::cout << "Successfully loaded config file." << std::endl;
	}
	else if ((commandWord == "HDD") && gimeBus.emuDiskDriver.isEnabled)
	{
		std::string subCommandWord = stringToUpper(nextStringWord(sText));
		if (subCommandWord == "MOUNT")
		{
			uint8_t targetDriveNum = std::stoi(nextStringWord(sText));
			std::string diskImageFilename = nextStringWord(sText);
			uint8_t mountResult = gimeBus.emuDiskDriver.vhdMountDisk(targetDriveNum, diskImageFilename);
			if (mountResult == EMUDISK_OPERATION_COMPLETE)
				std::cout << "Successfully mounted " << diskImageFilename << " to HDD " << std::to_string(targetDriveNum) << std::endl;
			else if (mountResult == EMUDISK_ERROR_NOT_ENABLED)
				std::cout << "Error: EmuDisk driver is not enabled." << std::endl;
			else
				std::cout << "Error: Disk image mount failed." << std::endl;
		}
		else if (subCommandWord == "EJECT")
		{
			uint8_t targetDriveNum = std::stoi(nextStringWord(sText));
			uint8_t ejectStatus = gimeBus.emuDiskDriver.vhdEjectDisk(targetDriveNum);
			if (ejectStatus == EMUDISK_ERROR_NOT_ENABLED)
				std::cout << "Error: EmuDisk driver is not enabled." << std::endl;
			else if (ejectStatus == EMUDISK_ERROR_NO_DISK_MOUNTED)
				std::cout << "Error: No HDD image file is currently mounted to HDD " << targetDriveNum << std::endl;
			else
				std::cout << "Successfully ejected HDD image file from HDD " << targetDriveNum << std::endl;
		}
	}
	else if (commandWord == "STATUS")
	{
		std::cout << std::endl << "Emulator Status" << std::endl << "---------------" << std::endl;

		for (int i = 0; i < 2; i++)
		{
			std::cout << "Joystick " << gimeBus.joystickDevice[i].portName << " Port";
			if (i == JOYSTICK_PORT_LEFT)
				std::cout << "  = ";
			else
				std::cout << " = ";
			if (gimeBus.joystickDevice[i].isAttached)
				std::cout << "Connected to " << gimeBus.joystickTypeName[gimeBus.joystickDevice[i].deviceType];
			else
				std::cout << "Not connected";
			std::cout << std::endl;
		}

		std::string strStatusFDC = gimeBus.diskController.isConnected ? "Enabled" : "Disabled";
		std::cout << std::endl << "FD502 Floppy Disk Controller = " << strStatusFDC << std::endl;
		std::string strStatusEmuDisk = gimeBus.emuDiskDriver.isEnabled ? "Enabled" : "Disabled";
		std::cout << "EmuDisk Virtual HDD Driver   = " << strStatusEmuDisk << std::endl;
		
		if (gimeBus.diskController.isConnected)
		{
			for (int i = 0; i < 3; i++)
			{
				if (gimeBus.diskController.fdcDrive[i].isDriveAttached)
				{
					std::cout << std::endl << "[Drive " << std::to_string(i) << " Connected]" << std::endl;
					std::cout << "Tracks Supported  = " << std::to_string(gimeBus.diskController.fdcDrive[i].totalCylinders) << std::endl;
					std::cout << "Double-sided?       " << (gimeBus.diskController.fdcDrive[i].isDoubleSided ? "Yes" : "No") << std::endl;
					std::cout << "Double-density?     " << (gimeBus.diskController.fdcDrive[i].isDoubleDensity ? "Yes" : "No") << std::endl;
					std::cout << "Mounted Disk Image: ";

					if (gimeBus.diskController.fdcDrive[i].isDiskInserted)
					{
						std::cout << gimeBus.diskController.fdcDrive[i].imgFilePathname << std::endl;
						std::cout << "                    - Tracks/Side     = " << std::to_string(gimeBus.diskController.fdcDrive[i].diskImageGeometry->tracksPerSide) << std::endl;
						std::cout << "                    - Sectors/Track   = " << std::to_string(gimeBus.diskController.fdcDrive[i].diskImageGeometry->sectorsPerTrack) << std::endl;
						std::cout << "                    - Number of Sides = " << std::to_string(gimeBus.diskController.fdcDrive[i].diskImageGeometry->totalSides) << std::endl;
					}
					else
						std::cout << "NONE" << std::endl;  
				}
			}
		}

		if (gimeBus.emuDiskDriver.isEnabled)
		{
			for (int i = 0; i < 2; i++)
			{
				if (gimeBus.emuDiskDriver.emuDiskDrive[i].imageMounted)
				{
					std::cout << std::endl << "[HDD " << std::to_string(i) << " Mounted]" << std::endl;
					std::cout << "Mounted Image File:  " << gimeBus.emuDiskDriver.emuDiskDrive[i].imgFilePathname << std::endl;
					std::cout << "HDD Size (in bytes): " << std::to_string(gimeBus.emuDiskDriver.emuDiskDrive[i].diskImageFilesize) << std::endl;
				}
			}
		}

	}
	else
		std::cout << "Error: Invalid command. Type HELP for a list of supported commands." << std::endl;

	std::cout << std::endl;
	return true;
}

void CoCoEmuPGE::commandLOADM(std::string inputFilename)
{
	// Load a CoCo .BIN file into RAM (for testing purposes)
	FILE* cocoBinFile = nullptr;
	cocoBinFile = fopen(inputFilename.c_str(), "rb");
	//fopen_s(&cocoBinFile, inputFilename.c_str(), "rb");
	uint8_t headerBuffer[5];
	uint8_t fileBuffer[0x10000];
	char stringBuffer[128];

	if (cocoBinFile)
	{
		fread(headerBuffer, 1, 5, cocoBinFile);
		while (headerBuffer[0] == 0x00)
		{
			dataBlockSize = (headerBuffer[1] * 256) + headerBuffer[2];
			blockLoadAddress = (headerBuffer[3] * 256) + headerBuffer[4];
			fread(&fileBuffer, 1, dataBlockSize, cocoBinFile);
			// Copy the block from our file buffer into destination CoCo physical RAM
			for (uint16_t i = 0; i < dataBlockSize; i++)
				gimeBus.writePhysicalByte(blockLoadAddress + i, fileBuffer[i]);
			fread(headerBuffer, 1, 5, cocoBinFile);
		}
		execAddress = (headerBuffer[3] * 256) + headerBuffer[4];
		uiLoadmHasExecAddr = true;
		//sprintf_s(stringBuffer, sizeof(stringBuffer), "File loaded. Execution address = $%04X", execAddress);
		sprintf(stringBuffer, "File loaded. Execution address = $%04X", execAddress);
		std::cout << stringBuffer << std::endl;
		fclose(cocoBinFile);
	}
	else
	{
		uiLoadmHasExecAddr = false;
		std::cout << "File not found." << std::endl;
	}
}

void CoCoEmuPGE::commandEXEC()
{
	if (uiLoadmHasExecAddr)
	{
		std::cout << "Jumping CPU PC to EXEC address." << std::endl;
		gimeBus.cpu.cpuReg.PC = execAddress;
	}
	else
		std::cout << "No valid BIN file loaded to EXEC." << std::endl;
}

std::string CoCoEmuPGE::nextStringWord(std::string inputString)
{
	std::string resultWord;
	// Skip any leading whitespace
	for (commandStringIndex; commandStringIndex < inputString.length(); commandStringIndex++)
		if (inputString.at(commandStringIndex) != ' ')
			break;

	if (commandStringIndex < inputString.length())
	{
		// Find end of string word
		unsigned int endOffset = inputString.find_first_of(" \r\n", commandStringIndex + 1);
		resultWord = inputString.substr(commandStringIndex, endOffset - commandStringIndex);
		commandStringIndex = endOffset + 1;
	}
	else
		resultWord = "";
	return resultWord;
}

std::string CoCoEmuPGE::stringToUpper(std::string inputString)
{
	std::string outputString = inputString;
	for (int i = 0; i < outputString.length(); i++)
		outputString[i] = std::toupper(inputString[i]);
	return outputString;
}

DeviceROM* CoCoEmuPGE::loadFileROM(const char* romFilePath)
{
	FILE* romFile = nullptr;

	romFile = fopen(romFilePath, "rb");
	//int romOpenResult = fopen_s(&romFile, romFilePath, "rb");
	//if (romOpenResult)
	if (romFile == NULL)
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

uint8_t CoCoEmuPGE::loadConfigFile()
{
	std::string inputLine;
	configResultStruct curConfigLine;
	std::ifstream configFile("coco3emu.conf");

	if (!configFile.is_open())
		return CONFIG_ERROR_OPENING_FILE;

	while (std::getline(configFile, inputLine))
	{
		if (inputLine == "[Attached]")
		{
			while (std::getline(configFile, inputLine) && !inputLine.empty())
			{
				curConfigLine = parseConfigLine(inputLine);
				if (curConfigLine.validResult && curConfigLine.paramKeyword == "DEVICE")
				{
					if (curConfigLine.paramValue == "FD502")
					{
						std::cout << "FD502 Connected" << std::endl;
						gimeBus.diskController.isConnected = true;
					}
					else if (curConfigLine.paramValue == "EMUDISK")
					{
						std::cout << "EmuDisk Driver Enabled" << std::endl;
						gimeBus.emuDiskDriver.isEnabled = true;
					}
				}
			}
		}
		else if (inputLine.substr(0, 7) == "[Drive ")
		{
			int driveNum = -1;
			if (!std::isdigit(inputLine.at(7)) || (inputLine.at(7) > '2') || (inputLine.at(8) != ']'))
				return CONFIG_ERROR_INVALID_SYNTAX;		// Invalid config file
			else
				driveNum = inputLine.at(7) - '0';	// convert ascii number to its integer equivalent

			driveStruct driveDef;
			bool validDriveConfig = true;
			std::string imageFilepath;
			
			while (std::getline(configFile, inputLine) && !inputLine.empty())
			{
				curConfigLine = parseConfigLine(inputLine);
				if (curConfigLine.validResult)
				{
					if (curConfigLine.paramKeyword == "PHYSICALTRACKS")
						try { driveDef.totalCylinders = std::stoi(curConfigLine.paramValue); }
						catch (std::exception& ex) { validDriveConfig = false; }
					else if (curConfigLine.paramKeyword == "SIDES")
					{
						if (curConfigLine.paramValue == "2")
							driveDef.isDoubleSided = true;
						else if (curConfigLine.paramValue == "1")
							driveDef.isDoubleSided = false;
						else
							validDriveConfig = false;
					}
					else if (curConfigLine.paramKeyword == "DATADENSITY")
					{
						if (curConfigLine.paramValue == "SINGLE")
							driveDef.isDoubleDensity = false;
						else if (curConfigLine.paramValue == "DOUBLE")
							driveDef.isDoubleDensity = true;
						else
							validDriveConfig = false;
					}
					else if (curConfigLine.paramKeyword == "IMAGEFILEPATH")
						imageFilepath = curConfigLine.paramValue;
					else
						validDriveConfig = false;
				}
			}
			
			if (validDriveConfig)
			{
				driveDef.isDriveAttached = true;
				gimeBus.diskController.fdcDrive[driveNum] = driveDef;
				//std::cout << "Loaded valid config for Drive " << driveNum << std::endl;
				if (!imageFilepath.empty())
				{
					if (gimeBus.diskController.fdcInsertDisk(driveNum, imageFilepath) != FD502_OPERATION_COMPLETE)
						//std::cout << "Error mounting config file disk image " << imageFilepath << std::endl;
						return CONFIG_ERROR_MOUNTING_DISK;
					//else
						//std::cout << "Successfully mounted disk image " << imageFilepath << std::endl;

				}
			}
		}
		else if (inputLine.substr(0, 5) == "[HDD ")
		{
			int driveNum = -1;
			if (!std::isdigit(inputLine.at(5)) || (inputLine.at(5) > '1') || (inputLine.at(6) != ']'))
				return CONFIG_ERROR_INVALID_SYNTAX;		// Invalid config file
			else
				driveNum = inputLine.at(5) - '0';	// convert ascii number to its integer equivalent

			while (std::getline(configFile, inputLine) && !inputLine.empty())
			{
				curConfigLine = parseConfigLine(inputLine);
				if (curConfigLine.validResult && (curConfigLine.paramKeyword == "IMAGEFILEPATH") && !curConfigLine.paramValue.empty())
				{
					if (gimeBus.emuDiskDriver.vhdMountDisk(driveNum, curConfigLine.paramValue) != EMUDISK_OPERATION_COMPLETE)
						//std::cout << "Successfully mounted HDD image " << curConfigLine.paramValue << std::endl;
						return CONFIG_ERROR_MOUNTING_DISK;
					//else
						//std::cout << "Error mounting config HDD image " << curConfigLine.paramValue << std::endl;
				}
			}
		}
		else if (inputLine.substr(0, 10) == "[Joystick]")
		{
			int portNum = -1, deviceType = -1;
			while (std::getline(configFile, inputLine) && !inputLine.empty())
			{
				curConfigLine = parseConfigLine(inputLine);
				if (curConfigLine.validResult)
				{
					if (curConfigLine.paramValue.empty())
						return CONFIG_ERROR_INVALID_SYNTAX;
					if (curConfigLine.paramKeyword == "PORT")
					{
						if (curConfigLine.paramValue == "RIGHT")
							portNum = JOYSTICK_PORT_RIGHT;
						else if (curConfigLine.paramValue == "LEFT")
							portNum = JOYSTICK_PORT_LEFT;
						else
							return CONFIG_ERROR_INVALID_SYNTAX;
					}
					else if (curConfigLine.paramKeyword == "DEVICETYPE")
					{
						if (curConfigLine.paramValue == "KEYBOARD")
							deviceType = JOYSTICK_DEVICE_KEYBOARD;
						else if (curConfigLine.paramValue == "MOUSE")
							deviceType = JOYSTICK_DEVICE_MOUSE;
						else if (curConfigLine.paramValue == "GANEPAD")
							deviceType = JOYSTICK_DEVICE_GAMEPAD;
						else
							return CONFIG_ERROR_INVALID_SYNTAX;
					}
					else
						return CONFIG_ERROR_INVALID_SYNTAX;
				}
				else
					return CONFIG_ERROR_INVALID_SYNTAX;
			}

			gimeBus.joystickDevice[portNum].deviceType = deviceType;
			gimeBus.joystickDevice[portNum].isAttached = true;
			//std::cout << "Joystick port " << gimeBus.joystickDevice[portNum].portName << " connected to device " << gimeBus.joystickTypeName[deviceType] << std::endl;
		}
	}

	configFile.close();
	return CONFIG_SUCCESSFULLY_LOADED;
}

configResultStruct CoCoEmuPGE::parseConfigLine(std::string inputLine)
{
	configResultStruct paramLineResult = { false, "", "" };

	// First skip any leading whitespace. If it finds no other valid characters, return empty string
	unsigned int indexOfFirstChar = inputLine.find_first_not_of(" \t\n\r");
	if (indexOfFirstChar == std::string::npos)
		return paramLineResult;
	// Next find the end of param keyword by searching for either equal-sign or space as delimitter
	unsigned int indexOfDelimitter = inputLine.find(" =", indexOfFirstChar);
	if (indexOfDelimitter == std::string::npos)
		return paramLineResult;

	paramLineResult.paramKeyword = stringToUpper(inputLine.substr(indexOfFirstChar, indexOfDelimitter - indexOfFirstChar));
	unsigned int indexOfParamBegin = inputLine.find_first_not_of(" \t\n\r", indexOfDelimitter + 1);
	if (indexOfParamBegin != std::string::npos)
	{
		// If the config file is using an equal-sign as delimitter, skip over it and find the next valid character
		if (inputLine.at(indexOfParamBegin) == '=')
			indexOfParamBegin = inputLine.find_first_not_of(" \t\n\r", indexOfParamBegin + 1);
		paramLineResult.paramValue = stringToUpper(inputLine.substr(indexOfParamBegin, inputLine.length() - indexOfParamBegin));
		paramLineResult.validResult = true;
	}
	return paramLineResult;
}

void CoCoEmuPGE::renderScanlineVDG(unsigned int curScanline)
{
	if (!gimeBus.vdgVideoConfig.gfxModeEnabled)
		borderPixel = olc::BLACK;
	curScanline -= 13;	// Convert physical scanline number to zero-based visible scanline offset to make this easier to read
	topBorderEnd = 25;
	bottomBorderStart = 217;

	// This IF statement should handle both the top and bottom border scanlines
	if ((curScanline < topBorderEnd) || ((curScanline >= bottomBorderStart) && (curScanline < 242)))
		DrawLine(0, curScanline, 639, curScanline, borderPixel);	// Draw top or bottom border scanline
	else 
	{
		rowPixelCounter = 64;
		DrawLine(0, curScanline, rowPixelCounter - 1, curScanline, borderPixel);

		if (!gimeBus.vdgVideoConfig.gfxModeEnabled)
		{
			screenRamPtr = (((curScanline - topBorderEnd) / 12) * vdgColumnsPerRow) + gimeBus.videoStartAddr;
			fontLineOffset = (curScanline - topBorderEnd) % 12;
			for (int y = 0; y < vdgColumnsPerRow; y++)
			{
				curScreenChar = gimeBus.physicalRAM[screenRamPtr];
				if (curScreenChar & 0x80)
				{
					// If here, we are rendering a semi-graphics mode 4 character
					vdgFontDataIndex = ((curScreenChar & 0x0F) * 12) + fontLineOffset;
					fontDataByte = sg4_fontdata8x12[vdgFontDataIndex];
					uint8_t semiGfxColor = gimeBus.gimePaletteRegs[(curScreenChar & 0x7F) >> 4];		// Mask off semi-graphics flag bit 7, and shift color bits 6-4 over to get our color value
					for (int x = 0; x < 8; x++)
					{
						fontPixel = (fontDataByte & 0x80) ? gimeBus.gimePaletteDefs[cmpOrRgb][semiGfxColor] : olc::BLACK;
						Draw(rowPixelCounter, curScanline, fontPixel);
						Draw(rowPixelCounter + 1, curScanline, fontPixel);
						fontDataByte <<= 1;
						rowPixelCounter += 2;
					}
				}
				else
				{
					// If here, we are rendering an ASCII character
					vdgFontDataIndex = ((curScreenChar & 0x3F) * 12) + fontLineOffset;		// Mask off bits 6 and 7 to get character index. 12 bytes per character bitmap entry
					if (gimeBus.vdgVideoConfig.fontIsExternal)
					{
						if (curScreenChar & 0b01000000)
							fontDataByte = lowres_font[vdgFontDataIndex];
						else
							fontDataByte = lowres_font[vdgFontDataIndex + vdgLowercaseOffset];
					}
					else
					{
						fontDataByte = lowres_font[vdgFontDataIndex];
						if (!(curScreenChar & 0b01000000))		// Check the "inverted" color bit
							fontDataByte = ~fontDataByte;	// Invert the character's bitmap
					}
					for (int x = 0; x < 8; x++)
					{
						fontPixel = (fontDataByte & 0x80) ? olc::BLACK : olc::GREEN;
						Draw(rowPixelCounter, curScanline, fontPixel);
						Draw(rowPixelCounter + 1, curScanline, fontPixel);
						fontDataByte <<= 1;
						rowPixelCounter += 2;
					}
				}
				screenRamPtr++;
			}
		}
		else
		{
			// If here, the coco is configured for a graphics mode
			screenRamPtr = (((curScanline - 25) / curLinesPerRow) * bytesPerPixelRow) + gimeBus.videoStartAddr;
			switch (gimeBus.vdgVideoConfig.colorDepth)
			{
			case 4:
				for (int y = 0; y < bytesPerPixelRow; y++)
				{
					curScreenByte = gimeBus.physicalRAM[screenRamPtr + y];
					pixelArray[0] = palDefs[cmpOrRgb][palRegs[(curScreenByte >> 6) + colorSetOffset]];
					pixelArray[1] = palDefs[cmpOrRgb][palRegs[((curScreenByte & 0b00110000) >> 4) + colorSetOffset]];
					pixelArray[2] = palDefs[cmpOrRgb][palRegs[((curScreenByte & 0b00001100) >> 2) + colorSetOffset]];
					pixelArray[3] = palDefs[cmpOrRgb][palRegs[(curScreenByte & 0b00000011) + colorSetOffset]];
					if (pixelsPerDraw == 2)
					{
						for (int x = 0; x < 4; x++)
						{
							Draw(rowPixelCounter + (x * 2), curScanline, pixelArray[x]);
							Draw(rowPixelCounter + (x * 2) + 1, curScanline, pixelArray[x]);
						}
						rowPixelCounter += 8;
					}
					else
					{
						for (int x = 0; x < 4; x++)
							DrawLine(rowPixelCounter + (x * pixelsPerDraw), curScanline, rowPixelCounter + (x * pixelsPerDraw) + 3, curScanline, pixelArray[x]);
						rowPixelCounter += (4 * pixelsPerDraw);
					}
				}
				break;
			case 2:
				for (int y = 0; y < bytesPerPixelRow; y++)
				{
					curScreenByte = gimeBus.physicalRAM[screenRamPtr + y];
					for (int x = 7; x >= 0; x--)
					{
						// In 2-color mode on a CoCo 3, the relevant palette register is offset from $FFB8, so add 8 to the index
						pixelArray[x] = palDefs[cmpOrRgb][palRegs[(curScreenByte & 0x01) + 8 + colorSetOffset]];
						curScreenByte >>= 1;
					}
					if (pixelsPerDraw == 1)
					{
						for (int x = 0; x < 8; x++)
							Draw(rowPixelCounter + x, curScanline, pixelArray[x]);
						rowPixelCounter += 8;
					}
					else
					{
						for (int x = 0; x < 8; x++)
						{
							Draw(rowPixelCounter + (x * 2), curScanline, pixelArray[x]);
							Draw(rowPixelCounter + (x * 2) + 1, curScanline, pixelArray[x]);
						}
						rowPixelCounter += 16;
					}
				}
				break;
			}
		}

		// Finally draw right-hand border
		DrawLine(rowPixelCounter, curScanline, 639, curScanline, borderPixel);
	}
}

void CoCoEmuPGE::renderScanlineGIME(unsigned int curScanline)
{
	borderPixel = gimeBus.gimePaletteDefs[cmpOrRgb][gimeBus.gimeRegBorder];
	curScanline -= 13;	// Convert physical scanline number to zero-based visible scanline offset to make this easier to read
	topBorderEnd = gimeBus.gimeTopBorderEnd[gimeBus.gimeRegVRES.LPF];
	bottomBorderStart = gimeBus.gimeBottomBorderStart[gimeBus.gimeRegVRES.LPF];

	// This IF statement should handle both the top and bottom border scanlines
	if ((curScanline < topBorderEnd) || ((curScanline >= bottomBorderStart) && (curScanline < 255)))
		DrawLine(0, curScanline, 639, curScanline, borderPixel);	// Draw top or bottom border scanline
	else
	{
		// Does the current video mode have any side borders? If so, draw left and right borders for this scanline
		if (!(gimeBus.gimeRegVRES.HRES & 0x01))
		{
			DrawLine(0, curScanline, 63, curScanline, borderPixel);
			DrawLine(640 - 64, curScanline, 639, curScanline, borderPixel);
			rowPixelCounter = 64;
		}
		else
			rowPixelCounter = 0;	// No side borders so first active pixel will be at 0 X-coordinate
		// Next we need to know if we are currently in a Text or Graphics video mode
		if (!gimeBus.gimeRegVMode.gfxOrTextMode)
		{
			// Text display mode
			screenRamPtr = (((curScanline - topBorderEnd) / curLinesPerRow) * curBytesPerCharRow) + gimeBus.videoStartAddr;
			fontLineOffset = (curScanline - topBorderEnd) % curLinesPerRow;

			for (int y = 0; y < curCharsPerRow; y++)
			{
				curScreenChar = gimeBus.physicalRAM[screenRamPtr] & 0x7F;
				if (curBytesPerChar > 1)
				{
					curScreenAttr = gimeBus.physicalRAM[screenRamPtr + 1];
					foregroundPixel = palDefs[cmpOrRgb][palRegs[((curScreenAttr & 0x3F) >> 3) + 8]];	// Bits 5-3 of attribute byte contain Foreground color number
					backgroundPixel = palDefs[cmpOrRgb][palRegs[(curScreenAttr & 0x07)]];				// Bits 2-0 of attribute byte contain Background color number
					if ((curScreenAttr & GIME_TEXT_UNDERLINE) && (curLinesPerRow > 2) && ((fontLineOffset + 1) == underlineRowOffset))
						fontDataByte = 0xFF;
					else if ((curScreenAttr & GIME_TEXT_BLINKING) && !gimeBus.gimeBlinkStateOn)
						fontDataByte = 0x00;	// If the "blink" attribute is set and the current on/off state governed by GIME timer is "off", display blank scanlines for current character
					else if (fontLineOffset > 7)
						fontDataByte = 0x00;	// If Lines Per Row video mode setting permits font heights beyond the GIME internal font's built-in 8x8 size, then display blank lines for the rest
					else
						fontDataByte = hires_font[curScreenChar][fontLineOffset];
				}
				else
				{
					// Color attributes are DISABLED. Background and Foreground colors always default to 0 and 1 respectively
					foregroundPixel = palDefs[cmpOrRgb][palRegs[1]];
					backgroundPixel = palDefs[cmpOrRgb][palRegs[0]];
					if (fontLineOffset > 7)
						fontDataByte = 0x00;	// If Lines Per Row video mode setting permits font heights beyond the GIME internal font's built-in 8, then display blank lines for the rest
					else
						fontDataByte = hires_font[curScreenChar][fontLineOffset];
				}
				// Is this a high or low resolution display mode? if it's low-res, double each pixel to conform to main window aspect ratio
				if (gimeBus.gimeRegVRES.HRES & 0x04)
				{
					// Render 8 pixels for each character in this scanline
					for (int x = 0; x < 8; x++)
					{
						fontPixel = (fontDataByte & 0x80) ? foregroundPixel : backgroundPixel;
						Draw(rowPixelCounter, curScanline, fontPixel);
						rowPixelCounter++;
						fontDataByte <<= 1;
					}
				}
				else
				{
					// Render 8 pixels for each character in this scanline
					for (int x = 0; x < 8; x++)
					{
						fontPixel = (fontDataByte & 0x80) ? foregroundPixel : backgroundPixel;
						Draw(rowPixelCounter, curScanline, fontPixel);
						Draw(rowPixelCounter + 1, curScanline, fontPixel);
						rowPixelCounter += 2;
						fontDataByte <<= 1;
					}
				}
				screenRamPtr += curBytesPerChar; 
			}
		}
		else
		{
			// GIME Graphics Mode
			screenRamPtr = (((curScanline - topBorderEnd) / curLinesPerRow) * bytesPerPixelRow) + gimeBus.videoStartAddr;
			switch (gimeBus.gimeRegVRES.CRES)
			{
			case GIME_GFX_COLORS_16:
				for (int x = 0; x < bytesPerPixelRow; x++)
				{
					curScreenByte = gimeBus.physicalRAM[screenRamPtr + x];
					pixelArray[0] = palDefs[cmpOrRgb][palRegs[curScreenByte >> 4]];
					pixelArray[1] = palDefs[cmpOrRgb][palRegs[curScreenByte & 0x0F]];

					if (pixelsPerDraw == 2)
					{
						Draw(rowPixelCounter, curScanline, pixelArray[0]);
						Draw(rowPixelCounter + 1, curScanline, pixelArray[0]);
						Draw(rowPixelCounter + 2, curScanline, pixelArray[1]);
						Draw(rowPixelCounter + 3, curScanline, pixelArray[1]);
						rowPixelCounter += 4;
					}
					else
					{
						DrawLine(rowPixelCounter, curScanline, rowPixelCounter + 3, curScanline, pixelArray[0]);
						DrawLine(rowPixelCounter + 4, curScanline, rowPixelCounter + 7, curScanline, pixelArray[1]);
						rowPixelCounter += 8;
					}
				}
				break;
			case GIME_GFX_COLORS_4:
				for (int y = 0; y < bytesPerPixelRow; y++)
				{
					curScreenByte = gimeBus.physicalRAM[screenRamPtr + y];
					pixelArray[0] = palDefs[cmpOrRgb][palRegs[curScreenByte >> 6]];
					pixelArray[1] = palDefs[cmpOrRgb][palRegs[(curScreenByte & 0b00110000) >> 4]];
					pixelArray[2] = palDefs[cmpOrRgb][palRegs[(curScreenByte & 0b00001100) >> 2]];
					pixelArray[3] = palDefs[cmpOrRgb][palRegs[(curScreenByte & 0b00000011)]];
					if (pixelsPerDraw == 1)
					{
						for (int x = 0; x < 4; x++)
							Draw(rowPixelCounter + x, curScanline, pixelArray[x]);
						rowPixelCounter += 4;
					}
					else if (pixelsPerDraw == 2)
					{
						for (int x = 0; x < 4; x++)
						{
							Draw(rowPixelCounter + (x * 2), curScanline, pixelArray[x]);
							Draw(rowPixelCounter + (x * 2) + 1, curScanline, pixelArray[x]);
						}
						rowPixelCounter += 8;
					}
					else
					{
						for (int x = 0; x < 4; x++)
							DrawLine(rowPixelCounter + (x * pixelsPerDraw), curScanline, rowPixelCounter + (x * pixelsPerDraw) + 3, curScanline, pixelArray[x]);
						rowPixelCounter += 16;
					}
				}
				break;
			case GIME_GFX_COLORS_2:
				for (int y = 0; y < bytesPerPixelRow; y++)
				{
					curScreenByte = gimeBus.physicalRAM[screenRamPtr + y];
					for (int x = 7; x >= 0; x--)
					{
						pixelArray[x] = palDefs[cmpOrRgb][palRegs[curScreenByte & 0x01]];
						curScreenByte >>= 1;
					}

					if (pixelsPerDraw == 1)
					{
						for (int x = 0; x < 8; x++)
							Draw(rowPixelCounter + x, curScanline, pixelArray[x]);
						rowPixelCounter += 8;
					}
					else if (pixelsPerDraw == 2)
					{
						for (int x = 0; x < 8; x++)
						{
							Draw(rowPixelCounter + (x * 2), curScanline, pixelArray[x]);
							Draw(rowPixelCounter + (x * 2) + 1, curScanline, pixelArray[x]);
						}
						rowPixelCounter += 16;
					}
					else
					{
						for (int x = 0; x < 8; x++)
							DrawLine(rowPixelCounter + (x * pixelsPerDraw), curScanline, rowPixelCounter + (x * pixelsPerDraw) + 3, curScanline, pixelArray[x]);
						rowPixelCounter += 32;
					}
				}
				break;
			}
		}
	}
}

bool CoCoEmuPGE::updateStatusBar()
{
	if ((gimeBus.statusBarText != "Idle") && (gimeBus.statusBarText != prevStatusBarMsg))
	{
		FillRect(0, STATUSBAR_START_Y, 640, STATUSBAR_HEIGHT, olc::GREY);
		DrawString(STATUSBAR_TEXT_OFFSET_X, STATUSBAR_TEXT_OFFSET_Y, gimeBus.statusBarText, olc::BLACK);
		prevStatusBarMsg = gimeBus.statusBarText;
		return true;
	}
		return false;
}

float CoCoEmuPGE::SoundHandler(int nChannel, float fGlobalTime, float fTimeStep)
{
	//return 0;

	if (nChannel != 0)
		return 0;

	if (leftAudioBuffer.empty())
	{
		printf("Audio buffer is empty!\n");
		return 0;
	}

	cocoAudioSample = leftAudioBuffer.front();
	leftAudioBuffer.pop();
	return cocoAudioSample;
	
	if ((nChannel != 0) && (nChannel != 1))
		printf("Weird channel!!\n");

	if (audioBufferReady)
	{
		if (leftAudioBuffer.empty() || rightAudioBuffer.empty())
		{
			printf("Error: Audio buffer %d is empty!\n", nChannel);
			return 0;
		}

		if (nChannel == 0)
		{
			cocoAudioSample = leftAudioBuffer.front();
			unsigned int size = leftAudioBuffer.size();
 			leftAudioBuffer.pop();
		}
		else if (nChannel == 1)
		{
			cocoAudioSample = rightAudioBuffer.front();
			rightAudioBuffer.pop();
		}
		return cocoAudioSample;
	}
	else
	{
		printf("Audio buffer not ready yet.\n");
		return 0;
	}

	

	//if ((gimeBus.devPIA1.SideA.dataReg >> 2) != 0)
	//	printf("Sound = %u\n", (gimeBus.devPIA1.SideA.dataReg >> 2));
	//return sin(fGlobalTime * 440.0f * 2.0f * 3.14159f);
	//return 0;
}