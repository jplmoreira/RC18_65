UserFolder = User CS

TOPTARGETS := all clean

SUBDIRS := $(UserFolder)

$(TOPTARGETS): $(SUBDIRS)
$(SUBDIRS):
	$(MAKE) -C $@ $(MAKECMDGOALS)

.PHONY: $(TOPTARGETS) $(SUBDIRS)
