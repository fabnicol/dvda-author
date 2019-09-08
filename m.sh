sed -i 's/cdrtools//g' Makefile
mv local local.2
mv local.osx local

make  $PARALLEL CC=/usr/local/bin/gcc-9 CXX=/usr/local/bin/g++-9  AR=/usr/local/bin/gcc-ar-9 LD=/usr/local/bin/g++-9   -k
if ! test -f $PWD/local/bin/cdrecord || ! test -f $PWD/local/bin/mkisofs; then
  cd cdrtools-3.02
  make clean
  make CCOM=clang64
  make INS_BASE=/Users/fab/dvda-author/local install
fi
cd -
cd libiberty/src
make CC=/usr/local/bin/gcc-9 CXX=/usr/local/bin/g++-9 LD=/usr/local/bin/g++-9   
cd -
cd libutils/src
make CC=/usr/local/bin/gcc-9 CXX=/usr/local/bin/g++-9 LD=/usr/local/bin/g++-9
cd -
cd libfixwav/src
make CC=/usr/local/bin/gcc-9 CXX=/usr/local/bin/g++-9 LD=/usr/local/bin/g++-9 
cd -
cd src
make CC=/usr/local/bin/gcc-9 CXX=/usr/local/bin/g++-9 LD=/usr/local/bin/g++-9    
if test -f dvda-author; then 
  cp dvda-author dvda-author-dev
fi
cd -
make manpage
make htmlpage   
make install
chown -R fab .
mv local local.osx
mv local.2 local

