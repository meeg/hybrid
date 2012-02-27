HPPCI_HOME := $(shell pwd)
HPPCI_KVER := $(shell uname -r)

obj-m := hppci.o

all:
	make -C /lib/modules/$(HPPCI_KVER)/build M=$(HPPCI_HOME) modules

clean:
	make -C /lib/modules/$(HPPCI_KVER)/build M=$(HPPCI_HOME) clean
