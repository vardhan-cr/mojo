# benchmark

This application connects to another mojo application, collects traces during
indicated period of time and computes a number of results based on the collected
traces. It can be used to measure performance of a mojo app, provided that the
app being benchmarked participates in the [tracing
ecosystem](mojo/services/tracing/public/interfaces/tracing.mojom).

## Arguments

The benchmarking app expects the following arguments:

 - `--app=<app_url>` - url of the application to be benchmarked
 - `--duration=<duration_seconds>` - duration of the benchmark in seconds

any other arguments are assumed to be descriptions of measurements to be
conducted on the collected trace data. Each measurement has to be of form:
`<measurement_type>/<trace_event_category>/<trace_event_name>`.

The following measurement types are available:

 - `time_until` - measures time until the first occurence of the event named
   `trace_event_name` in category `trace_event_category`.
 - `avg_duration` - measures average duration of all events named
   `trace_event_name` in category `trace_event_category`.

## Runner script

Devtools offers [a helper script](mojo/devtools/common/mojo_benchmark) allowing
to run a list of benchmarks, including on an Android device.
