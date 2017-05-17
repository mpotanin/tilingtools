libtilingtools:
	cd libtilingtools; make; cd ..
imagetiling:
	cd imagetiling; make; cd ..
copytiles:
	cd copytiles; make; cd ..
x64-release:
	mkdir x64; mkdir x64/release
	mv imagetiling/imagetiling.exe x64/release/imagetiling.exe
	mv copytiles/copytiles.exe x64/release/copytiles.exe