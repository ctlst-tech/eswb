r"""Wrapper for api.h

Generated with:
/opt/homebrew/bin/ctypesgen -leswb ../src/lib/include/public/eswb/api.h ../src/lib/include/public/eswb/errors.h ../src/lib/include/public/eswb/types.h ../src/lib/include/public/eswb/event_queue.h ../src/lib/include/public/eswb/services/eqrb.h ../src/lib/include/public/eswb/services/sdtl.h ../src/lib/include/topic_mem.h ../src/lib/include/registry.h -o c_eswb_wrappers.py

Do not modify this file.
"""

__docformat__ = "restructuredtext"

# Begin preamble for Python

import ctypes
import sys
from ctypes import *  # noqa: F401, F403

_int_types = (ctypes.c_int16, ctypes.c_int32)
if hasattr(ctypes, "c_int64"):
    # Some builds of ctypes apparently do not have ctypes.c_int64
    # defined; it's a pretty good bet that these builds do not
    # have 64-bit pointers.
    _int_types += (ctypes.c_int64,)
for t in _int_types:
    if ctypes.sizeof(t) == ctypes.sizeof(ctypes.c_size_t):
        c_ptrdiff_t = t
del t
del _int_types



class UserString:
    def __init__(self, seq):
        if isinstance(seq, bytes):
            self.data = seq
        elif isinstance(seq, UserString):
            self.data = seq.data[:]
        else:
            self.data = str(seq).encode()

    def __bytes__(self):
        return self.data

    def __str__(self):
        return self.data.decode()

    def __repr__(self):
        return repr(self.data)

    def __int__(self):
        return int(self.data.decode())

    def __long__(self):
        return int(self.data.decode())

    def __float__(self):
        return float(self.data.decode())

    def __complex__(self):
        return complex(self.data.decode())

    def __hash__(self):
        return hash(self.data)

    def __le__(self, string):
        if isinstance(string, UserString):
            return self.data <= string.data
        else:
            return self.data <= string

    def __lt__(self, string):
        if isinstance(string, UserString):
            return self.data < string.data
        else:
            return self.data < string

    def __ge__(self, string):
        if isinstance(string, UserString):
            return self.data >= string.data
        else:
            return self.data >= string

    def __gt__(self, string):
        if isinstance(string, UserString):
            return self.data > string.data
        else:
            return self.data > string

    def __eq__(self, string):
        if isinstance(string, UserString):
            return self.data == string.data
        else:
            return self.data == string

    def __ne__(self, string):
        if isinstance(string, UserString):
            return self.data != string.data
        else:
            return self.data != string

    def __contains__(self, char):
        return char in self.data

    def __len__(self):
        return len(self.data)

    def __getitem__(self, index):
        return self.__class__(self.data[index])

    def __getslice__(self, start, end):
        start = max(start, 0)
        end = max(end, 0)
        return self.__class__(self.data[start:end])

    def __add__(self, other):
        if isinstance(other, UserString):
            return self.__class__(self.data + other.data)
        elif isinstance(other, bytes):
            return self.__class__(self.data + other)
        else:
            return self.__class__(self.data + str(other).encode())

    def __radd__(self, other):
        if isinstance(other, bytes):
            return self.__class__(other + self.data)
        else:
            return self.__class__(str(other).encode() + self.data)

    def __mul__(self, n):
        return self.__class__(self.data * n)

    __rmul__ = __mul__

    def __mod__(self, args):
        return self.__class__(self.data % args)

    # the following methods are defined in alphabetical order:
    def capitalize(self):
        return self.__class__(self.data.capitalize())

    def center(self, width, *args):
        return self.__class__(self.data.center(width, *args))

    def count(self, sub, start=0, end=sys.maxsize):
        return self.data.count(sub, start, end)

    def decode(self, encoding=None, errors=None):  # XXX improve this?
        if encoding:
            if errors:
                return self.__class__(self.data.decode(encoding, errors))
            else:
                return self.__class__(self.data.decode(encoding))
        else:
            return self.__class__(self.data.decode())

    def encode(self, encoding=None, errors=None):  # XXX improve this?
        if encoding:
            if errors:
                return self.__class__(self.data.encode(encoding, errors))
            else:
                return self.__class__(self.data.encode(encoding))
        else:
            return self.__class__(self.data.encode())

    def endswith(self, suffix, start=0, end=sys.maxsize):
        return self.data.endswith(suffix, start, end)

    def expandtabs(self, tabsize=8):
        return self.__class__(self.data.expandtabs(tabsize))

    def find(self, sub, start=0, end=sys.maxsize):
        return self.data.find(sub, start, end)

    def index(self, sub, start=0, end=sys.maxsize):
        return self.data.index(sub, start, end)

    def isalpha(self):
        return self.data.isalpha()

    def isalnum(self):
        return self.data.isalnum()

    def isdecimal(self):
        return self.data.isdecimal()

    def isdigit(self):
        return self.data.isdigit()

    def islower(self):
        return self.data.islower()

    def isnumeric(self):
        return self.data.isnumeric()

    def isspace(self):
        return self.data.isspace()

    def istitle(self):
        return self.data.istitle()

    def isupper(self):
        return self.data.isupper()

    def join(self, seq):
        return self.data.join(seq)

    def ljust(self, width, *args):
        return self.__class__(self.data.ljust(width, *args))

    def lower(self):
        return self.__class__(self.data.lower())

    def lstrip(self, chars=None):
        return self.__class__(self.data.lstrip(chars))

    def partition(self, sep):
        return self.data.partition(sep)

    def replace(self, old, new, maxsplit=-1):
        return self.__class__(self.data.replace(old, new, maxsplit))

    def rfind(self, sub, start=0, end=sys.maxsize):
        return self.data.rfind(sub, start, end)

    def rindex(self, sub, start=0, end=sys.maxsize):
        return self.data.rindex(sub, start, end)

    def rjust(self, width, *args):
        return self.__class__(self.data.rjust(width, *args))

    def rpartition(self, sep):
        return self.data.rpartition(sep)

    def rstrip(self, chars=None):
        return self.__class__(self.data.rstrip(chars))

    def split(self, sep=None, maxsplit=-1):
        return self.data.split(sep, maxsplit)

    def rsplit(self, sep=None, maxsplit=-1):
        return self.data.rsplit(sep, maxsplit)

    def splitlines(self, keepends=0):
        return self.data.splitlines(keepends)

    def startswith(self, prefix, start=0, end=sys.maxsize):
        return self.data.startswith(prefix, start, end)

    def strip(self, chars=None):
        return self.__class__(self.data.strip(chars))

    def swapcase(self):
        return self.__class__(self.data.swapcase())

    def title(self):
        return self.__class__(self.data.title())

    def translate(self, *args):
        return self.__class__(self.data.translate(*args))

    def upper(self):
        return self.__class__(self.data.upper())

    def zfill(self, width):
        return self.__class__(self.data.zfill(width))


class MutableString(UserString):
    """mutable string objects

    Python strings are immutable objects.  This has the advantage, that
    strings may be used as dictionary keys.  If this property isn't needed
    and you insist on changing string values in place instead, you may cheat
    and use MutableString.

    But the purpose of this class is an educational one: to prevent
    people from inventing their own mutable string class derived
    from UserString and than forget thereby to remove (override) the
    __hash__ method inherited from UserString.  This would lead to
    errors that would be very hard to track down.

    A faster and better solution is to rewrite your program using lists."""

    def __init__(self, string=""):
        self.data = string

    def __hash__(self):
        raise TypeError("unhashable type (it is mutable)")

    def __setitem__(self, index, sub):
        if index < 0:
            index += len(self.data)
        if index < 0 or index >= len(self.data):
            raise IndexError
        self.data = self.data[:index] + sub + self.data[index + 1 :]

    def __delitem__(self, index):
        if index < 0:
            index += len(self.data)
        if index < 0 or index >= len(self.data):
            raise IndexError
        self.data = self.data[:index] + self.data[index + 1 :]

    def __setslice__(self, start, end, sub):
        start = max(start, 0)
        end = max(end, 0)
        if isinstance(sub, UserString):
            self.data = self.data[:start] + sub.data + self.data[end:]
        elif isinstance(sub, bytes):
            self.data = self.data[:start] + sub + self.data[end:]
        else:
            self.data = self.data[:start] + str(sub).encode() + self.data[end:]

    def __delslice__(self, start, end):
        start = max(start, 0)
        end = max(end, 0)
        self.data = self.data[:start] + self.data[end:]

    def immutable(self):
        return UserString(self.data)

    def __iadd__(self, other):
        if isinstance(other, UserString):
            self.data += other.data
        elif isinstance(other, bytes):
            self.data += other
        else:
            self.data += str(other).encode()
        return self

    def __imul__(self, n):
        self.data *= n
        return self


class String(MutableString, ctypes.Union):

    _fields_ = [("raw", ctypes.POINTER(ctypes.c_char)), ("data", ctypes.c_char_p)]

    def __init__(self, obj=b""):
        if isinstance(obj, (bytes, UserString)):
            self.data = bytes(obj)
        else:
            self.raw = obj

    def __len__(self):
        return self.data and len(self.data) or 0

    def from_param(cls, obj):
        # Convert None or 0
        if obj is None or obj == 0:
            return cls(ctypes.POINTER(ctypes.c_char)())

        # Convert from String
        elif isinstance(obj, String):
            return obj

        # Convert from bytes
        elif isinstance(obj, bytes):
            return cls(obj)

        # Convert from str
        elif isinstance(obj, str):
            return cls(obj.encode())

        # Convert from c_char_p
        elif isinstance(obj, ctypes.c_char_p):
            return obj

        # Convert from POINTER(ctypes.c_char)
        elif isinstance(obj, ctypes.POINTER(ctypes.c_char)):
            return obj

        # Convert from raw pointer
        elif isinstance(obj, int):
            return cls(ctypes.cast(obj, ctypes.POINTER(ctypes.c_char)))

        # Convert from ctypes.c_char array
        elif isinstance(obj, ctypes.c_char * len(obj)):
            return obj

        # Convert from object
        else:
            return String.from_param(obj._as_parameter_)

    from_param = classmethod(from_param)


def ReturnString(obj, func=None, arguments=None):
    return String.from_param(obj)


# As of ctypes 1.0, ctypes does not support custom error-checking
# functions on callbacks, nor does it support custom datatypes on
# callbacks, so we must ensure that all callbacks return
# primitive datatypes.
#
# Non-primitive return values wrapped with UNCHECKED won't be
# typechecked, and will be converted to ctypes.c_void_p.
def UNCHECKED(type):
    if hasattr(type, "_type_") and isinstance(type._type_, str) and type._type_ != "P":
        return type
    else:
        return ctypes.c_void_p


# ctypes doesn't have direct support for variadic functions, so we have to write
# our own wrapper class
class _variadic_function(object):
    def __init__(self, func, restype, argtypes, errcheck):
        self.func = func
        self.func.restype = restype
        self.argtypes = argtypes
        if errcheck:
            self.func.errcheck = errcheck

    def _as_parameter_(self):
        # So we can pass this variadic function as a function pointer
        return self.func

    def __call__(self, *args):
        fixed_args = []
        i = 0
        for argtype in self.argtypes:
            # Typecheck what we can
            fixed_args.append(argtype.from_param(args[i]))
            i += 1
        return self.func(*fixed_args + list(args[i:]))


def ord_if_char(value):
    """
    Simple helper used for casts to simple builtin types:  if the argument is a
    string type, it will be converted to it's ordinal value.

    This function will raise an exception if the argument is string with more
    than one characters.
    """
    return ord(value) if (isinstance(value, bytes) or isinstance(value, str)) else value

# End preamble

_libs = {}
_libdirs = []

# Begin loader

