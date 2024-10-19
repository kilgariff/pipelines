# Pipelines
A Linux command-line utility for watching directories and running commands when files in those directories change.

It was developed to enable fast iteration when developing command-line programs with dependencies in many different directories. Changing a file in any dependency directory could trigger a rebuild of the dependency, then a rebuild of the top-level program, and then run it with predefined arguments.

Works well in conjunction with GNU Make.

# How it works
A `Pipefile` is created in the working directory of a project. It defines a series of pipelines in the YAML format:

```
pipelines:
    - mkdcdisc:
        workdir: "/opt/toolchains/dc/mkdcdisc/build/mkdcdisc"
        watch_paths: "src"
        cmd: "meson compile -C build"
    
    - libisofs:
        workdir: "/home/ross/libisofs"
        watch_paths: "libisofs"
        cmd: "make"

    - iso-tools:
        workdir: "/home/ross/iso-tools"
        watch_paths:
            - "/opt/toolchains/dc/mkdcdisc/build/mkdcdisc"
            - "/home/ross/libisofs/install"
            - "."
        cmd: "make run"
```

Running `pipelines` will read the `Pipefile` from the working directory and start monitoring each pipeline's `watch_paths` for changes. When a change is detected, the pipeline's `cmd` is executed using `/bin/sh`.

# Contributing

This tool is something I threw together quickly because it solved an immediate problem I had. There are rough endges and missing features. Please feel free to file Issues, submit Pull Requests or get in touch with me at https://ross.codes/ if you have any questions.
