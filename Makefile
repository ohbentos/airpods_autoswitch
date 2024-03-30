args = "Bento AirPods" "Razer Seiren Mini"

build:
	gcc -O2 main.c -framework CoreAudio -framework CoreFoundation -o airpods_autoswitch

run: clean build
	./airpods_autoswitch $(args)

clean:
	rm -f a.out

uninstall:
	rm -f /usr/local/bin/airpods_autoswitch
	launchctl unload ~/Library/LaunchAgents/com.airpods.autoswitch.plist
	rm -f ~/Library/LaunchAgents/com.airpods.autoswitch.plist

debug: build
	DEBUG=1 ./airpods_autoswitch $(args)

install: build
	sudo cp airpods_autoswitch /usr/local/bin/airpods_autoswitch

plist:
	cp com.airpods.autoswitch.plist ~/Library/LaunchAgents/
	launchctl load ~/Library/LaunchAgents/com.airpods.autoswitch.plist

removeplist:
	launchctl unload ~/Library/LaunchAgents/com.airpods.autoswitch.plist
