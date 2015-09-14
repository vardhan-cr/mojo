# mojo_benchmark

`mojo_benchmark` allows you to run performance tests for any Mojo application
that participates in the [tracing
ecosystem](https://github.com/domokit/mojo/blob/master/mojo/services/tracing/public/interfaces/tracing.mojom)
with no app changes required.

The script reads a list of benchmarks to run from a file, runs each benchmark in
controlled caching conditions with tracing enabled and performs specified
measurements on the collected trace data.

## Defining benchmarks

`mojo_benchmark` runs performance tests defined in a benchmark file. The
benchmark file is a Python dictionary of the following format:

```python
benchmarks = [
  {
    'name': '<name of the benchmark>',
    'app': '<url of the app to benchmark>',
    'shell-args': [],
    'duration': <duration in seconds>,

    # List of measurements to make.
    'measurements': [
      '<measurement type>/<event category>/<event name>',
    ],
  },
]
```

The following types of measurements are available:

 - `time_until` - measures time until the first occurence of the specified event
 - `avg_duration` - measures the average duration of all instances of the 
    specified event

## Caching

The script runs each benchmark twice. The first run (**cold start**) clears the
following caches before start:

 - url_response_disk_cache

The second run (**warm start**) runs immediately afterwards, without clearing
any caches.

## Time origin

The underlying benchmark runner records the time origin just before issuing the
connection call to the application being benchmarked. Results of `time_until`
measurements are relative to this time.

## Example

For an app that records a trace event named "initialized" in category "my_app"
once its initialization is complete, we can benchmark the initialization time of
the app (from the moment someone tries to connect to it to the app completing
its initialization) using the following benchmark file:

```python
benchmarks = [
  {
    'name': 'My app initialization',
    'app': 'https://my_domain/my_app.mojo',
    'duration': 10,
    'measurements': [
      'time_until/my_app/initialized',
    ],
  },
]
```