"""
Load libraries - appropriately for all our supported platforms
"""
# ----------------------------------------------------------------------------
# Copyright (c) 2008 David James
# Copyright (c) 2006-2008 Alex Holkner
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
#  * Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
#  * Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in
#    the documentation and/or other materials provided with the
#    distribution.
#  * Neither the name of pyglet nor the names of its
#    contributors may be used to endorse or promote products
#    derived from this software without specific prior written
#    permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
# FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
# COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
# BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
# ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
# ----------------------------------------------------------------------------

import ctypes
import ctypes.util
import glob
import os.path
import platform
import re
import sys


def _environ_path(name):
    """Split an environment variable into a path-like list elements"""
    if name in os.environ:
        return os.environ[name].split(":")
    return []


class LibraryLoader:
    """
    A base class For loading of libraries ;-)
    Subclasses load libraries for specific platforms.
    """

    # library names formatted specifically for platforms
    name_formats = ["%s"]

    class Lookup:
        """Looking up calling conventions for a platform"""

        mode = ctypes.DEFAULT_MODE

        def __init__(self, path):
            super(LibraryLoader.Lookup, self).__init__()
            self.access = dict(cdecl=ctypes.CDLL(path, self.mode))

        def get(self, name, calling_convention="cdecl"):
            """Return the given name according to the selected calling convention"""
            if calling_convention not in self.access:
                raise LookupError(
                    "Unknown calling convention '{}' for function '{}'".format(
                        calling_convention, name
                    )
                )
            return getattr(self.access[calling_convention], name)

        def has(self, name, calling_convention="cdecl"):
            """Return True if this given calling convention finds the given 'name'"""
            if calling_convention not in self.access:
                return False
            return hasattr(self.access[calling_convention], name)

        def __getattr__(self, name):
            return getattr(self.access["cdecl"], name)

    def __init__(self):
        self.other_dirs = []

    def __call__(self, libname):
        """Given the name of a library, load it."""
        paths = self.getpaths(libname)

        for path in paths:
            # noinspection PyBroadException
            try:
                return self.Lookup(path)
            except Exception:  # pylint: disable=broad-except
                pass

        raise ImportError("Could not load %s." % libname)

    def getpaths(self, libname):
        """Return a list of paths where the library might be found."""
        if os.path.isabs(libname):
            yield libname
        else:
            # search through a prioritized series of locations for the library

            # we first search any specific directories identified by user
            for dir_i in self.other_dirs:
                for fmt in self.name_formats:
                    # dir_i should be absolute already
                    yield os.path.join(dir_i, fmt % libname)

            # check if this code is even stored in a physical file
            try:
                this_file = __file__
            except NameError:
                this_file = None

            # then we search the directory where the generated python interface is stored
            if this_file is not None:
                for fmt in self.name_formats:
                    yield os.path.abspath(os.path.join(os.path.dirname(__file__), fmt % libname))

            # now, use the ctypes tools to try to find the library
            for fmt in self.name_formats:
                path = ctypes.util.find_library(fmt % libname)
                if path:
                    yield path

            # then we search all paths identified as platform-specific lib paths
            for path in self.getplatformpaths(libname):
                yield path

            # Finally, we'll try the users current working directory
            for fmt in self.name_formats:
                yield os.path.abspath(os.path.join(os.path.curdir, fmt % libname))

    def getplatformpaths(self, _libname):  # pylint: disable=no-self-use
        """Return all the library paths available in this platform"""
        return []


# Darwin (Mac OS X)


class DarwinLibraryLoader(LibraryLoader):
    """Library loader for MacOS"""

    name_formats = [
        "lib%s.dylib",
        "lib%s.so",
        "lib%s.bundle",
        "%s.dylib",
        "%s.so",
        "%s.bundle",
        "%s",
    ]

    class Lookup(LibraryLoader.Lookup):
        """
        Looking up library files for this platform (Darwin aka MacOS)
        """

        # Darwin requires dlopen to be called with mode RTLD_GLOBAL instead
        # of the default RTLD_LOCAL.  Without this, you end up with
        # libraries not being loadable, resulting in "Symbol not found"
        # errors
        mode = ctypes.RTLD_GLOBAL

    def getplatformpaths(self, libname):
        if os.path.pathsep in libname:
            names = [libname]
        else:
            names = [fmt % libname for fmt in self.name_formats]

        for directory in self.getdirs(libname):
            for name in names:
                yield os.path.join(directory, name)

    @staticmethod
    def getdirs(libname):
        """Implements the dylib search as specified in Apple documentation:

        http://developer.apple.com/documentation/DeveloperTools/Conceptual/
            DynamicLibraries/Articles/DynamicLibraryUsageGuidelines.html

        Before commencing the standard search, the method first checks
        the bundle's ``Frameworks`` directory if the application is running
        within a bundle (OS X .app).
        """

        dyld_fallback_library_path = _environ_path("DYLD_FALLBACK_LIBRARY_PATH")
        if not dyld_fallback_library_path:
            dyld_fallback_library_path = [
                os.path.expanduser("~/lib"),
                "/usr/local/lib",
                "/usr/lib",
            ]

        dirs = []

        if "/" in libname:
            dirs.extend(_environ_path("DYLD_LIBRARY_PATH"))
        else:
            dirs.extend(_environ_path("LD_LIBRARY_PATH"))
            dirs.extend(_environ_path("DYLD_LIBRARY_PATH"))
            dirs.extend(_environ_path("LD_RUN_PATH"))

        if hasattr(sys, "frozen") and getattr(sys, "frozen") == "macosx_app":
            dirs.append(os.path.join(os.environ["RESOURCEPATH"], "..", "Frameworks"))

        dirs.extend(dyld_fallback_library_path)

        return dirs


# Posix


class PosixLibraryLoader(LibraryLoader):
    """Library loader for POSIX-like systems (including Linux)"""

    _ld_so_cache = None

    _include = re.compile(r"^\s*include\s+(?P<pattern>.*)")

    name_formats = ["lib%s.so", "%s.so", "%s"]

    class _Directories(dict):
        """Deal with directories"""

        def __init__(self):
            dict.__init__(self)
            self.order = 0

        def add(self, directory):
            """Add a directory to our current set of directories"""
            if len(directory) > 1:
                directory = directory.rstrip(os.path.sep)
            # only adds and updates order if exists and not already in set
            if not os.path.exists(directory):
                return
            order = self.setdefault(directory, self.order)
            if order == self.order:
                self.order += 1

        def extend(self, directories):
            """Add a list of directories to our set"""
            for a_dir in directories:
                self.add(a_dir)

        def ordered(self):
            """Sort the list of directories"""
            return (i[0] for i in sorted(self.items(), key=lambda d: d[1]))

    def _get_ld_so_conf_dirs(self, conf, dirs):
        """
        Recursive function to help parse all ld.so.conf files, including proper
        handling of the `include` directive.
        """

        try:
            with open(conf) as fileobj:
                for dirname in fileobj:
                    dirname = dirname.strip()
                    if not dirname:
                        continue

                    match = self._include.match(dirname)
                    if not match:
                        dirs.add(dirname)
                    else:
                        for dir2 in glob.glob(match.group("pattern")):
                            self._get_ld_so_conf_dirs(dir2, dirs)
        except IOError:
            pass

    def _create_ld_so_cache(self):
        # Recreate search path followed by ld.so.  This is going to be
        # slow to build, and incorrect (ld.so uses ld.so.cache, which may
        # not be up-to-date).  Used only as fallback for distros without
        # /sbin/ldconfig.
        #
        # We assume the DT_RPATH and DT_RUNPATH binary sections are omitted.

        directories = self._Directories()
        for name in (
            "LD_LIBRARY_PATH",
            "SHLIB_PATH",  # HP-UX
            "LIBPATH",  # OS/2, AIX
            "LIBRARY_PATH",  # BE/OS
        ):
            if name in os.environ:
                directories.extend(os.environ[name].split(os.pathsep))

        self._get_ld_so_conf_dirs("/etc/ld.so.conf", directories)

        bitage = platform.architecture()[0]

        unix_lib_dirs_list = []
        if bitage.startswith("64"):
            # prefer 64 bit if that is our arch
            unix_lib_dirs_list += ["/lib64", "/usr/lib64"]

        # must include standard libs, since those paths are also used by 64 bit
        # installs
        unix_lib_dirs_list += ["/lib", "/usr/lib"]
        if sys.platform.startswith("linux"):
            # Try and support multiarch work in Ubuntu
            # https://wiki.ubuntu.com/MultiarchSpec
            if bitage.startswith("32"):
                # Assume Intel/AMD x86 compat
                unix_lib_dirs_list += ["/lib/i386-linux-gnu", "/usr/lib/i386-linux-gnu"]
            elif bitage.startswith("64"):
                # Assume Intel/AMD x86 compatible
                unix_lib_dirs_list += [
                    "/lib/x86_64-linux-gnu",
                    "/usr/lib/x86_64-linux-gnu",
                ]
            else:
                # guess...
                unix_lib_dirs_list += glob.glob("/lib/*linux-gnu")
        directories.extend(unix_lib_dirs_list)

        cache = {}
        lib_re = re.compile(r"lib(.*)\.s[ol]")
        # ext_re = re.compile(r"\.s[ol]$")
        for our_dir in directories.ordered():
            try:
                for path in glob.glob("%s/*.s[ol]*" % our_dir):
                    file = os.path.basename(path)

                    # Index by filename
                    cache_i = cache.setdefault(file, set())
                    cache_i.add(path)

                    # Index by library name
                    match = lib_re.match(file)
                    if match:
                        library = match.group(1)
                        cache_i = cache.setdefault(library, set())
                        cache_i.add(path)
            except OSError:
                pass

        self._ld_so_cache = cache

    def getplatformpaths(self, libname):
        if self._ld_so_cache is None:
            self._create_ld_so_cache()

        result = self._ld_so_cache.get(libname, set())
        for i in result:
            # we iterate through all found paths for library, since we may have
            # actually found multiple architectures or other library types that
            # may not load
            yield i


# Windows


class WindowsLibraryLoader(LibraryLoader):
    """Library loader for Microsoft Windows"""

    name_formats = ["%s.dll", "lib%s.dll", "%slib.dll", "%s"]

    class Lookup(LibraryLoader.Lookup):
        """Lookup class for Windows libraries..."""

        def __init__(self, path):
            super(WindowsLibraryLoader.Lookup, self).__init__(path)
            self.access["stdcall"] = ctypes.windll.LoadLibrary(path)


# Platform switching

# If your value of sys.platform does not appear in this dict, please contact
# the Ctypesgen maintainers.

loaderclass = {
    "darwin": DarwinLibraryLoader,
    "cygwin": WindowsLibraryLoader,
    "win32": WindowsLibraryLoader,
    "msys": WindowsLibraryLoader,
}

load_library = loaderclass.get(sys.platform, PosixLibraryLoader)()


def add_library_search_dirs(other_dirs):
    """
    Add libraries to search paths.
    If library paths are relative, convert them to absolute with respect to this
    file's directory
    """
    for path in other_dirs:
        if not os.path.isabs(path):
            path = os.path.abspath(path)
        load_library.other_dirs.append(path)


del loaderclass

# End loader

add_library_search_dirs([])

# Begin libraries
_libs["eswb"] = load_library("eswb")

# 1 libraries
# End libraries

# No modules

uint8_t = c_ubyte# /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/_types/_uint8_t.h: 31

uint16_t = c_ushort# /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/_types/_uint16_t.h: 31

uint32_t = c_uint# /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/_types/_uint32_t.h: 31

__darwin_dev_t = c_int32# /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/sys/_types.h: 57

u_int32_t = c_uint# /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/sys/_types/_u_int32_t.h: 30

u_int64_t = c_ulonglong# /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/sys/_types/_u_int64_t.h: 30

register_t = c_int64# /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/arm/types.h: 66

uintptr_t = c_ulong# /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/sys/_types/_uintptr_t.h: 34

user_addr_t = u_int64_t# /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/arm/types.h: 77

