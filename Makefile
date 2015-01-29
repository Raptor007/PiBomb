CC = g++
CFLAGS = -Wall
O = 2
LIB = picolcd asound
OBJ = $(patsubst %.cpp,%.o,$(wildcard *.cpp))

USER = bomb
INSTALL_STEPS = install-bin
ifneq (,$(wildcard /home/$(USER)/.))
	INSTALL_STEPS += install-user
endif

pibomb: $(OBJ)
	$(CC) $(CFLAGS) -O$(O) $^ $(foreach lib,$(LIB),-l$(lib)) -o $@
	-chmod ugo+rx $@

%.o: %.cpp
	$(CC) $(CFLAGS) -O$(O) -c $< -o $@

clean:
	rm -rf *.o pibomb

install: $(INSTALL_STEPS)

install-bin:
	mkdir -p /srv/PiBomb
	rsync -ax pibomb /srv/PiBomb/
	-rsync -ax *.wav /srv/PiBomb/

install-user:
	-rsync -ax bash_profile /home/$(USER)/.bash_profile
	-chown $(USER) /home/$(USER)/.bash_profile
	-chmod u+rx /home/$(USER)/.bash_profile
	-rsync -ax sudoer /etc/sudoers.d/$(USER)
	-chown root:root /etc/sudoers.d/$(USER)
	-chmod 0440 /etc/sudoers.d/$(USER)
