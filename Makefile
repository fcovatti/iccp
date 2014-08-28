#Main Makefile

#defines
CLIENT 	 = client
SERVER   = #server


DIRS =  $(CLIENT) $(SERVER) 

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
