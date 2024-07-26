# namigator

namigator is a drop-in, high-performance, and *nearly* feature complete
pathfinding system for server-controlled movement of units within a World of
Warcraft environment.

## Components

namigator is made of three different types of components: applications, shared
libraries, and third-party dependencies.

### Applications

* **MapBuilder** -- Generates navigation mesh for a given map, or geometrical
data for a collection of objects.  **NOTE:** This application can also build a
python module.
* **MapViewer** -- A DirectX application to load input data, any mesh data, and
allows for testing and debugging.

### Shared Libraries

* **pathfind** -- The core pathfinding library.  This is the only library you
should need to include in your server.  **NOTE:** This library can also build a
python module.
* **parser** -- For parsing source terrain data.  This library is not needed
for navigation functionality.  Only for computation.  It should **not** be
needed in your server.
* **utility** -- Various generic mathematical and structural utilities used
across the other components.  You should also not need to include this in your
server.

### Third-Party Dependencies

* **stormlib** -- Used for extracting source data from its containers.
* **recastnavigation** -- Underlying computational geometry library.  Used for
mesh generation, pathfinding, and more.
* **pybind11** -- Optional.  Used in creating python modules.
* **python** -- Optional.  Used in creating python modules.

## Future Plans

namigator is being released because I have lost interest in taking it to the
next step myself.  I also think it might give other interested people a good
start.  I have some thoughts on what functionality should be added.  This is
documented on the issue tracker for the most part.  My interest in developing
this functionality will probably be proportional to adoption by others.  While
initially this was a purely academic exercise, I no longer have time to devote
to development unless it will be helpful to the larger community.

## Caveats

This is something I have developed off and on for nearly a decade.  I have
never used it, but have tested it fairly thoroughly in a controlled setting. I
have also put in a fair amount of effort to understand why common bugs happen
and mitigate them (e.g. falling through the world, stuttering, etc.).  At this
point I do not have time to invest in integrating this library into an existing
server.  However, if someone else wants to do that, and in doing so discovers
some issues, I can probably assist in correcting them.  I can also provide some
guidance in how to integrate the library.  Feel free to contact me privately
for this.

### Production Testing

namigator has been integrated into [The Alpha Project](https://github.com/The-Alpha-Project/alpha-core)
and several robustness issues have been addressed as a result.  Thanks to them!

### Thread Safety

The thread safety of the map builder and parser should be correct.  However the
thread safety of the pathfind library is likely not fool proof.  Use with
caution!

### Bots

This software is not designed to honor client-side movement restrictions, and
is therefore not natively suitable for use in bots.  Though its possible such
capability may evolve, I also avoided explicitly supporting this use case. My
goal is not to improve the ability of those who undermine the integrity of a
server with large scale automation.

## Feedback

Please use the issue tracker on this repository to submit issues, feature
requests, etc..  Do not use the issue tracker to request help with integration,
unless there is a bug causing your problem.
