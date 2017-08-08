# pseudo makefile that allows the user to do a make clean even if he hasn't
# got GNU make or if the configure script failed.

.PHONY: all info dvi install dist check
.PHONY: mostlyclean clean toolclean distclean realclean
.PHONY: ajour depend bindist release
.PHONY: install-analyzer instal-speclib install-cmixshow install-shadows

all info dvi install dist check check_all ajour depend bindist release \
install-analyzer install-speclib install-cmixshow install-shadows:
	@echo "You need GNU make to compile C-Mix."
	@echo "If you *are* using GNU make you're getting this message"
	@echo "because the source code has not been configured properly."
	@echo Use the '`'configure"' script in the parent directory."
	@false

clean toolclean:
	@echo "Since either you don't use GNU make or you haven't run"
	@echo "the configure script yet I'll assume there's nothing to"
	@echo clean up here...

distclean realclean:
	rm -f config.h varstrings.h GNUmakefile
	rm -f config.status config.cache config.log
	rm -f *.stamp
	rm -f speclib/GNUmakefile analyzer/GNUmakefile cmixshow/GNUmakefile
	rm -f bindist/GNUmakefile
