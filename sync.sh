if [ -d ./nall ]; then rm -r ./nall; fi
if [ -d ./hiro ]; then rm -r ./hiro; fi
cp -r ../nall ./nall
cp -r ../hiro ./hiro
rm -r nall/test
rm -r hiro/nall
rm -r hiro/test
