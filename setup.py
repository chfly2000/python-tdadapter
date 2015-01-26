from distutils.core import setup, Extension

clivii = Extension("clivii",
                    define_macros = [("MAJOR_VERSION", "1"),
                                     ("MINOR_VERSION", "0")],
                    include_dirs = ["C:/Program Files/Teradata/Client/13.10/CLIv2/inc"],
                    libraries = ["wincli32"],
                    library_dirs = ["C:/Program Files/Teradata/Client/13.10/CLIv2/lib"],
                    sources = ["clivii.c", "cliviipy.c"])

setup(name = "clivii",
       version = "1.0",
       license = "PSF",
       description = "Python for Access Teradata Database.",
       author = "Felix",
       author_email = "chfly2000@google.com",
       url = "https://github.com/chfly2000/tdcliviipy",
       long_description = """Simple Python extensions for access Teradata Database
by the Teradata Call-Level Interface Version 2.""",
       ext_modules = [clivii])