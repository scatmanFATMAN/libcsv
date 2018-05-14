dirs=lib test/adhoc test/unit ../doc

all:
	@for dir in $(dirs); do (cd $$dir && $(MAKE)); done

clean:
	@for dir in $(dirs); do (cd $$dir && $(MAKE) $@); done
