#ifndef LAUNCH_MANAGER_H
#define LAUNCH_MANAGER_H

#include "structures.h"
#include "commonvars.h"

int  launch_manager(command_t *command);
command_t *scan_wavfile_audio_characteristics(command_t *command);
#endif
