### Example of OpenDDS Pub / Sub architecture
#### How to run : 
1. Build idl
```
./generate_idl idl/Foo
```
2. Install deps
```
conan install . --output-folder=build --build=missing
```
3. Build
```
cd build

cmake -DCMAKE_POLICY_DEFAULT_CMP0091=NEW -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake \
    -S /home/isaac/Projects/opendds-try/different-process \
    -B /home/isaac/Projects/opendds-try/different-process/build \
    -G "Unix Makefiles"

make
```
4. Run (separate terminal)
```
./sender
./receiver
```
