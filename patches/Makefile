all: anomaly_warzone_patch.so divinity_os_patch.so

anomaly_warzone_patch.so: anomaly_warzone_patch.c
	- $(CC) -m32 -o anomaly_warzone_patch.so anomaly_warzone_patch.c -ldl -shared -fPIC

divinity_os_patch.so: divinity_os_patch.c
	- $(CC) -o divinity_os_patch.so divinity_os_patch.c -s -O2  -ldl -shared -fPIC

install:
	-mkdir -p $(HOME)/.sommelier/patches
	-cp *.so $(HOME)/.sommelier/patches
	-mkdir -p $(HOME)/.sommelier/patches/TombRaider
	-cp TombRaider/* $(HOME)/.sommelier/patches/TombRaider
	-mkdir -p $(HOME)/.sommelier/patches/FantasyGeneral
	-cp FantasyGeneral/* $(HOME)/.sommelier/patches/FantasyGeneral


clean:
	rm -f *.so
