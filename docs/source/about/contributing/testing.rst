
*********************
Testing and reporting
*********************

Testing
=======

Testing is what helps making applications rock stable.
Since we are a community project, we rely on you, yes — you, our dear users.

We don't really encourage you to compile the most unstable cutting edge source code,
but in case you find some bugs in the latest released version, do not hesitate to point them out to us.


Crashes
-------

So you found a reproducible way to crash Luminance HDR. If you are on Linux,
please use application called ``gdb`` to create a report which in programmers lingo is called a *backtrace*.
Here is how you do it:

#. Install gdb via package manager
#. Open terminal
#. $ gdb luminance
#. gdb's console appears
#. > run
#. Luminance HDR starts, a little slower than usually
#. Reproduce the crash
#. Go back to terminal
#. > bt
#. Copy the output using mouse and paste it somewhere
#. > quit

If you are on Windows or Mac or simply do not have time to fiddle with gdb,
at least own up and tell us exactly what you did.


Reporting bugs and requesting features
======================================

You can submit bugs reports to our `bugtracker <https://github.com/LuminanceHDR/LuminanceHDR/issues/new>`__.

A good, useful bug report contains:

#. List of actions that led to a bug
#. Backtrace, if the application crashed (see above)
#. Information on your system

Typical information on your system we would appreciate:

- Linux: name and version of distribution, version of Qt, version of Luminance HDR.
- Mac OS X: version of Mac OS X, version of Luminance HDR.
- Windows: version of Windows, version of Luminance HDR.

If you want to request a new feature, please also use the feature request tracker.
