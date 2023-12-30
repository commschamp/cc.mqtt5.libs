# Overview
This repository provides well documented and easy to use MQTT5 client library.
It is:

- single threaded
- asynchronous and non-blocking,
- fuzz testable
- compile time configurable (disable unneeded features and/or configure some data types)
- suitable for **embedded** platforms with limited resources and/or without heap

The library is completely generic and allows end application to have a complete control
over the I/O link as well as perform extra manipulation on the exchanged
raw data (such as encryption or extra framing).

# Client Library
The MQTT5 **client** library is implemented
using **C++(17)** programming language, but provides **C** interface. The library's
code doesn't use [RTTI](https://en.wikipedia.org/wiki/Run-time_type_information)
or exceptions, but by default
the library's implementation uses C++ STL data types, such as
[std::string](http://en.cppreference.com/w/cpp/string/basic_string) and
[std::vector](http://en.cppreference.com/w/cpp/container/vector). However,
it is possible to compile the library not to use any dynamic memory allocation,
and make it suitable for bare-metal environment without any heap. Please
refer to [doc/custom_client_build.md](doc/custom_client_build.md) for instructions on
how to do it. Such customization also allows removal some of the library features
resulting in smaller code size and improved runtime performance. Initial set
of features customization may be somewhat limited, but if needed please
[get in touch](#contact-information) and request the missing customization to be added.
It should not take long.

The doxygen generated documentation of the library with its full tutorial can
be downloaded from the [release artefacts](https://github.com/commschamp/cc.mqtt5.libs/releases) or browsed
[online](https://commschamp.github.io/cc_mqtt5_client_doc).

# Client Applications
This repository also provides extra utilities (example applications) which
use the [client library](#client-library) described above.

* **cc_mqtt5_client_pub** - Publish client application
* **cc_mqtt5_client_sub** - Subscribe client application

These applications use [Boost](https://www.boost.org) libraries,
([boost::program_options](https://www.boost.org/doc/libs/1_83_0/doc/html/program_options.html)
to parse the command line arguments and
[boost::asio](https://www.boost.org/doc/libs/1_83_0/doc/html/boost_asio.html) to run
the events loop and manage network connection(s)).

# How to Build
Detailed instructions on how to build and install all the components can be
found in [doc/BUILD.md](doc/BUILD.md) file.

# How to Fuzz Test
The provided MQTT5 client library as well as its dependencies from the
[CommsChampion Ecosystem](https://commschamp.github.io/) have been designed with
reliability in mind and to be able to safely handle malformed data as well as
withstand unexpected behaviour from a MQTT broker. In order to
verify the library's reliability it is highly recommended to perform
[AFL++](https://github.com/AFLplusplus/AFLplusplus) based fuzz testing.
The detailed instruction on how to fuzz test the
library can be found in [doc/fuzz_test.md](doc/fuzz_test.md) file.

# Branching Model
This repository will follow the
[Successful Git Branching Model](http://nvie.com/posts/a-successful-git-branching-model/).

The **master** branch will always point to the latest release, the
development is performed on **develop** branch. As the result it is safe
to just clone the sources of this repository and use it without
any extra manipulations of looking for the latest stable version among the tags and
checking it out.

For any pull request please submit it against the **develop** branch.

# Contact Information
For bug reports, feature requests, or any other question you may open an issue
here in **github** or e-mail me directly to: **arobenko@gmail.com**
