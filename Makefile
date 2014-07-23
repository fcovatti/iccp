#Main Makefile

#defines
CLIENT 	 = client
SERVER   = #server


DIRS =  $(CLIENT) $(SERVER) 

#General
QUIET = @

.PHONY: all clean install distclean clobber doc

all:
	$(QUIET)cp ../libiec61850-iccp-0.7.7/build/libiec61850.a build
	$(QUIET)find ../libiec61850-iccp-0.7.7/ -name *.h -exec cp '{}' build/include \;
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
