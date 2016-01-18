#include "g_commands.h"

#include <stdlib.h>

#include <rogueliek/render.h>

static void c_exit(console_t *con, int argc, char **argv)
{
	exit(0);
}

static void c_help(console_t *con, int argc, char **argv)
{
	printConsole(con, "Press tab to view commands\n");
}

static void c_addSector(console_t *con, int argc, char **argv)
{
	if(argc == 3){
		sectp_t sectid = createSector(atoi(argv[2]), atoi(argv[1]));
		printConsole(con, "Created sector with id %d\n", sectid);
	}else if(argc == 2){
		sectp_t sectid = createSector(0, atoi(argv[1]));
		printConsole(con, "Created sector with id %d\n", sectid);
	}else{
		printConsole(con, "Usage: %s ceil [floor=0]\n", argv[0]);
	}
}

static void c_addVert(console_t *con, int argc, char **argv)
{
	if(argc == 4){
		addVertToSector((p_t){atoi(argv[1]), atoi(argv[2])}, atoi(argv[3]));
		printConsole(con, "Created vertex\n");
	}else if(argc == 3){
		addVertToSector((p_t){atoi(argv[1]), atoi(argv[2])}, -1);
		printConsole(con, "Created vertex\n");
	}else{
		printConsole(con, "Usage: %s x y [neighbor=-1]\n", argv[0]);
	}
}

void mapConsoleCmds(console_t *console)
{
	mapCmdConsole(console, "sectadd", c_addSector);
	mapCmdConsole(console, "vertadd", c_addVert);
	mapCmdConsole(console, "exit", c_exit);
	mapCmdConsole(console, "help", c_help);
}
