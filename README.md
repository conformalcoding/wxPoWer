# wxPoWer

This is _wxPoWer_: a proof-of-concept, pro-human, drop-in augmentation for textual communication platforms. It takes the message you would like to post and digitally 'signs' it by solving a proof-of-work (PoW) problem. Other users can easily verify that you have done this at any time without the need for a central authority.

Please see the `doc` directory for more information about this project.

It is not being actively developed, but documentation may be expanded in due course. Feel free to get in touch if there is any interest in it.

## Building

### Windows

You'll need:

- 7-Zip (or something that can extract `.7z` files)
- Visual Studio 2022 Community Edition

Download the following dependencies:

- [GoogleTest v1.14.0](https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip) (extract into `depends` folder as the archive contains the folder `googletest-1.14.0`)
- [RandomX v1.2.1](https://github.com/tevador/RandomX/archive/refs/tags/v1.2.1.tar.gz) (extract into `depends` folder as the archive contains the folder `RandomX-1.2.1`)
- wxWidgets v3.2.5 (extract all the below into folder `depends/wxWidgets-3.2.5` - ignore file conflicts, the replacements should be identical)
    - [Headers](https://github.com/wxWidgets/wxWidgets/releases/download/v3.2.5/wxWidgets-3.2.5-headers.7z)
    - [Development files](https://github.com/wxWidgets/wxWidgets/releases/download/v3.2.5/wxMSW-3.2.5_vc14x_x64_Dev.7z)
    - [Release DLLs](https://github.com/wxWidgets/wxWidgets/releases/download/v3.2.5/wxMSW-3.2.5_vc14x_x64_ReleaseDLL.7z)

Now open `depends/RandomX-1.2.1/src/configuration.h` and change the following macro definitions:

Macro | Default value | New value
---|---|---
`RANDOMX_ARGON_ITERATIONS` | `3` | `2`
`RANDOMX_ARGON_SALT` | `"RandomX\x03"` | `"wxPoWer\x03"`
`RANDOMX_CACHE_ACCESSES` | `8` | `10`
`RANDOMX_DATASET_EXTRA_SIZE` | `33554368` | `33554304`
`RANDOMX_PROGRAM_SIZE` | `256` | `192`

_wxPoWer_ will refuse to build without these changes to RandomX.

Using VS2022, open `depends/RandomX-1.2.1/randomx.sln` and build the `randomx` project in the Debug and Release configurations. (If building the whole solution, the `randomx-dll` project will fail to build. But it is not used, so it can be disregarded.)

Now open `wxpower_vs2022.sln` with VS2022 and build the whole solution with the Debug or Release configurations as desired.

Now, choose what you want to run by right-clicking the appropriate project and choose _Set as Startup Project_.

- To run the unit tests, set the `power_test_vs2022` project as the startup project and run.

- To run the main _wxPoWer_ application, set the `power_wxwidgets_vs2022` as the startup project and run.

### Linux

Requires the OpenSSL dev package to be installed (`openssl-dev` on Ubuntu).

At time of writing, the wxWidgets version provided by default from the Ubuntu package manager is too old. The steps on this page can be used to install version 3.2.X: https://docs.codelite.org/wxWidgets/repo32/.

Surprisingly, on the Raspberry Pi, this is not true - just install using:

```
sudo apt install libwxbase\* libwxgtk\*-dev wx-common wx\*-{headers,i18n}
```

Verify your version is acceptable using `wx-config --version`.

Go to the `depends` folder and download RandomX and GoogleTest using:

```
curl -L https://github.com/tevador/RandomX/archive/refs/tags/v1.2.1.tar.gz | tar -xz
curl -L https://github.com/google/googletest/archive/refs/tags/v1.14.0.tar.gz | tar -xz
```

GoogleTest does not need building.

Execute `patch_rx.sh` to modify the default RandomX configuration.

Now build RandomX:

```
cd RandomX-1.2.1
mkdir build
cd build
cmake .. -DARCH=native -DCMAKE_BUILD_TYPE=Release
make
```

Finally, build _wxPoWer_:

```
cd ../../../src
make
```

Run the tests:

```
./wxpowertest
```

Run the application:

```
./wxpower
```
