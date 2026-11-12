# Linux build steps

### 1. Open terminal

### 2. Install gcc and g++ compilers

```bash
sudo apt install gcc
sudo apt install g++
```
### 3. Install cmake and make
```bash
sudo apt install cmake
sudo apt install make
```
### 4. Install unzip
```bash
sudo apt install unzip
```
### 5. Download the zip file from github
```bash
wget https://github.com/AvansICT/ESE2.1-CPP/archive/refs/heads/main.zip
```
### 6. Unzip the file
```bash
unzip main.zip
```
### 7. navigate to the unzipped folder
```bash
cd ESE2.1-CPP-main/Workshop\ 7/Workshop/tcpclient/
```
### 8. Create a build folder and navigate to it
```bash
mkdir build
cd build
```
### 9. Make sure g++ is used as C++ compiler
   (note: for c files use the gcc compiler must be used)
```bash
export CXX=/usr/bin/g++
```
### 10. Run cmake
```bash
cmake ..
```
### 11. Run make
```bash
make
```
### 12. Run the program
```bash
./tcpclient
```