user_size_t = u_int64_t# /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/arm/types.h: 78

user_ssize_t = c_int64# /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/arm/types.h: 79

user_long_t = c_int64# /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/arm/types.h: 80

user_ulong_t = u_int64_t# /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/arm/types.h: 81

user_time_t = c_int64# /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/arm/types.h: 82

user_off_t = c_int64# /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/arm/types.h: 83

syscall_arg_t = u_int64_t# /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/arm/types.h: 104

enum_anon_2 = c_int# /usr/local/include/eswb/errors.h: 59

eswb_e_ok = 0# /usr/local/include/eswb/errors.h: 59

eswb_e_name_too_long = (eswb_e_ok + 1)# /usr/local/include/eswb/errors.h: 59

eswb_e_invargs = (eswb_e_name_too_long + 1)# /usr/local/include/eswb/errors.h: 59

eswb_e_type_missmatch = (eswb_e_invargs + 1)# /usr/local/include/eswb/errors.h: 59

eswb_e_notdir = (eswb_e_type_missmatch + 1)# /usr/local/include/eswb/errors.h: 59

eswb_e_path_too_long = (eswb_e_notdir + 1)# /usr/local/include/eswb/errors.h: 59

eswb_e_timedout = (eswb_e_path_too_long + 1)# /usr/local/include/eswb/errors.h: 59

eswb_e_sync_init = (eswb_e_timedout + 1)# /usr/local/include/eswb/errors.h: 59

eswb_e_sync_take = (eswb_e_sync_init + 1)# /usr/local/include/eswb/errors.h: 59

eswb_e_sync_inconsistent = (eswb_e_sync_take + 1)# /usr/local/include/eswb/errors.h: 59

eswb_e_sync_give = (eswb_e_sync_inconsistent + 1)# /usr/local/include/eswb/errors.h: 59

eswb_e_sync_wait = (eswb_e_sync_give + 1)# /usr/local/include/eswb/errors.h: 59

eswb_e_sync_broadcast = (eswb_e_sync_wait + 1)# /usr/local/include/eswb/errors.h: 59

eswb_e_no_update = (eswb_e_sync_broadcast + 1)# /usr/local/include/eswb/errors.h: 59

eswb_e_mem_topic_max = (eswb_e_no_update + 1)# /usr/local/include/eswb/errors.h: 59

eswb_e_mem_reg_na = (eswb_e_mem_topic_max + 1)# /usr/local/include/eswb/errors.h: 59

eswb_e_mem_topic_na = (eswb_e_mem_reg_na + 1)# /usr/local/include/eswb/errors.h: 59

eswb_e_mem_sync_na = (eswb_e_mem_topic_na + 1)# /usr/local/include/eswb/errors.h: 59

eswb_e_mem_data_na = (eswb_e_mem_sync_na + 1)# /usr/local/include/eswb/errors.h: 59

eswb_e_mem_static_exceeded = (eswb_e_mem_data_na + 1)# /usr/local/include/eswb/errors.h: 59

eswb_e_vector_inv_index = (eswb_e_mem_static_exceeded + 1)# /usr/local/include/eswb/errors.h: 59

eswb_e_vector_len_exceeded = (eswb_e_vector_inv_index + 1)# /usr/local/include/eswb/errors.h: 59

eswb_e_inv_naming = (eswb_e_vector_len_exceeded + 1)# /usr/local/include/eswb/errors.h: 59

eswb_e_inv_bus_spec = (eswb_e_inv_naming + 1)# /usr/local/include/eswb/errors.h: 59

eswb_e_no_topic = (eswb_e_inv_bus_spec + 1)# /usr/local/include/eswb/errors.h: 59

eswb_e_topic_exist = (eswb_e_no_topic + 1)# /usr/local/include/eswb/errors.h: 59

eswb_e_topic_is_not_dir = (eswb_e_topic_exist + 1)# /usr/local/include/eswb/errors.h: 59

eswb_e_not_fifo = (eswb_e_topic_is_not_dir + 1)# /usr/local/include/eswb/errors.h: 59

eswb_e_not_vector = (eswb_e_not_fifo + 1)# /usr/local/include/eswb/errors.h: 59

eswb_e_not_evq = (eswb_e_not_vector + 1)# /usr/local/include/eswb/errors.h: 59

eswb_e_fifo_rcvr_underrun = (eswb_e_not_evq + 1)# /usr/local/include/eswb/errors.h: 59

eswb_e_not_supported = (eswb_e_fifo_rcvr_underrun + 1)# /usr/local/include/eswb/errors.h: 59

eswb_e_bus_not_exist = (eswb_e_not_supported + 1)# /usr/local/include/eswb/errors.h: 59

eswb_e_bus_exists = (eswb_e_bus_not_exist + 1)# /usr/local/include/eswb/errors.h: 59

eswb_e_max_busses_reached = (eswb_e_bus_exists + 1)# /usr/local/include/eswb/errors.h: 59

eswb_e_max_topic_desrcs = (eswb_e_max_busses_reached + 1)# /usr/local/include/eswb/errors.h: 59

eswb_e_ev_queue_payload_too_large = (eswb_e_max_topic_desrcs + 1)# /usr/local/include/eswb/errors.h: 59

eswb_e_ev_queue_not_enabled = (eswb_e_ev_queue_payload_too_large + 1)# /usr/local/include/eswb/errors.h: 59

eswb_e_repl_prtree_fraiming_error = (eswb_e_ev_queue_not_enabled + 1)# /usr/local/include/eswb/errors.h: 59

eswb_e_map_full = (eswb_e_repl_prtree_fraiming_error + 1)# /usr/local/include/eswb/errors.h: 59

eswb_e_map_key_exists = (eswb_e_map_full + 1)# /usr/local/include/eswb/errors.h: 59

eswb_e_map_no_mem = (eswb_e_map_key_exists + 1)# /usr/local/include/eswb/errors.h: 59

eswb_e_map_no_match = (eswb_e_map_no_mem + 1)# /usr/local/include/eswb/errors.h: 59

eswb_e_bridge_vector = (eswb_e_map_no_match + 1)# /usr/local/include/eswb/errors.h: 59

eswb_rv_t = enum_anon_2# /usr/local/include/eswb/errors.h: 59

# /usr/local/include/eswb/errors.h: 61
if _libs["eswb"].has("eswb_strerror", "cdecl"):
    eswb_strerror = _libs["eswb"].get("eswb_strerror", "cdecl")
    eswb_strerror.argtypes = [eswb_rv_t]
    eswb_strerror.restype = c_char_p

eswb_index_t = uint32_t# /usr/local/include/eswb/types.h: 19

eswb_update_counter_t = uint32_t# /usr/local/include/eswb/types.h: 20

eswb_fifo_index_t = uint16_t# /usr/local/include/eswb/types.h: 21

eswb_size_t = uint32_t# /usr/local/include/eswb/types.h: 22

eswb_topic_id_t = uint32_t# /usr/local/include/eswb/types.h: 23

eswb_event_queue_mask_t = uint32_t# /usr/local/include/eswb/types.h: 24

enum_anon_3 = c_int# /usr/local/include/eswb/types.h: 56

tt_none = 0x00# /usr/local/include/eswb/types.h: 56

tt_dir = 0x01# /usr/local/include/eswb/types.h: 56

tt_struct = 0x02# /usr/local/include/eswb/types.h: 56

tt_fifo = 0x03# /usr/local/include/eswb/types.h: 56

tt_vector = 0x04# /usr/local/include/eswb/types.h: 56

tt_bitfield = 0x04# /usr/local/include/eswb/types.h: 56

tt_uint8 = 0x10# /usr/local/include/eswb/types.h: 56

tt_int8 = 0x11# /usr/local/include/eswb/types.h: 56

tt_uint16 = 0x12# /usr/local/include/eswb/types.h: 56

tt_int16 = 0x13# /usr/local/include/eswb/types.h: 56

tt_uint32 = 0x14# /usr/local/include/eswb/types.h: 56

tt_int32 = 0x15# /usr/local/include/eswb/types.h: 56

tt_uint64 = 0x16# /usr/local/include/eswb/types.h: 56

tt_int64 = 0x17# /usr/local/include/eswb/types.h: 56

tt_float = 0x20# /usr/local/include/eswb/types.h: 56

tt_double = 0x21# /usr/local/include/eswb/types.h: 56

tt_string = 0x30# /usr/local/include/eswb/types.h: 56

tt_plain_data = 0x31# /usr/local/include/eswb/types.h: 56

tt_byte_buffer = 0x40# /usr/local/include/eswb/types.h: 56

tt_event_queue = 0xF0# /usr/local/include/eswb/types.h: 56

topic_data_type_t = enum_anon_3# /usr/local/include/eswb/types.h: 56

topic_data_type_s_t = uint8_t# /usr/local/include/eswb/types.h: 58

enum_anon_4 = c_int# /usr/local/include/eswb/types.h: 67

upd_proclaim_topic = 0# /usr/local/include/eswb/types.h: 67

upd_update_topic = (upd_proclaim_topic + 1)# /usr/local/include/eswb/types.h: 67

upd_push_fifo = (upd_update_topic + 1)# /usr/local/include/eswb/types.h: 67

upd_write_vector = (upd_push_fifo + 1)# /usr/local/include/eswb/types.h: 67

upd_push_event_queue = (upd_write_vector + 1)# /usr/local/include/eswb/types.h: 67

upd_withdraw_topic = (upd_push_event_queue + 1)# /usr/local/include/eswb/types.h: 67

eswb_update_t = enum_anon_4# /usr/local/include/eswb/types.h: 67

enum_anon_5 = c_int# /usr/local/include/eswb/types.h: 75

eswb_not_defined = 0# /usr/local/include/eswb/types.h: 75

eswb_non_synced = (eswb_not_defined + 1)# /usr/local/include/eswb/types.h: 75

eswb_inter_thread = (eswb_non_synced + 1)# /usr/local/include/eswb/types.h: 75

eswb_inter_process = (eswb_inter_thread + 1)# /usr/local/include/eswb/types.h: 75

eswb_type_t = enum_anon_5# /usr/local/include/eswb/types.h: 75

enum_anon_6 = c_int# /usr/local/include/eswb/types.h: 86

eswb_ctl_enable_event_queue = 0# /usr/local/include/eswb/types.h: 86

eswb_ctl_request_topics_to_evq = (eswb_ctl_enable_event_queue + 1)# /usr/local/include/eswb/types.h: 86

eswb_ctl_evq_set_receive_mask = (eswb_ctl_request_topics_to_evq + 1)# /usr/local/include/eswb/types.h: 86

eswb_ctl_evq_get_params = (eswb_ctl_evq_set_receive_mask + 1)# /usr/local/include/eswb/types.h: 86

eswb_ctl_get_topic_path = (eswb_ctl_evq_get_params + 1)# /usr/local/include/eswb/types.h: 86

eswb_ctl_get_next_proclaiming_info = (eswb_ctl_get_topic_path + 1)# /usr/local/include/eswb/types.h: 86

eswb_ctl_fifo_flush = (eswb_ctl_get_next_proclaiming_info + 1)# /usr/local/include/eswb/types.h: 86

eswb_ctl_arm_timeout = (eswb_ctl_fifo_flush + 1)# /usr/local/include/eswb/types.h: 86

eswb_ctl_t = enum_anon_6# /usr/local/include/eswb/types.h: 86

eswb_topic_descr_t = c_int# /usr/local/include/eswb/types.h: 94

# /usr/local/include/eswb/types.h: 101
class struct_anon_7(Structure):
    pass

struct_anon_7.__slots__ = [
    'name',
    'parent_name',
    'type',
    'size',
]
struct_anon_7._fields_ = [
    ('name', c_char * int((30 + 1))),
    ('parent_name', c_char * int((30 + 1))),
    ('type', topic_data_type_t),
    ('size', eswb_size_t),
]

