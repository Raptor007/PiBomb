CC = g++
CFLAGS = -Wall
O = 2
LIB = picolcd asound
OBJ = $(patsubst %.cpp,%.o,$(wildcard *.cpp))

pibomb: $(OBJ)
	$(CC) $(CFLAGS) -O$(O) $^ $(foreach lib,$(LIB),-l$(lib)) -o $@
	-chmod ugo+rx $@

%.o: %.cpp
	$(CC) $(CFLAGS) -O$(O) -c $< -o $@

clean:
	rm -rf *.o pibomb

install:
	mkdir -p /srv/PiBomb
	rsync -ax pibomb /srv/PiBomb/
	-rsync -ax *.wav /srv/PiBomb/
