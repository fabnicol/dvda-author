#ifndef LAUNCH_MANAGER_H
#define LAUNCH_MANAGER_H

#include "structures.h"
#include "commonvars.h"

int  launch_manager(command_t *command);
int run(char* application, char* args[], int option);
#endif
