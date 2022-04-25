
LIB_ROOT="../src/lib"

# TODO
# libraries['FIXME_STUB'] explanation
# As you did not list (-l libraryname.so) a library that exports this function
# This is a non-working stub instead.
# You can either re-run clan2py with -l /path/to/library.so
# Or manually fix this by comment the ctypes.CDLL loading

TARGET=c_eswb.py

$(TARGET):
	clang2py --clang-args="-I $(LIB_ROOT)/include/public" \
			$(LIB_ROOT)/include/public/eswb/types.h \
			$(LIB_ROOT)/include/topic_mem.h \
			$(LIB_ROOT)/include/registry.h \
			$(LIB_ROOT)/include/public/eswb/api.h \
			$(LIB_ROOT)/include/public/eswb/eqrb.h \
			-o $@

install_clang2py:
	pip3 install ctypeslib2

clean:
	-rm $(TARGET)

update: clean $(TARGET)
	@echo Done!
