A Tandy Color Computer 3 emulator written from scratch with the intention of eventually being cross-platform.
<br><br>
<p align="center"><img src="https://github.com/user-attachments/assets/9cdb8856-64e1-4fbe-a2ed-21d617dc97e4"></p>
<br>
Why write yet another CoCo 3 emulator when we already have an excellent assortment of extremely capable and stable emulators? Well, mainly to see if I can! Ever since discovering Jeff Vavasour's CoCo emulators back in the late 90s, i've been itching to try and write one of my own someday. Over the intervening years, I have been accumulating more and more of the skills needed to actually make an attempt at it, and I finally believe I'm (mostly) ready! One thing that might make my emulator unique is that I hope to eventually have NATIVE builds for both Android and iOS, and perhaps even in the official App Store now that Apple has relaxed their restrictions on emulation apps. For the moment though, while i'm still deep in the development stage, it only builds for Windows under Visual Studio. If you find this interesting, feel free to follow along as I hopefully progress towards something I can actually call "fully functional"!
<br><br>

**Things that are working:**
- 6809 CPU Core
- Most of the various video modes (Both CoCo 3 and CoCo-compatible ones)
- Joystick support either using the keyboard or mouse for Host Computer input
- Emulated FD502 Floppy Disk Controller: (Sector Read/Write + Write Track support for formatting disks)
- Virtual Hard Drive (.vhd) Read/Write support
- Simple built-in command-line style interface for configuring the emulator, mounting disk images, etc.
- Very buggy 6-bit audio (expect clicks/glitches and distortion)

**Things NOT working or NOT implemented at all yet:**
- 6309 CPU Support
- 1-bit Audio
- Serial (Bitbanger) support
- Printer support
- Using Host Computer gamepads as input for Joystick
- A more user-friendly way to configure the emulator
- Probably alot more stuff I haven't even realized yet :-D