topic_params_t = struct_anon_7# /usr/local/include/eswb/types.h: 101

# /usr/local/include/eswb/types.h: 107
class struct_anon_8(Structure):
    pass

struct_anon_8.__slots__ = [
    'elems_num',
    'elem_index',
    'flags',
]
struct_anon_8._fields_ = [
    ('elems_num', eswb_size_t),
    ('elem_index', eswb_size_t),
    ('flags', uint32_t),
]

array_alter_t = struct_anon_8# /usr/local/include/eswb/types.h: 107

# /usr/local/include/eswb/types.h: 109
if _libs["eswb"].has("eswb_type_name", "cdecl"):
    eswb_type_name = _libs["eswb"].get("eswb_type_name", "cdecl")
    eswb_type_name.argtypes = [topic_data_type_t]
    eswb_type_name.restype = c_char_p

# /usr/local/include/eswb/topic_proclaiming_tree.h: 43
class struct_anon_9(Structure):
    pass

struct_anon_9._pack_ = 1
struct_anon_9.__slots__ = [
    'name',
    'abs_ind',
    'type',
    'data_offset',
    'data_size',
    'flags',
    'topic_id',
    'parent_ind',
    'first_child_ind',
    'next_sibling_ind',
]
struct_anon_9._fields_ = [
    ('name', c_char * int(((30 + 1) + ((30 + 1) % 4)))),
    ('abs_ind', c_int32),
    ('type', topic_data_type_s_t),
    ('data_offset', uint16_t),
    ('data_size', uint16_t),
    ('flags', uint16_t),
    ('topic_id', uint16_t),
    ('parent_ind', c_int16),
    ('first_child_ind', c_int16),
    ('next_sibling_ind', c_int16),
]

topic_proclaiming_tree_t = struct_anon_9# /usr/local/include/eswb/topic_proclaiming_tree.h: 43

# /usr/local/include/eswb/topic_proclaiming_tree.h: 48
class struct_topic_extract(Structure):
    pass

struct_topic_extract.__slots__ = [
    'info',
    'parent_id',
]
struct_topic_extract._fields_ = [
    ('info', topic_proclaiming_tree_t),
    ('parent_id', eswb_topic_id_t),
]

topic_extract_t = struct_topic_extract# /usr/local/include/eswb/topic_proclaiming_tree.h: 48

# eswb/src/lib/include/public/eswb/api.h: 26
if _libs["eswb"].has("eswb_local_init", "cdecl"):
    eswb_local_init = _libs["eswb"].get("eswb_local_init", "cdecl")
    eswb_local_init.argtypes = [c_int]
    eswb_local_init.restype = eswb_rv_t

# eswb/src/lib/include/public/eswb/api.h: 38
if _libs["eswb"].has("eswb_create", "cdecl"):
    eswb_create = _libs["eswb"].get("eswb_create", "cdecl")
    eswb_create.argtypes = [String, eswb_type_t, eswb_size_t]
    eswb_create.restype = eswb_rv_t

# eswb/src/lib/include/public/eswb/api.h: 45
if _libs["eswb"].has("eswb_delete", "cdecl"):
    eswb_delete = _libs["eswb"].get("eswb_delete", "cdecl")
    eswb_delete.argtypes = [String]
    eswb_delete.restype = eswb_rv_t

# eswb/src/lib/include/public/eswb/api.h: 52
if _libs["eswb"].has("eswb_delete_by_td", "cdecl"):
    eswb_delete_by_td = _libs["eswb"].get("eswb_delete_by_td", "cdecl")
    eswb_delete_by_td.argtypes = [eswb_topic_descr_t]
    eswb_delete_by_td.restype = eswb_rv_t

# eswb/src/lib/include/public/eswb/api.h: 60
if _libs["eswb"].has("eswb_mkdir", "cdecl"):
    eswb_mkdir = _libs["eswb"].get("eswb_mkdir", "cdecl")
    eswb_mkdir.argtypes = [String, String]
    eswb_mkdir.restype = eswb_rv_t

# eswb/src/lib/include/public/eswb/api.h: 68
if _libs["eswb"].has("eswb_mkdir_nested", "cdecl"):
    eswb_mkdir_nested = _libs["eswb"].get("eswb_mkdir_nested", "cdecl")
    eswb_mkdir_nested.argtypes = [eswb_topic_descr_t, String, POINTER(eswb_topic_descr_t)]
    eswb_mkdir_nested.restype = eswb_rv_t

# eswb/src/lib/include/public/eswb/api.h: 79
if _libs["eswb"].has("eswb_proclaim_tree", "cdecl"):
    eswb_proclaim_tree = _libs["eswb"].get("eswb_proclaim_tree", "cdecl")
    eswb_proclaim_tree.argtypes = [eswb_topic_descr_t, POINTER(topic_proclaiming_tree_t), eswb_size_t, POINTER(eswb_topic_descr_t)]
    eswb_proclaim_tree.restype = eswb_rv_t

# eswb/src/lib/include/public/eswb/api.h: 90
if _libs["eswb"].has("eswb_proclaim_tree_by_path", "cdecl"):
    eswb_proclaim_tree_by_path = _libs["eswb"].get("eswb_proclaim_tree_by_path", "cdecl")
    eswb_proclaim_tree_by_path.argtypes = [String, POINTER(topic_proclaiming_tree_t), eswb_size_t, POINTER(eswb_topic_descr_t)]
    eswb_proclaim_tree_by_path.restype = eswb_rv_t

# eswb/src/lib/include/public/eswb/api.h: 101
if _libs["eswb"].has("eswb_proclaim_plain", "cdecl"):
    eswb_proclaim_plain = _libs["eswb"].get("eswb_proclaim_plain", "cdecl")
    eswb_proclaim_plain.argtypes = [String, String, c_size_t, POINTER(eswb_topic_descr_t)]
    eswb_proclaim_plain.restype = eswb_rv_t

# eswb/src/lib/include/public/eswb/api.h: 109
if _libs["eswb"].has("eswb_update_topic", "cdecl"):
    eswb_update_topic = _libs["eswb"].get("eswb_update_topic", "cdecl")
    eswb_update_topic.argtypes = [eswb_topic_descr_t, POINTER(None)]
    eswb_update_topic.restype = eswb_rv_t

# eswb/src/lib/include/public/eswb/api.h: 117
if _libs["eswb"].has("eswb_connect", "cdecl"):
    eswb_connect = _libs["eswb"].get("eswb_connect", "cdecl")
    eswb_connect.argtypes = [String, POINTER(eswb_topic_descr_t)]
    eswb_connect.restype = eswb_rv_t

# eswb/src/lib/include/public/eswb/api.h: 125
if _libs["eswb"].has("eswb_read", "cdecl"):
    eswb_read = _libs["eswb"].get("eswb_read", "cdecl")
    eswb_read.argtypes = [eswb_topic_descr_t, POINTER(None)]
    eswb_read.restype = eswb_rv_t

# eswb/src/lib/include/public/eswb/api.h: 133
if _libs["eswb"].has("eswb_get_update", "cdecl"):
    eswb_get_update = _libs["eswb"].get("eswb_get_update", "cdecl")
    eswb_get_update.argtypes = [eswb_topic_descr_t, POINTER(None)]
    eswb_get_update.restype = eswb_rv_t

# eswb/src/lib/include/public/eswb/api.h: 143
if _libs["eswb"].has("eswb_read_check_update", "cdecl"):
    eswb_read_check_update = _libs["eswb"].get("eswb_read_check_update", "cdecl")
    eswb_read_check_update.argtypes = [eswb_topic_descr_t, POINTER(None)]
    eswb_read_check_update.restype = eswb_rv_t

# eswb/src/lib/include/public/eswb/api.h: 151
if _libs["eswb"].has("eswb_get_topic_params", "cdecl"):
    eswb_get_topic_params = _libs["eswb"].get("eswb_get_topic_params", "cdecl")
    eswb_get_topic_params.argtypes = [eswb_topic_descr_t, POINTER(topic_params_t)]
    eswb_get_topic_params.restype = eswb_rv_t

# eswb/src/lib/include/public/eswb/api.h: 160
if _libs["eswb"].has("eswb_check_topic_type", "cdecl"):
    eswb_check_topic_type = _libs["eswb"].get("eswb_check_topic_type", "cdecl")
    eswb_check_topic_type.argtypes = [eswb_topic_descr_t, topic_data_type_t, eswb_size_t]
    eswb_check_topic_type.restype = eswb_rv_t

# eswb/src/lib/include/public/eswb/api.h: 174
if _libs["eswb"].has("eswb_get_next_topic_info", "cdecl"):
    eswb_get_next_topic_info = _libs["eswb"].get("eswb_get_next_topic_info", "cdecl")
    eswb_get_next_topic_info.argtypes = [eswb_topic_descr_t, POINTER(eswb_topic_id_t), POINTER(struct_topic_extract)]
    eswb_get_next_topic_info.restype = eswb_rv_t

# eswb/src/lib/include/public/eswb/api.h: 182
if _libs["eswb"].has("eswb_get_topic_path", "cdecl"):
    eswb_get_topic_path = _libs["eswb"].get("eswb_get_topic_path", "cdecl")
    eswb_get_topic_path.argtypes = [eswb_topic_descr_t, String]
    eswb_get_topic_path.restype = eswb_rv_t

# eswb/src/lib/include/public/eswb/api.h: 197
if _libs["eswb"].has("eswb_vector_write", "cdecl"):
    eswb_vector_write = _libs["eswb"].get("eswb_vector_write", "cdecl")
    eswb_vector_write.argtypes = [eswb_topic_descr_t, eswb_index_t, POINTER(None), eswb_index_t, uint32_t]
    eswb_vector_write.restype = eswb_rv_t

# eswb/src/lib/include/public/eswb/api.h: 208
if _libs["eswb"].has("eswb_vector_read", "cdecl"):
    eswb_vector_read = _libs["eswb"].get("eswb_vector_read", "cdecl")
    eswb_vector_read.argtypes = [eswb_topic_descr_t, eswb_index_t, POINTER(None), eswb_index_t, POINTER(eswb_index_t)]
    eswb_vector_read.restype = eswb_rv_t

# eswb/src/lib/include/public/eswb/api.h: 222
if _libs["eswb"].has("eswb_vector_read_check_update", "cdecl"):
    eswb_vector_read_check_update = _libs["eswb"].get("eswb_vector_read_check_update", "cdecl")
    eswb_vector_read_check_update.argtypes = [eswb_topic_descr_t, eswb_index_t, POINTER(None), eswb_index_t, POINTER(eswb_index_t)]
    eswb_vector_read_check_update.restype = eswb_rv_t

# eswb/src/lib/include/public/eswb/api.h: 233
if _libs["eswb"].has("eswb_vector_get_update", "cdecl"):
    eswb_vector_get_update = _libs["eswb"].get("eswb_vector_get_update", "cdecl")
    eswb_vector_get_update.argtypes = [eswb_topic_descr_t, eswb_index_t, POINTER(None), eswb_index_t, POINTER(eswb_index_t)]
    eswb_vector_get_update.restype = eswb_rv_t

# eswb/src/lib/include/public/eswb/api.h: 241
if _libs["eswb"].has("eswb_fifo_subscribe", "cdecl"):
    eswb_fifo_subscribe = _libs["eswb"].get("eswb_fifo_subscribe", "cdecl")
    eswb_fifo_subscribe.argtypes = [String, POINTER(eswb_topic_descr_t)]
    eswb_fifo_subscribe.restype = eswb_rv_t

# eswb/src/lib/include/public/eswb/api.h: 249
if _libs["eswb"].has("eswb_fifo_push", "cdecl"):
    eswb_fifo_push = _libs["eswb"].get("eswb_fifo_push", "cdecl")
    eswb_fifo_push.argtypes = [eswb_topic_descr_t, POINTER(None)]
    eswb_fifo_push.restype = eswb_rv_t

