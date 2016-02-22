#include "g_commands.h"

#include <stdlib.h>

#include <driedee/render.h>

static void c_exit(cctTerm *con, int argc, char **argv)
{
	exit(0);
}

static void c_help(cctTerm *con, int argc, char **argv)
{
	cctPrintf(con, "Press tab to view commands\n");
}

static void c_addSector(cctTerm *con, int argc, char **argv)
{
	if(argc == 3){
		sectp_t sectid = createSector(atoi(argv[2]), atoi(argv[1]));
		cctPrintf(con, "Created sector with id %d\n", sectid);
	}else if(argc == 2){
		sectp_t sectid = createSector(0, atoi(argv[1]));
		cctPrintf(con, "Created sector with id %d\n", sectid);
	}else{
		cctPrintf(con, "Usage: %s ceil [floor=0]\n", argv[0]);
	}
}

static void c_addVert(cctTerm *con, int argc, char **argv)
{
	if(argc == 4){
		addVertToSector((p_t){atoi(argv[1]), atoi(argv[2])}, atoi(argv[3]));
		cctPrintf(con, "Created vertex\n");
	}else if(argc == 3){
		addVertToSector((p_t){atoi(argv[1]), atoi(argv[2])}, -1);
		cctPrintf(con, "Created vertex\n");
	}else{
		cctPrintf(con, "Usage: %s x y [neighbor=-1]\n", argv[0]);
	}
}

void mapConsoleCmds(cctTerm *console)
{
	cctMapCmd(console, "sectadd", c_addSector);
	cctMapCmd(console, "vertadd", c_addVert);
	cctMapCmd(console, "exit", c_exit);
	cctMapCmd(console, "help", c_help);
}
