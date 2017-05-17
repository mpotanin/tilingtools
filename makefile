release:
	cd libtilingtools; make; cd ..
	cd imagetiling; rm -f imagetiling.exe; make; cd ..
	cd copytiles; rm -f copytiles.exe; make; cd ..
	mkdir -p x64/release
	mv imagetiling/imagetiling.exe x64/release/imagetiling.exe
	mv copytiles/copytiles.exe x64/release/copytiles.exe
	\cp version.txt x64/release/version.txt