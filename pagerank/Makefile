# Build the searchio module, and symlink it into the root directory.

linalgebra: buildit
	ln -sf linalgebra/build/lib.*/linalgebra.so linalgebra.so

buildit:
	cd linalgebra; python setup.py build

clean:
	cd linalgebra; python setup.py clean
	rm -f linalgebra.so
	rm -rf linalgebra/build

