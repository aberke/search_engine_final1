# Build the searchio module, and symlink it into the root directory.

searchio: buildit
	ln -sf searchio/build/lib.*/searchio.so searchio.so

buildit:
	cd searchio; python setup.py build

clean:
	cd searchio; python setup.py clean
	rm -f searchio.so
	rm -rf searchio/build
