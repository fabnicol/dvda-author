

lplex: Makefile FLAC
	$(call config_exec_package,$@,lplex,--version 2>&1|line)


