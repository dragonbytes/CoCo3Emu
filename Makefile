# https://sourceforge.net/projects/linux-conioh/

OBJS=CPU6809.o CoCo3EmuPGE.o DeviceROM.o EmuDisk.o FD502.o GimeBus.o Main.o
CXXFLAGS=-I. -Dsprintf_s=sprintf -Dfopen_s=fopen -fpermissive

CoCo3Emu: $(OBJS)
	$(CXX) $(OBJS) -o $@


clean:
	$(RM) $(OBJS)
