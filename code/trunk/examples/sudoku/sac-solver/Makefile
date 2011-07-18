

TARGET=sudoku

SAC2C=sac2c -nophm

.PHONY: all clean test

all: $(TARGET)

$(TARGET): $(TARGET).sac
	$(SAC2C) -o $@ $<
	@echo "=============================================="
	@echo "Type 'make test' to run with an example input."

test:
	./$(TARGET) -i ../inputs/easy01.inp

clean:
	rm -fr $(TARGET) $(TARGET).c

