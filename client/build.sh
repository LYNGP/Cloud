# chmod +x build.sh
# mkdir build
rm -rf build/*
cd build
cmake ..
make clean
make -j4
