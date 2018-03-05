#Main Makefile

#defines
CLIENT 	 = client
IHM 	 = ihm
DUMPER 	 = dumper
HIST 	 = hist

DIRS =  $(CLIENT) $(IHM) $(DUMPER) $(HIST)

#General
QUIET = @

.PHONY: all clean install distclean clobber doc

all:
	$(QUIET)for i in $(DIRS); do cd $$i; $(MAKE); cd -;done
#	$(QUIET)find . -name "*.gch" | xargs rm

clean:
	$(QUIET)for i in $(DIRS); do cd $$i; $(MAKE) $@; cd -;  done

install:

clobber: clean
	$(QUIET)for i in $(DIRS); do cd $$i; $(MAKE) $@ ; cd -; done

doc:
	$(QUIET)doxygen Doxyfile

release:
	$(QUIET)git archive -o ~/iccp.tar.gz HEAD

# DO NOT DELETE
