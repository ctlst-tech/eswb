LIB_ROOT="../src/lib"

TARGET=c_eswb_wrappers.py

BUILD_PATH=$(shell cd ../..; pwd)

$(TARGET):
	ctypesgen -leswb --strip-build-path=$(BUILD_PATH) \
			$(LIB_ROOT)/include/public/eswb/api.h \
			$(LIB_ROOT)/include/public/eswb/errors.h \
			$(LIB_ROOT)/include/public/eswb/types.h \
			$(LIB_ROOT)/include/public/eswb/event_queue.h \
			$(LIB_ROOT)/include/public/eswb/services/eqrb.h \
			$(LIB_ROOT)/include/public/eswb/services/sdtl.h \
			$(LIB_ROOT)/include/topic_mem.h \
			$(LIB_ROOT)/include/registry.h \
			-o $@

install_tools:
	pip3 install ctypesgen

clean:
	-rm $(TARGET)

update_wrappers: clean $(TARGET)
	@echo Done!

streamer:
	sanic app_streamer.app --dev

demo:
	python3 app_demo.py

graph:
	python3 app_graph.py
