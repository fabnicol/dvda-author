mjpegtools: Makefile
	$(call config_exec_package,$@,mplex,2>&1|line)