# eswb/src/lib/include/public/eswb/api.h: 259
if _libs["eswb"].has("eswb_fifo_pop", "cdecl"):
    eswb_fifo_pop = _libs["eswb"].get("eswb_fifo_pop", "cdecl")
    eswb_fifo_pop.argtypes = [eswb_topic_descr_t, POINTER(None)]
    eswb_fifo_pop.restype = eswb_rv_t

# eswb/src/lib/include/public/eswb/api.h: 266
if _libs["eswb"].has("eswb_fifo_flush", "cdecl"):
    eswb_fifo_flush = _libs["eswb"].get("eswb_fifo_flush", "cdecl")
    eswb_fifo_flush.argtypes = [eswb_topic_descr_t]
    eswb_fifo_flush.restype = eswb_rv_t

# eswb/src/lib/include/public/eswb/api.h: 278
if _libs["eswb"].has("eswb_fifo_try_pop", "cdecl"):
    eswb_fifo_try_pop = _libs["eswb"].get("eswb_fifo_try_pop", "cdecl")
    eswb_fifo_try_pop.argtypes = [eswb_topic_descr_t, POINTER(None)]
    eswb_fifo_try_pop.restype = eswb_rv_t

# eswb/src/lib/include/public/eswb/api.h: 287
if _libs["eswb"].has("eswb_arm_timeout", "cdecl"):
    eswb_arm_timeout = _libs["eswb"].get("eswb_arm_timeout", "cdecl")
    eswb_arm_timeout.argtypes = [eswb_topic_descr_t, uint32_t]
    eswb_arm_timeout.restype = eswb_rv_t

# eswb/src/lib/include/public/eswb/api.h: 296
if _libs["eswb"].has("eswb_connect_nested", "cdecl"):
    eswb_connect_nested = _libs["eswb"].get("eswb_connect_nested", "cdecl")
    eswb_connect_nested.argtypes = [eswb_topic_descr_t, String, POINTER(eswb_topic_descr_t)]
    eswb_connect_nested.restype = eswb_rv_t

# eswb/src/lib/include/public/eswb/api.h: 309
if _libs["eswb"].has("eswb_wait_connect_nested", "cdecl"):
    eswb_wait_connect_nested = _libs["eswb"].get("eswb_wait_connect_nested", "cdecl")
    eswb_wait_connect_nested.argtypes = [eswb_topic_descr_t, String, POINTER(eswb_topic_descr_t), uint32_t]
    eswb_wait_connect_nested.restype = eswb_rv_t

# eswb/src/lib/include/public/eswb/api.h: 316
if _libs["eswb"].has("eswb_disconnect", "cdecl"):
    eswb_disconnect = _libs["eswb"].get("eswb_disconnect", "cdecl")
    eswb_disconnect.argtypes = [eswb_topic_descr_t]
    eswb_disconnect.restype = eswb_rv_t

# eswb/src/lib/include/public/eswb/api.h: 323
if _libs["eswb"].has("eswb_get_bus_prefix", "cdecl"):
    eswb_get_bus_prefix = _libs["eswb"].get("eswb_get_bus_prefix", "cdecl")
    eswb_get_bus_prefix.argtypes = [eswb_type_t]
    eswb_get_bus_prefix.restype = c_char_p

# eswb/src/lib/include/public/eswb/api.h: 333
if _libs["eswb"].has("eswb_path_compose", "cdecl"):
    eswb_path_compose = _libs["eswb"].get("eswb_path_compose", "cdecl")
    eswb_path_compose.argtypes = [eswb_type_t, String, String, String]
    eswb_path_compose.restype = eswb_rv_t

# eswb/src/lib/include/public/eswb/api.h: 343
if _libs["eswb"].has("eswb_parse_path", "cdecl"):
    eswb_parse_path = _libs["eswb"].get("eswb_parse_path", "cdecl")
    eswb_parse_path.argtypes = [String, POINTER(eswb_type_t), String, String]
    eswb_parse_path.restype = eswb_rv_t

# eswb/src/lib/include/public/eswb/api.h: 352
if _libs["eswb"].has("eswb_path_split", "cdecl"):
    eswb_path_split = _libs["eswb"].get("eswb_path_split", "cdecl")
    eswb_path_split.argtypes = [String, String, String]
    eswb_path_split.restype = eswb_rv_t

# eswb/src/lib/include/public/eswb/api.h: 358
if _libs["eswb"].has("eswb_set_thread_name", "cdecl"):
    eswb_set_thread_name = _libs["eswb"].get("eswb_set_thread_name", "cdecl")
    eswb_set_thread_name.argtypes = [String]
    eswb_set_thread_name.restype = None

# eswb/src/lib/include/public/eswb/api.h: 364
if _libs["eswb"].has("eswb_set_delta_priority", "cdecl"):
    eswb_set_delta_priority = _libs["eswb"].get("eswb_set_delta_priority", "cdecl")
    eswb_set_delta_priority.argtypes = [c_int]
    eswb_set_delta_priority.restype = None

enum_anon_11 = c_int# eswb/src/lib/include/public/eswb/event_queue.h: 18

eqr_none = 0# eswb/src/lib/include/public/eswb/event_queue.h: 18

eqr_topic_proclaim = 1# eswb/src/lib/include/public/eswb/event_queue.h: 18

eqr_topic_update = 2# eswb/src/lib/include/public/eswb/event_queue.h: 18

eqr_fifo_push = 3# eswb/src/lib/include/public/eswb/event_queue.h: 18

event_queue_record_type_t = enum_anon_11# eswb/src/lib/include/public/eswb/event_queue.h: 18

event_queue_record_type_s_t = uint8_t# eswb/src/lib/include/public/eswb/event_queue.h: 20

# eswb/src/lib/include/public/eswb/event_queue.h: 28
class struct_anon_12(Structure):
    pass

struct_anon_12.__slots__ = [
    'size',
    'topic_id',
    'ch_mask',
    'type',
    'data',
]
struct_anon_12._fields_ = [
    ('size', eswb_size_t),
    ('topic_id', eswb_topic_id_t),
    ('ch_mask', eswb_event_queue_mask_t),
    ('type', event_queue_record_type_s_t),
    ('data', POINTER(None)),
]

event_queue_record_t = struct_anon_12# eswb/src/lib/include/public/eswb/event_queue.h: 28

# eswb/src/lib/include/public/eswb/event_queue.h: 35
class struct_event_queue_transfer(Structure):
    pass

struct_event_queue_transfer._pack_ = 1
struct_event_queue_transfer.__slots__ = [
    'size',
    'topic_id',
    'type',
]
struct_event_queue_transfer._fields_ = [
    ('size', uint32_t),
    ('topic_id', uint32_t),
    ('type', uint8_t),
]

event_queue_transfer_t = struct_event_queue_transfer# eswb/src/lib/include/public/eswb/event_queue.h: 35

# eswb/src/lib/include/public/eswb/event_queue.h: 42
class struct_anon_13(Structure):
    pass

struct_anon_13.__slots__ = [
    'subch_ind',
    'path_mask_2order',
]
struct_anon_13._fields_ = [
    ('subch_ind', eswb_index_t),
    ('path_mask_2order', c_char * int((100 + 1))),
]

eswb_ctl_evq_order_t = struct_anon_13# eswb/src/lib/include/public/eswb/event_queue.h: 42

# eswb/src/lib/include/public/eswb/event_queue.h: 50
if _libs["eswb"].has("eswb_event_queue_enable", "cdecl"):
    eswb_event_queue_enable = _libs["eswb"].get("eswb_event_queue_enable", "cdecl")
    eswb_event_queue_enable.argtypes = [eswb_topic_descr_t, eswb_size_t, eswb_size_t]
    eswb_event_queue_enable.restype = eswb_rv_t

# eswb/src/lib/include/public/eswb/event_queue.h: 51
if _libs["eswb"].has("eswb_event_queue_order_topic", "cdecl"):
    eswb_event_queue_order_topic = _libs["eswb"].get("eswb_event_queue_order_topic", "cdecl")
    eswb_event_queue_order_topic.argtypes = [eswb_topic_descr_t, String, eswb_index_t]
    eswb_event_queue_order_topic.restype = eswb_rv_t

# eswb/src/lib/include/public/eswb/event_queue.h: 53
if _libs["eswb"].has("eswb_event_queue_set_receive_mask", "cdecl"):
    eswb_event_queue_set_receive_mask = _libs["eswb"].get("eswb_event_queue_set_receive_mask", "cdecl")
    eswb_event_queue_set_receive_mask.argtypes = [eswb_topic_descr_t, eswb_event_queue_mask_t]
    eswb_event_queue_set_receive_mask.restype = eswb_rv_t

# eswb/src/lib/include/public/eswb/event_queue.h: 54
if _libs["eswb"].has("eswb_event_queue_subscribe", "cdecl"):
    eswb_event_queue_subscribe = _libs["eswb"].get("eswb_event_queue_subscribe", "cdecl")
    eswb_event_queue_subscribe.argtypes = [String, POINTER(eswb_topic_descr_t)]
    eswb_event_queue_subscribe.restype = eswb_rv_t

# eswb/src/lib/include/public/eswb/event_queue.h: 57
if _libs["eswb"].has("eswb_event_queue_pop", "cdecl"):
    eswb_event_queue_pop = _libs["eswb"].get("eswb_event_queue_pop", "cdecl")
    eswb_event_queue_pop.argtypes = [eswb_topic_descr_t, POINTER(event_queue_transfer_t)]
    eswb_event_queue_pop.restype = eswb_rv_t

# eswb/src/lib/include/public/eswb/event_queue.h: 59
class struct_topic_id_map(Structure):
    pass

# eswb/src/lib/include/public/eswb/event_queue.h: 61
if _libs["eswb"].has("eswb_event_queue_replicate", "cdecl"):
    eswb_event_queue_replicate = _libs["eswb"].get("eswb_event_queue_replicate", "cdecl")
    eswb_event_queue_replicate.argtypes = [eswb_topic_descr_t, POINTER(struct_topic_id_map), POINTER(event_queue_transfer_t)]
    eswb_event_queue_replicate.restype = eswb_rv_t

enum_anon_14 = c_int# eswb/src/lib/include/public/eswb/services/eqrb.h: 38

eqrb_rv_ok = 0# eswb/src/lib/include/public/eswb/services/eqrb.h: 38

eqrb_small_buf = (eqrb_rv_ok + 1)# eswb/src/lib/include/public/eswb/services/eqrb.h: 38

eqrb_inv_code = (eqrb_small_buf + 1)# eswb/src/lib/include/public/eswb/services/eqrb.h: 38

eqrb_rv_nomem = (eqrb_inv_code + 1)# eswb/src/lib/include/public/eswb/services/eqrb.h: 38

eqrb_notsup = (eqrb_rv_nomem + 1)# eswb/src/lib/include/public/eswb/services/eqrb.h: 38

eqrb_invarg = (eqrb_notsup + 1)# eswb/src/lib/include/public/eswb/services/eqrb.h: 38

eqrb_nomem = (eqrb_invarg + 1)# eswb/src/lib/include/public/eswb/services/eqrb.h: 38

eqrb_inv_size = (eqrb_nomem + 1)# eswb/src/lib/include/public/eswb/services/eqrb.h: 38

eqrb_eswb_err = (eqrb_inv_size + 1)# eswb/src/lib/include/public/eswb/services/eqrb.h: 38

eqrb_os_based_err = (eqrb_eswb_err + 1)# eswb/src/lib/include/public/eswb/services/eqrb.h: 38

eqrb_media_err = (eqrb_os_based_err + 1)# eswb/src/lib/include/public/eswb/services/eqrb.h: 38

eqrb_media_timedout = (eqrb_media_err + 1)# eswb/src/lib/include/public/eswb/services/eqrb.h: 38

