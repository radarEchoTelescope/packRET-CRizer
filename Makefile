BUILD_DIR=build
CFLAGS=-fPIC -Og -Wall -Wextra -g -std=gnu11 -I./src 
LDFLAGS=-shared 
LIBS=-lcurl 
INCLUDES=src/radar.h

.PHONY: radar  clean


radar: $(BUILD_DIR)/libradar.so  $(BUILD_DIR)/radar-get 

clean: 
	@echo Nuking $(BUILD_DIR) from orbit 
	@rm -rf $(BUILD_DIR) 

$(BUILD_DIR): 
	@echo "Creating $(BUILD_DIR) if it doesn't exist"
	@mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/%.o: src/%.c $(INCLUDES) | $(BUILD_DIR)
	@echo Compiling $@
	@cc -c -o $@ $(CFLAGS) $< 


RAD_OBJS=radar.o 
$(BUILD_DIR)/libradar.so: $(addprefix $(BUILD_DIR)/, $(RAD_OBJS))
	@echo Linking $@
	@cc -o $@ $(LDFLAGS) $^  $(LIBS) 

(BUILD_DIR)/%: src/%.c
	@echo Linking $@
	@cc -o $@ $(LDFLAGS) $^  $(LIBS) -L$(BUILD_DIR)/ -lradar 


