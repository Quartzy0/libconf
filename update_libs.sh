mkdir tmp
cd include
mkdir vendor
cd vendor
rm -rf uthash
mkdir uthash
cd ../../tmp
git clone https://github.com/troydhanson/uthash
cp -r ./uthash/src/* ../include/vendor/uthash
cd ..
rm -rf ./tmp