eqrb_media_stop = (eqrb_media_timedout + 1)# eswb/src/lib/include/public/eswb/services/eqrb.h: 38

eqrb_media_invarg = (eqrb_media_stop + 1)# eswb/src/lib/include/public/eswb/services/eqrb.h: 38

eqrb_media_reset_cmd = (eqrb_media_invarg + 1)# eswb/src/lib/include/public/eswb/services/eqrb.h: 38

eqrb_media_remote_need_reset = (eqrb_media_reset_cmd + 1)# eswb/src/lib/include/public/eswb/services/eqrb.h: 38

eqrb_server_already_launched = (eqrb_media_remote_need_reset + 1)# eswb/src/lib/include/public/eswb/services/eqrb.h: 38

eqrb_rv_rx_eswb_fatal_err = (eqrb_server_already_launched + 1)# eswb/src/lib/include/public/eswb/services/eqrb.h: 38

eqrb_rv_t = enum_anon_14# eswb/src/lib/include/public/eswb/services/eqrb.h: 38

enum_anon_15 = c_int# eswb/src/lib/include/public/eswb/services/eqrb.h: 43

eqrb_cmd_reset_remote = 0# eswb/src/lib/include/public/eswb/services/eqrb.h: 43

eqrb_cmd_reset_local_state = (eqrb_cmd_reset_remote + 1)# eswb/src/lib/include/public/eswb/services/eqrb.h: 43

eqrb_cmd_t = enum_anon_15# eswb/src/lib/include/public/eswb/services/eqrb.h: 43

# eswb/src/lib/include/public/eswb/services/eqrb.h: 47
if _libs["eswb"].has("eqrb_sdtl_server_start", "cdecl"):
    eqrb_sdtl_server_start = _libs["eswb"].get("eqrb_sdtl_server_start", "cdecl")
    eqrb_sdtl_server_start.argtypes = [String, String, String, String, uint32_t, String, POINTER(POINTER(c_char))]
    eqrb_sdtl_server_start.restype = eqrb_rv_t

# eswb/src/lib/include/public/eswb/services/eqrb.h: 54
if _libs["eswb"].has("eqrb_sdtl_client_connect", "cdecl"):
    eqrb_sdtl_client_connect = _libs["eswb"].get("eqrb_sdtl_client_connect", "cdecl")
    eqrb_sdtl_client_connect.argtypes = [String, String, String, String, uint32_t, c_int]
    eqrb_sdtl_client_connect.restype = eqrb_rv_t

# eswb/src/lib/include/public/eswb/services/eqrb.h: 61
for _lib in _libs.values():
    if not _lib.has("eqrb_file_server_start", "cdecl"):
        continue
    eqrb_file_server_start = _lib.get("eqrb_file_server_start", "cdecl")
    eqrb_file_server_start.argtypes = [String, String, String, String, POINTER(POINTER(c_char))]
    eqrb_file_server_start.restype = eqrb_rv_t
    break

# eswb/src/lib/include/public/eswb/services/eqrb.h: 66
for _lib in _libs.values():
    if not _lib.has("eqrb_file_client_connect", "cdecl"):
        continue
    eqrb_file_client_connect = _lib.get("eqrb_file_client_connect", "cdecl")
    eqrb_file_client_connect.argtypes = [String, String, String, String, uint32_t, POINTER(POINTER(c_char))]
    eqrb_file_client_connect.restype = eqrb_rv_t
    break

# eswb/src/lib/include/public/eswb/services/eqrb.h: 72
if _libs["eswb"].has("eqrb_strerror", "cdecl"):
    eqrb_strerror = _libs["eswb"].get("eqrb_strerror", "cdecl")
    eqrb_strerror.argtypes = [eqrb_rv_t]
    eqrb_strerror.restype = c_char_p

enum_sdtl_rv = c_int# eswb/src/lib/include/public/eswb/services/sdtl.h: 44

SDTL_OK = 0# eswb/src/lib/include/public/eswb/services/sdtl.h: 44

SDTL_TIMEDOUT = (SDTL_OK + 1)# eswb/src/lib/include/public/eswb/services/sdtl.h: 44

SDTL_OK_FIRST_PACKET = (SDTL_TIMEDOUT + 1)# eswb/src/lib/include/public/eswb/services/sdtl.h: 44

SDTL_OK_OMIT = (SDTL_OK_FIRST_PACKET + 1)# eswb/src/lib/include/public/eswb/services/sdtl.h: 44

SDTL_OK_REPEATED = (SDTL_OK_OMIT + 1)# eswb/src/lib/include/public/eswb/services/sdtl.h: 44

SDTL_OK_MISSED_PKT_IN_SEQ = (SDTL_OK_REPEATED + 1)# eswb/src/lib/include/public/eswb/services/sdtl.h: 44

SDTL_REMOTE_RX_CANCELED = (SDTL_OK_MISSED_PKT_IN_SEQ + 1)# eswb/src/lib/include/public/eswb/services/sdtl.h: 44

SDTL_REMOTE_RX_NO_CLIENT = (SDTL_REMOTE_RX_CANCELED + 1)# eswb/src/lib/include/public/eswb/services/sdtl.h: 44

SDTL_RX_BUF_SMALL = (SDTL_REMOTE_RX_NO_CLIENT + 1)# eswb/src/lib/include/public/eswb/services/sdtl.h: 44

SDTL_TX_BUF_SMALL = (SDTL_RX_BUF_SMALL + 1)# eswb/src/lib/include/public/eswb/services/sdtl.h: 44

SDTL_NON_CONSIST_FRM_LEN = (SDTL_TX_BUF_SMALL + 1)# eswb/src/lib/include/public/eswb/services/sdtl.h: 44

SDTL_INVALID_FRAME_TYPE = (SDTL_NON_CONSIST_FRM_LEN + 1)# eswb/src/lib/include/public/eswb/services/sdtl.h: 44

SDTL_NO_CHANNEL_REMOTE = (SDTL_INVALID_FRAME_TYPE + 1)# eswb/src/lib/include/public/eswb/services/sdtl.h: 44

SDTL_NO_CHANNEL_LOCAL = (SDTL_NO_CHANNEL_REMOTE + 1)# eswb/src/lib/include/public/eswb/services/sdtl.h: 44

SDTL_ESWB_ERR = (SDTL_NO_CHANNEL_LOCAL + 1)# eswb/src/lib/include/public/eswb/services/sdtl.h: 44

SDTL_RX_FIFO_OVERFLOW = (SDTL_ESWB_ERR + 1)# eswb/src/lib/include/public/eswb/services/sdtl.h: 44

SDTL_NO_MEM = (SDTL_RX_FIFO_OVERFLOW + 1)# eswb/src/lib/include/public/eswb/services/sdtl.h: 44

SDTL_CH_EXIST = (SDTL_NO_MEM + 1)# eswb/src/lib/include/public/eswb/services/sdtl.h: 44

SDTL_SERVICE_EXIST = (SDTL_CH_EXIST + 1)# eswb/src/lib/include/public/eswb/services/sdtl.h: 44

SDTL_NO_SERVICE = (SDTL_SERVICE_EXIST + 1)# eswb/src/lib/include/public/eswb/services/sdtl.h: 44

SDTL_INVALID_MTU = (SDTL_NO_SERVICE + 1)# eswb/src/lib/include/public/eswb/services/sdtl.h: 44

SDTL_INVALID_MEDIA = (SDTL_INVALID_MTU + 1)# eswb/src/lib/include/public/eswb/services/sdtl.h: 44

SDTL_NAMES_TOO_LONG = (SDTL_INVALID_MEDIA + 1)# eswb/src/lib/include/public/eswb/services/sdtl.h: 44

SDTL_SYS_ERR = (SDTL_NAMES_TOO_LONG + 1)# eswb/src/lib/include/public/eswb/services/sdtl.h: 44

SDTL_INVALID_CH_TYPE = (SDTL_SYS_ERR + 1)# eswb/src/lib/include/public/eswb/services/sdtl.h: 44

SDTL_MEDIA_NO_ENTITY = (SDTL_INVALID_CH_TYPE + 1)# eswb/src/lib/include/public/eswb/services/sdtl.h: 44

SDTL_MEDIA_NOT_SUPPORTED = (SDTL_MEDIA_NO_ENTITY + 1)# eswb/src/lib/include/public/eswb/services/sdtl.h: 44

SDTL_MEDIA_ERR = (SDTL_MEDIA_NOT_SUPPORTED + 1)# eswb/src/lib/include/public/eswb/services/sdtl.h: 44

SDTL_MEDIA_EOF = (SDTL_MEDIA_ERR + 1)# eswb/src/lib/include/public/eswb/services/sdtl.h: 44

SDTL_APP_CANCEL = (SDTL_MEDIA_EOF + 1)# eswb/src/lib/include/public/eswb/services/sdtl.h: 44

SDTL_APP_RESET = (SDTL_APP_CANCEL + 1)# eswb/src/lib/include/public/eswb/services/sdtl.h: 44

sdtl_rv_t = enum_sdtl_rv# eswb/src/lib/include/public/eswb/services/sdtl.h: 44

enum_sdtl_channel_type = c_int# eswb/src/lib/include/public/eswb/services/sdtl.h: 49

SDTL_CHANNEL_UNRELIABLE = 0# eswb/src/lib/include/public/eswb/services/sdtl.h: 49

SDTL_CHANNEL_RELIABLE = 1# eswb/src/lib/include/public/eswb/services/sdtl.h: 49

sdtl_channel_type_t = enum_sdtl_channel_type# eswb/src/lib/include/public/eswb/services/sdtl.h: 49

media_open_t = CFUNCTYPE(UNCHECKED(sdtl_rv_t), String, POINTER(None), POINTER(POINTER(None)))# eswb/src/lib/include/public/eswb/services/sdtl.h: 52

media_close_t = CFUNCTYPE(UNCHECKED(sdtl_rv_t), POINTER(None))# eswb/src/lib/include/public/eswb/services/sdtl.h: 53

media_read_t = CFUNCTYPE(UNCHECKED(sdtl_rv_t), POINTER(None), POINTER(None), c_size_t, POINTER(c_size_t))# eswb/src/lib/include/public/eswb/services/sdtl.h: 54

media_write_t = CFUNCTYPE(UNCHECKED(sdtl_rv_t), POINTER(None), POINTER(None), c_size_t)# eswb/src/lib/include/public/eswb/services/sdtl.h: 55

# eswb/src/lib/include/public/eswb/services/sdtl.h: 62
class struct_sdtl_service_media(Structure):
    pass

struct_sdtl_service_media.__slots__ = [
    'open',
    'read',
    'write',
    'close',
]
struct_sdtl_service_media._fields_ = [
    ('open', media_open_t),
    ('read', media_read_t),
    ('write', media_write_t),
    ('close', media_close_t),
]

sdtl_service_media_t = struct_sdtl_service_media# eswb/src/lib/include/public/eswb/services/sdtl.h: 62

# eswb/src/lib/include/public/eswb/services/sdtl.h: 72
class struct_sdtl_channel_cfg(Structure):
    pass

struct_sdtl_channel_cfg.__slots__ = [
    'name',
    'id',
    'type',
    'mtu_override',
]
struct_sdtl_channel_cfg._fields_ = [
    ('name', String),
    ('id', uint8_t),
    ('type', sdtl_channel_type_t),
    ('mtu_override', uint32_t),
]

sdtl_channel_cfg_t = struct_sdtl_channel_cfg# eswb/src/lib/include/public/eswb/services/sdtl.h: 72

# eswb/src/lib/include/public/eswb/services/sdtl.h: 75
class struct_sdtl_service(Structure):
    pass

sdtl_service_t = struct_sdtl_service# eswb/src/lib/include/public/eswb/services/sdtl.h: 75

# eswb/src/lib/include/public/eswb/services/sdtl.h: 78
class struct_sdtl_channel_handle(Structure):
    pass

