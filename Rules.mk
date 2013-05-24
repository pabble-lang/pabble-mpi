#
# Global rules
#

.PHONY: all clean clean-all install

clean:
	$(RM) -r $(BUILD_DIR)/*
	$(RM) -r $(BIN_DIR)/*

clean-all: clean
	$(RM) $(LIB_DIR)/lib*.a
