cd /Users/fab/dvda-author/
mv local local.linux
mv local.osx local
if test -d /Users/fab/local; then
  rm -rf /Users/fab/dvda-author/local
  cp -a /Users/fab/local /Users/fab/dvda-author/
fi
cd libiberty/src
make CC=/usr/local/bin/gcc-9 CXX=/usr/local/bin/g++-9 LD=/usr/local/bin/g++-9 AR=/usr/local/bin/gcc-ar-9  
cd -
cd libutils/src
make CC=/usr/local/bin/gcc-9 CXX=/usr/local/bin/g++-9 LD=/usr/local/bin/g++-9 AR=/usr/local/bin/gcc-ar-9  
cd -
cd libfixwav/src
make CC=/usr/local/bin/gcc-9 CXX=/usr/local/bin/g++-9 LD=/usr/local/bin/g++-9 AR=/usr/local/bin/gcc-ar-9
cd -
cd src
make CC=/usr/local/bin/gcc-9 CXX=/usr/local/bin/g++-9 LD=/usr/local/bin/g++-9 AR=/usr/local/bin/gcc-ar-9   
if test -f dvda-author; then 
  cp dvda-author dvda-author-dev
fi
cd -
make manpage
make htmlpage   
make install
chown -R fab .
chown root  local/bin/cdrecord
chgrp bin   local/bin/cdrecord
chmod 04755 local/bin/cdrecord
mv local local.osx
mv local.linux local