sdtl_channel_handle_t = struct_sdtl_channel_handle# eswb/src/lib/include/public/eswb/services/sdtl.h: 78

# eswb/src/lib/include/public/eswb/services/sdtl.h: 85
class struct_sdtl_media_serial_params(Structure):
    pass

struct_sdtl_media_serial_params.__slots__ = [
    'baudrate',
]
struct_sdtl_media_serial_params._fields_ = [
    ('baudrate', uint32_t),
]

sdtl_media_serial_params_t = struct_sdtl_media_serial_params# eswb/src/lib/include/public/eswb/services/sdtl.h: 85

# eswb/src/lib/include/public/eswb/services/sdtl.h: 87
try:
    sdtl_media_serial = (sdtl_service_media_t).in_dll(_libs["eswb"], "sdtl_media_serial")
except:
    pass

# eswb/src/lib/include/public/eswb/services/sdtl.h: 98
if _libs["eswb"].has("sdtl_service_init", "cdecl"):
    sdtl_service_init = _libs["eswb"].get("sdtl_service_init", "cdecl")
    sdtl_service_init.argtypes = [POINTER(POINTER(sdtl_service_t)), String, String, c_size_t, c_size_t, POINTER(sdtl_service_media_t)]
    sdtl_service_init.restype = sdtl_rv_t

# eswb/src/lib/include/public/eswb/services/sdtl.h: 100
if _libs["eswb"].has("sdtl_service_init_w", "cdecl"):
    sdtl_service_init_w = _libs["eswb"].get("sdtl_service_init_w", "cdecl")
    sdtl_service_init_w.argtypes = [POINTER(POINTER(sdtl_service_t)), String, String, c_size_t, c_size_t, String]
    sdtl_service_init_w.restype = sdtl_rv_t

# eswb/src/lib/include/public/eswb/services/sdtl.h: 103
if _libs["eswb"].has("sdtl_service_lookup", "cdecl"):
    sdtl_service_lookup = _libs["eswb"].get("sdtl_service_lookup", "cdecl")
    sdtl_service_lookup.argtypes = [String]
    sdtl_service_lookup.restype = POINTER(sdtl_service_t)

# eswb/src/lib/include/public/eswb/services/sdtl.h: 104
if _libs["eswb"].has("sdtl_service_start", "cdecl"):
    sdtl_service_start = _libs["eswb"].get("sdtl_service_start", "cdecl")
    sdtl_service_start.argtypes = [POINTER(sdtl_service_t), String, POINTER(None)]
    sdtl_service_start.restype = sdtl_rv_t

# eswb/src/lib/include/public/eswb/services/sdtl.h: 105
if _libs["eswb"].has("sdtl_service_stop", "cdecl"):
    sdtl_service_stop = _libs["eswb"].get("sdtl_service_stop", "cdecl")
    sdtl_service_stop.argtypes = [POINTER(sdtl_service_t)]
    sdtl_service_stop.restype = sdtl_rv_t

# eswb/src/lib/include/public/eswb/services/sdtl.h: 107
if _libs["eswb"].has("sdtl_channel_create", "cdecl"):
    sdtl_channel_create = _libs["eswb"].get("sdtl_channel_create", "cdecl")
    sdtl_channel_create.argtypes = [POINTER(sdtl_service_t), POINTER(sdtl_channel_cfg_t)]
    sdtl_channel_create.restype = sdtl_rv_t

# eswb/src/lib/include/public/eswb/services/sdtl.h: 109
if _libs["eswb"].has("sdtl_channel_open", "cdecl"):
    sdtl_channel_open = _libs["eswb"].get("sdtl_channel_open", "cdecl")
    sdtl_channel_open.argtypes = [POINTER(sdtl_service_t), String, POINTER(POINTER(sdtl_channel_handle_t))]
    sdtl_channel_open.restype = sdtl_rv_t

# eswb/src/lib/include/public/eswb/services/sdtl.h: 110
if _libs["eswb"].has("sdtl_channel_close", "cdecl"):
    sdtl_channel_close = _libs["eswb"].get("sdtl_channel_close", "cdecl")
    sdtl_channel_close.argtypes = [POINTER(sdtl_channel_handle_t)]
    sdtl_channel_close.restype = sdtl_rv_t

# eswb/src/lib/include/public/eswb/services/sdtl.h: 112
if _libs["eswb"].has("sdtl_channel_recv_arm_timeout", "cdecl"):
    sdtl_channel_recv_arm_timeout = _libs["eswb"].get("sdtl_channel_recv_arm_timeout", "cdecl")
    sdtl_channel_recv_arm_timeout.argtypes = [POINTER(sdtl_channel_handle_t), uint32_t]
    sdtl_channel_recv_arm_timeout.restype = sdtl_rv_t

# eswb/src/lib/include/public/eswb/services/sdtl.h: 114
if _libs["eswb"].has("sdtl_channel_recv_data", "cdecl"):
    sdtl_channel_recv_data = _libs["eswb"].get("sdtl_channel_recv_data", "cdecl")
    sdtl_channel_recv_data.argtypes = [POINTER(sdtl_channel_handle_t), POINTER(None), uint32_t, POINTER(c_size_t)]
    sdtl_channel_recv_data.restype = sdtl_rv_t

# eswb/src/lib/include/public/eswb/services/sdtl.h: 115
if _libs["eswb"].has("sdtl_channel_send_data", "cdecl"):
    sdtl_channel_send_data = _libs["eswb"].get("sdtl_channel_send_data", "cdecl")
    sdtl_channel_send_data.argtypes = [POINTER(sdtl_channel_handle_t), POINTER(None), uint32_t]
    sdtl_channel_send_data.restype = sdtl_rv_t

# eswb/src/lib/include/public/eswb/services/sdtl.h: 120
if _libs["eswb"].has("sdtl_channel_send_cmd", "cdecl"):
    sdtl_channel_send_cmd = _libs["eswb"].get("sdtl_channel_send_cmd", "cdecl")
    sdtl_channel_send_cmd.argtypes = [POINTER(sdtl_channel_handle_t), uint8_t]
    sdtl_channel_send_cmd.restype = sdtl_rv_t

# eswb/src/lib/include/public/eswb/services/sdtl.h: 121
if _libs["eswb"].has("sdtl_channel_reset_condition", "cdecl"):
    sdtl_channel_reset_condition = _libs["eswb"].get("sdtl_channel_reset_condition", "cdecl")
    sdtl_channel_reset_condition.argtypes = [POINTER(sdtl_channel_handle_t)]
    sdtl_channel_reset_condition.restype = sdtl_rv_t

# eswb/src/lib/include/public/eswb/services/sdtl.h: 122
if _libs["eswb"].has("sdtl_channel_check_reset_condition", "cdecl"):
    sdtl_channel_check_reset_condition = _libs["eswb"].get("sdtl_channel_check_reset_condition", "cdecl")
    sdtl_channel_check_reset_condition.argtypes = [POINTER(sdtl_channel_handle_t)]
    sdtl_channel_check_reset_condition.restype = sdtl_rv_t

# eswb/src/lib/include/public/eswb/services/sdtl.h: 124
if _libs["eswb"].has("sdtl_channel_get_max_payload_size", "cdecl"):
    sdtl_channel_get_max_payload_size = _libs["eswb"].get("sdtl_channel_get_max_payload_size", "cdecl")
    sdtl_channel_get_max_payload_size.argtypes = [POINTER(sdtl_channel_handle_t)]
    sdtl_channel_get_max_payload_size.restype = uint32_t

# eswb/src/lib/include/public/eswb/services/sdtl.h: 125
if _libs["eswb"].has("sdtl_lookup_media", "cdecl"):
    sdtl_lookup_media = _libs["eswb"].get("sdtl_lookup_media", "cdecl")
    sdtl_lookup_media.argtypes = [String]
    sdtl_lookup_media.restype = POINTER(sdtl_service_media_t)

# eswb/src/lib/include/public/eswb/services/sdtl.h: 127
if _libs["eswb"].has("sdtl_strerror", "cdecl"):
    sdtl_strerror = _libs["eswb"].get("sdtl_strerror", "cdecl")
    sdtl_strerror.argtypes = [sdtl_rv_t]
    sdtl_strerror.restype = c_char_p

u_long = c_ulong# /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/sys/types.h: 88

ushort = c_ushort# /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/sys/types.h: 91

uint = c_uint# /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/sys/types.h: 92

u_quad_t = u_int64_t# /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/sys/types.h: 95

quad_t = c_int64# /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/sys/types.h: 96

qaddr_t = POINTER(quad_t)# /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/sys/types.h: 97

daddr_t = c_int32# /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/sys/types.h: 101

dev_t = __darwin_dev_t# /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/sys/_types/_dev_t.h: 31

fixpt_t = u_int32_t# /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/sys/types.h: 105

segsz_t = c_int32# /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/sys/types.h: 125

swblk_t = c_int32# /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/sys/types.h: 126

fd_mask = c_int32# /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/sys/types.h: 189

# eswb/src/lib/include/sync.h: 7
class struct_sync_handle(Structure):
    pass

# eswb/src/lib/include/registry.h: 8
class struct_registry(Structure):
    pass

# eswb/src/lib/include/topic_mem.h: 19
class struct_topic_array_state(Structure):
    pass

struct_topic_array_state.__slots__ = [
    'head',
    'lap_num',
]
struct_topic_array_state._fields_ = [
    ('head', eswb_fifo_index_t),
    ('lap_num', eswb_fifo_index_t),
]

topic_array_state_t = struct_topic_array_state# eswb/src/lib/include/topic_mem.h: 19

# eswb/src/lib/include/topic_mem.h: 31
class struct_fifo_ext(Structure):
    pass

struct_fifo_ext.__slots__ = [
    'len',
    'elem_step',
    'elem_size',
    'state',
    'curr_len',
]
struct_fifo_ext._fields_ = [
    ('len', eswb_size_t),
    ('elem_step', eswb_size_t),
    ('elem_size', eswb_size_t),
    ('state', topic_array_state_t),
    ('curr_len', eswb_size_t),
]

array_ext_t = struct_fifo_ext# eswb/src/lib/include/topic_mem.h: 31

# eswb/src/lib/include/topic_mem.h: 37
class struct_topic(Structure):
    pass

struct_topic.__slots__ = [
    'name',
    'type',
    'data_size',
    'data',
    'array_ext',
    'sync',
    'update_counter',
    'flags',
    'parent',
    'first_child',
    'next_sibling',
    'reg_ref',
    'id',
    'evq_mask',
]
struct_topic._fields_ = [
    ('name', c_char * int((30 + 1))),
    ('type', topic_data_type_t),
    ('data_size', eswb_size_t),
    ('data', POINTER(None)),
    ('array_ext', POINTER(array_ext_t)),
    ('sync', POINTER(struct_sync_handle)),
    ('update_counter', eswb_update_counter_t),
    ('flags', uint32_t),
    ('parent', POINTER(struct_topic)),
    ('first_child', POINTER(struct_topic)),
    ('next_sibling', POINTER(struct_topic)),
    ('reg_ref', POINTER(struct_registry)),
    ('id', eswb_topic_id_t),
    ('evq_mask', eswb_event_queue_mask_t),
]

topic_t = struct_topic# eswb/src/lib/include/topic_mem.h: 62

# eswb/src/lib/include/topic_mem.h: 71
if _libs["eswb"].has("topic_mem_write", "cdecl"):
    topic_mem_write = _libs["eswb"].get("topic_mem_write", "cdecl")
    topic_mem_write.argtypes = [POINTER(topic_t), POINTER(None)]
    topic_mem_write.restype = eswb_rv_t

# eswb/src/lib/include/topic_mem.h: 72
if _libs["eswb"].has("topic_mem_simply_copy", "cdecl"):
    topic_mem_simply_copy = _libs["eswb"].get("topic_mem_simply_copy", "cdecl")
    topic_mem_simply_copy.argtypes = [POINTER(topic_t), POINTER(None)]
    topic_mem_simply_copy.restype = eswb_rv_t

