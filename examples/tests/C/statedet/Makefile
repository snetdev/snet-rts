include ../../paths.mkf

TARGET = statedet
BOXES  = add.o

include ../../rules.mkf

geninput: geninput.hs
	ghc -O2 -rtsopts $<

run:
	./$(TARGET) -i input.xml
