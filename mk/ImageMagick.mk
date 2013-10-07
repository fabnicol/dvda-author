
ImageMagick: Makefile
	$(call config_exec_package,$@,convert,--version 2>&1|line)



