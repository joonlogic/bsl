BSL_SW_VERSION = 1.0.0
INSTALL_PATH = /bsl

export BSL_SW_VERSION
export INSTALL_PATH

SUBDIRS = module api app #scripts 
DISTDIR = diag data lib scripts log 

all:
	list='$(SUBDIRS)'; for subdir in $$list; do \
		$(MAKE) -C $$subdir; \
	done

#config:
#	cd cli; sh cli_config.sh
#	cd webs/src; sh webs_config.sh

install:
	# make directory
	list='$(DISTDIR)'; for subdir in $$list; do \
		test -d $(INSTALL_PATH)/$$subdir \
			 || mkdir -p $(INSTALL_PATH)/$$subdir \
			 || exit 1; \
	done
	# install file
	list='$(SUBDIRS)'; for subdir in $$list; do \
		$(MAKE) -C $$subdir install; \
	done
# copy nvram data file
# cp cli/unicond/nvram.dat.default $(INSTALL_PATH)/data/nvram.dat

clean:
	list='$(SUBDIRS)'; for subdir in $$list; do \
		$(MAKE) -C $$subdir clean; \
	done