# eswb/src/lib/include/topic_mem.h: 73
if _libs["eswb"].has("topic_mem_read_vector", "cdecl"):
    topic_mem_read_vector = _libs["eswb"].get("topic_mem_read_vector", "cdecl")
    topic_mem_read_vector.argtypes = [POINTER(topic_t), POINTER(None), eswb_index_t, eswb_index_t, POINTER(eswb_index_t)]
    topic_mem_read_vector.restype = eswb_rv_t

# eswb/src/lib/include/topic_mem.h: 74
if _libs["eswb"].has("topic_mem_write_fifo", "cdecl"):
    topic_mem_write_fifo = _libs["eswb"].get("topic_mem_write_fifo", "cdecl")
    topic_mem_write_fifo.argtypes = [POINTER(topic_t), POINTER(None)]
    topic_mem_write_fifo.restype = eswb_rv_t

# eswb/src/lib/include/topic_mem.h: 75
if _libs["eswb"].has("topic_mem_write_vector", "cdecl"):
    topic_mem_write_vector = _libs["eswb"].get("topic_mem_write_vector", "cdecl")
    topic_mem_write_vector.argtypes = [POINTER(topic_t), POINTER(None), POINTER(array_alter_t)]
    topic_mem_write_vector.restype = eswb_rv_t

# eswb/src/lib/include/topic_mem.h: 76
if _libs["eswb"].has("topic_mem_read_fifo", "cdecl"):
    topic_mem_read_fifo = _libs["eswb"].get("topic_mem_read_fifo", "cdecl")
    topic_mem_read_fifo.argtypes = [POINTER(topic_t), eswb_index_t, POINTER(None)]
    topic_mem_read_fifo.restype = None

# eswb/src/lib/include/topic_mem.h: 77
if _libs["eswb"].has("topic_mem_get_params", "cdecl"):
    topic_mem_get_params = _libs["eswb"].get("topic_mem_get_params", "cdecl")
    topic_mem_get_params.argtypes = [POINTER(topic_t), POINTER(topic_params_t)]
    topic_mem_get_params.restype = eswb_rv_t

crawling_lambda_t = CFUNCTYPE(UNCHECKED(eswb_rv_t), POINTER(None), POINTER(topic_t))# eswb/src/lib/include/topic_mem.h: 79

# eswb/src/lib/include/topic_mem.h: 81
if _libs["eswb"].has("topic_mem_walk_through", "cdecl"):
    topic_mem_walk_through = _libs["eswb"].get("topic_mem_walk_through", "cdecl")
    topic_mem_walk_through.argtypes = [POINTER(topic_t), String, crawling_lambda_t, POINTER(None)]
    topic_mem_walk_through.restype = eswb_rv_t

# eswb/src/lib/include/topic_mem.h: 83
if _libs["eswb"].has("topic_mem_event_queue_write", "cdecl"):
    topic_mem_event_queue_write = _libs["eswb"].get("topic_mem_event_queue_write", "cdecl")
    topic_mem_event_queue_write.argtypes = [POINTER(topic_t), POINTER(event_queue_record_t)]
    topic_mem_event_queue_write.restype = eswb_rv_t

# eswb/src/lib/include/topic_mem.h: 85
if _libs["eswb"].has("topic_mem_event_queue_get_data", "cdecl"):
    topic_mem_event_queue_get_data = _libs["eswb"].get("topic_mem_event_queue_get_data", "cdecl")
    topic_mem_event_queue_get_data.argtypes = [POINTER(topic_t), POINTER(event_queue_record_t), POINTER(None)]
    topic_mem_event_queue_get_data.restype = eswb_rv_t

# eswb/src/lib/include/topic_mem.h: 86
for _lib in _libs.values():
    if not _lib.has("topic_event_queue_read", "cdecl"):
        continue
    topic_event_queue_read = _lib.get("topic_event_queue_read", "cdecl")
    topic_event_queue_read.argtypes = [POINTER(topic_t), eswb_index_t, POINTER(event_queue_record_t)]
    topic_event_queue_read.restype = None
    break

struct_registry.__slots__ = [
    'sync',
    'max_topics',
    'topics_num',
    'topics',
]
struct_registry._fields_ = [
    ('sync', POINTER(struct_sync_handle)),
    ('max_topics', eswb_size_t),
    ('topics_num', eswb_index_t),
    ('topics', topic_t * int(0)),
]

registry_t = struct_registry# eswb/src/lib/include/registry.h: 15

# eswb/src/lib/include/registry.h: 19
if _libs["eswb"].has("reg_create", "cdecl"):
    reg_create = _libs["eswb"].get("reg_create", "cdecl")
    reg_create.argtypes = [String, POINTER(POINTER(registry_t)), eswb_size_t, c_int]
    reg_create.restype = eswb_rv_t

# eswb/src/lib/include/registry.h: 20
if _libs["eswb"].has("reg_destroy", "cdecl"):
    reg_destroy = _libs["eswb"].get("reg_destroy", "cdecl")
    reg_destroy.argtypes = [POINTER(registry_t)]
    reg_destroy.restype = eswb_rv_t

# eswb/src/lib/include/registry.h: 22
if _libs["eswb"].has("reg_tree_register", "cdecl"):
    reg_tree_register = _libs["eswb"].get("reg_tree_register", "cdecl")
    reg_tree_register.argtypes = [POINTER(registry_t), POINTER(topic_t), POINTER(topic_proclaiming_tree_t)]
    reg_tree_register.restype = eswb_rv_t

# eswb/src/lib/include/registry.h: 23
if _libs["eswb"].has("reg_find_topic", "cdecl"):
    reg_find_topic = _libs["eswb"].get("reg_find_topic", "cdecl")
    reg_find_topic.argtypes = [POINTER(registry_t), String]
    reg_find_topic.restype = POINTER(topic_t)

# eswb/src/lib/include/registry.h: 24
if _libs["eswb"].has("reg_get_next_topic_info", "cdecl"):
    reg_get_next_topic_info = _libs["eswb"].get("reg_get_next_topic_info", "cdecl")
    reg_get_next_topic_info.argtypes = [POINTER(registry_t), POINTER(topic_t), eswb_topic_id_t, POINTER(topic_extract_t)]
    reg_get_next_topic_info.restype = eswb_rv_t

# eswb/src/lib/include/registry.h: 26
if _libs["eswb"].has("reg_print", "cdecl"):
    reg_print = _libs["eswb"].get("reg_print", "cdecl")
    reg_print.argtypes = [POINTER(registry_t)]
    reg_print.restype = None

# /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/arm/types.h: 94
try:
    USER_ADDR_NULL = (user_addr_t (ord_if_char(0))).value
except:
    pass

# /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/arm/types.h: 95
def CAST_USER_ADDR_T(a_ptr):
    return (user_addr_t (ord_if_char((uintptr_t (ord_if_char(a_ptr))).value))).value

# /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/stdint.h: 108
try:
    UINT16_MAX = 65535
except:
    pass

# /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/stdint.h: 109
try:
    UINT32_MAX = 4294967295
except:
    pass

# /usr/local/include/eswb/types.h: 13
try:
    ESWB_FIFO_INDEX_OVERFLOW = UINT16_MAX
except:
    pass

# /usr/local/include/eswb/types.h: 26
try:
    ESWB_UPDATE_COUNTER_MAX = UINT32_MAX
except:
    pass

# /usr/local/include/eswb/types.h: 89
try:
    ESWB_BUS_NAME_MAX_LEN = 32
except:
    pass

# /usr/local/include/eswb/types.h: 90
try:
    ESWB_TOPIC_NAME_MAX_LEN = 30
except:
    pass

# /usr/local/include/eswb/types.h: 91
try:
    ESWB_TOPIC_MAX_PATH_LEN = 100
except:
    pass

# eswb/src/lib/include/public/eswb/api.h: 185
try:
    ESWB_VECTOR_WRITE_OPT_FLAG_DEFINE_END = (1 << 0)
except:
    pass

# eswb/src/lib/include/public/eswb/event_queue.h: 37
def EVENT_QUEUE_TRANSFER_DATA(__etp):
    return cast((cast(__etp, POINTER(uint8_t)) + sizeof(event_queue_transfer_t)), POINTER(uint8_t))

# eswb/src/lib/include/public/eswb/event_queue.h: 44
try:
    BUS_EVENT_QUEUE_NAME = '.bus_events_queue'
except:
    pass

# eswb/src/lib/include/public/eswb/services/eqrb.h: 45
try:
    EQRB_ERR_MSG_MAX_LEN = 128
except:
    pass

# eswb/src/lib/include/public/eswb/services/sdtl.h: 117
try:
    SDTL_PKT_CMD_CODE_CANCEL = 0x10
except:
    pass

# eswb/src/lib/include/public/eswb/services/sdtl.h: 118
try:
    SDTL_PKT_CMD_CODE_RESET = 0x11
except:
    pass

# /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/sys/types.h: 158
def major(x):
    return (c_int32 (ord_if_char((((u_int32_t (ord_if_char(x))).value >> 24) & 0xff)))).value

# /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/sys/types.h: 159
def minor(x):
    return (c_int32 (ord_if_char((x & 0xffffff)))).value

# /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/sys/types.h: 160
def makedev(x, y):
    return (dev_t (ord_if_char(((x << 24) | y)))).value

# /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/sys/_types/_fd_def.h: 45
try:
    __DARWIN_NBBY = 8
except:
    pass

# /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/sys/_types/_fd_def.h: 46
try:
    __DARWIN_NFDBITS = (sizeof(c_int32) * __DARWIN_NBBY)
except:
    pass

# /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/sys/_types/_fd_def.h: 47
def __DARWIN_howmany(x, y):
    return ((x % y) == 0) and (x / y) or ((x / y) + 1)

# /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/sys/types.h: 186
try:
    NBBY = __DARWIN_NBBY
except:
    pass

# /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/sys/types.h: 187
try:
    NFDBITS = __DARWIN_NFDBITS
except:
    pass

# /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/sys/types.h: 188
def howmany(x, y):
    return (__DARWIN_howmany (x, y))

# eswb/src/lib/include/topic_mem.h: 33
try:
    TOPIC_FLAGS_MAPPED_TO_PARENT = (1 << 0)
except:
    pass

# eswb/src/lib/include/topic_mem.h: 34
try:
    TOPIC_FLAGS_USES_PARENT_SYNC = (1 << 1)
except:
    pass

# eswb/src/lib/include/topic_mem.h: 35
try:
    TOPIC_FLAGS_INITED = (1 << 2)
except:
    pass

event_queue_transfer = struct_event_queue_transfer# eswb/src/lib/include/public/eswb/event_queue.h: 35

topic_id_map = struct_topic_id_map# eswb/src/lib/include/public/eswb/event_queue.h: 59

sdtl_service_media = struct_sdtl_service_media# eswb/src/lib/include/public/eswb/services/sdtl.h: 62

sdtl_channel_cfg = struct_sdtl_channel_cfg# eswb/src/lib/include/public/eswb/services/sdtl.h: 72

sdtl_service = struct_sdtl_service# eswb/src/lib/include/public/eswb/services/sdtl.h: 75

sdtl_channel_handle = struct_sdtl_channel_handle# eswb/src/lib/include/public/eswb/services/sdtl.h: 78

sdtl_media_serial_params = struct_sdtl_media_serial_params# eswb/src/lib/include/public/eswb/services/sdtl.h: 85

registry = struct_registry# eswb/src/lib/include/registry.h: 8

topic_array_state = struct_topic_array_state# eswb/src/lib/include/topic_mem.h: 19

fifo_ext = struct_fifo_ext# eswb/src/lib/include/topic_mem.h: 31

topic = struct_topic# eswb/src/lib/include/topic_mem.h: 37

# No inserted files

# No prefix-stripping

