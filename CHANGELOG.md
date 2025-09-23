- ```FEAT```:
    - Added support for environmental variables with ability to be set in the program itself, instead of setting them in the ```config.json``` file.
      (see examples/ for usage)
    - Added support for updates and deletes, see ```README.md``` examples section for a demo.

- ```FIX```:
    - Fixed the ```'text'``` datatype where we were appending the size '0' when it is not needed. Removed that from the ```text``` datatype in ```CharField``` class.
