.PHONY: all clean

all:
	make -C kudos
	make -C userland

clean:
	make -C kudos clean
	make -C userland clean
	rm store.file
