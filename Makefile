BUILD_DIR=build
CFLAGS=-Og -fPIC -Wall -Wextra -g -std=gnu11 -I./src -I/usr/include/libiberty 
LDFLAGS=-shared  -lpthread
LIBS=-lcurl -liberty -lsystemd -lz -lm
INCLUDES=src/radar.h

.PHONY: radar  clean


radar: $(BUILD_DIR)/libradar.so  $(BUILD_DIR)/radar-get   $(BUILD_DIR)/radar-dump $(BUILD_DIR)/radar-hk $(BUILD_DIR)/ret-interceptty-helper

clean: 
	@echo Nuking $(BUILD_DIR) from orbit 
	@rm -rf $(BUILD_DIR) 

$(BUILD_DIR): 
	@echo "Creating $(BUILD_DIR) if it doesn't exist"
	@mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/%.o: src/%.c $(INCLUDES) | $(BUILD_DIR)
	@echo Compiling $@
	@cc -c -o $@ $(CFLAGS) $< 


RAD_OBJS=radar.o ret-writer.o
$(BUILD_DIR)/libradar.so: $(addprefix $(BUILD_DIR)/, $(RAD_OBJS))
	@echo Linking $@
	@cc -o $@ $(LDFLAGS) $^  $(LIBS) 

$(BUILD_DIR)/%: src/%.c
	@echo Building $@
	@cc -o $@ $(CFLAGS)  $^ $(LIBS) -L$(BUILD_DIR)/ -lradar -lpthread


