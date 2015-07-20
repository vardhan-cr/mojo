# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Functions that configure the shell before it is run manipulating its argument
list.
"""

import urlparse

# When spinning up servers for local origins, we want to use predictable ports
# so that caching works between subsequent runs with the same command line.
_LOCAL_ORIGIN_PORT = 31840
_MAP_ORIGIN_BASE_PORT = 31841

_MAP_ORIGIN_PREFIX = '--map-origin='

# Port on which the mojo:debugger http server will be available on the host
# machine.
_MOJO_DEBUGGER_PORT = 7777

_SKY_SERVER_PORT = 9998


def _IsMapOrigin(arg):
  """Returns whether |arg| is a --map-origin argument."""
  return arg.startswith(_MAP_ORIGIN_PREFIX)


def _Split(l, pred):
  positive = []
  negative = []
  for v in l:
    if pred(v):
      positive.append(v)
    else:
      negative.append(v)
  return (positive, negative)


def _RewriteMapOriginParameter(shell, mapping, device_port):
  parts = mapping[len(_MAP_ORIGIN_PREFIX):].split('=')
  if len(parts) != 2:
    return mapping
  dest = parts[1]
  # If the destination is a url, don't map it.
  if urlparse.urlparse(dest)[0]:
    return mapping
  # Assume the destination is a local directory and serve it.
  localUrl = shell.ServeLocalDirectory(dest, device_port)
  print 'started server at %s for %s' % (dest, localUrl)
  return _MAP_ORIGIN_PREFIX + parts[0] + '=' + localUrl


def RewriteMapOriginParameters(shell, original_arguments):
  """Spawns a server for each local destination indicated in a map-origin
  argument in |original_arguments| and rewrites it to point to the server url.
  The arguments other than "map-origin" and "map-origin" arguments pointing to
  web urls are left intact.

  Args:
    shell: The shell that is being configured.
    original_arguments: List of arguments to be rewritten.

  Returns:
    The updated argument list.
  """
  map_arguments, other_arguments = _Split(original_arguments, _IsMapOrigin)
  arguments = other_arguments
  next_port = _MAP_ORIGIN_BASE_PORT
  for mapping in sorted(map_arguments):
    arguments.append(_RewriteMapOriginParameter(shell, mapping, next_port))
    next_port += 1
  return arguments


def ConfigureDebugger(shell):
  """Configures mojo:debugger to run and sets up port forwarding for its http
  server if the shell is running on a device.

  Returns:
    Arguments that need to be appended to the shell argument list in order to
    run with the debugger.
  """
  shell.ForwardHostPortToShell(_MOJO_DEBUGGER_PORT)
  return ['mojo:debugger %d' % _MOJO_DEBUGGER_PORT]


def ConfigureSky(shell, root_path, sky_packages_path, sky_target):
  """Configures additional mappings and a server needed to run the given Sky
  app.

  Args:
    root_path: Local path to the root from which Sky apps will be served.
    sky_packages_path: Local path to the root from which Sky packages will be
        served.
    sky_target: Path to the Sky app to be run, relative to |root_path|.

  Returns:
    Arguments that need to be appended to the shell argument list.
  """
  # Configure a server to serve the checkout root at / (so that Sky examples
  # are accessible using a root-relative path) and Sky packages at /packages.
  # This is independent from the server that potentially serves the origin
  # directory containing the mojo: apps.
  additional_mappings = [
      ('packages/', sky_packages_path),
  ]
  server_url = shell.ServeLocalDirectory(root_path, port=_SKY_SERVER_PORT,
      additional_mappings=additional_mappings)

  args = []
  # Configure the content type mappings for the sky_viewer. This is needed
  # only for the Sky apps that do not declare mojo:sky_viewer in a shebang,
  # and it is unfortunate as it configures the shell to map all items of the
  # application/dart content-type as Sky apps.
  # TODO(ppi): drop this part once we can rely on the Sky files declaring
  # correct shebang.
  args = AppendToArgument(args, '--content-handlers=',
                          'text/sky,mojo:sky_viewer')
  args = AppendToArgument(args, '--content-handlers=',
                          'application/dart,mojo:sky_viewer')

  # Configure the window manager to embed the sky_viewer.
  sky_url = server_url + sky_target
  args.append('mojo:window_manager %s' % sky_url)
  return args


def ConfigureLocalOrigin(shell, local_dir, fixed_port=True):
  """Sets up a local http server to serve files in |local_dir| along with
  device port forwarding if needed.

  Returns:
    The list of arguments to be appended to the shell argument list.
  """

  origin_url = shell.ServeLocalDirectory(
      local_dir, _LOCAL_ORIGIN_PORT if fixed_port else 0)
  return ["--origin=" + origin_url]


def AppendToArgument(arguments, key, value, delimiter=","):
  """Looks for an argument of the form "key=val1,val2" within |arguments| and
  appends |value| to it.

  If the argument is not present in |arguments| it is added.

  Args:
    arguments: List of arguments for the shell.
    key: Identifier of the argument, including the equal sign, eg.
        "--content-handlers=".
    value: The value to be appended, after |delimeter|, to the argument.
    delimiter: The string used to separate values within the argument.

  Returns:
    The updated argument list.
  """
  assert key and key.endswith('=')
  assert value

  for i, argument in enumerate(arguments):
    if not argument.startswith(key):
      continue
    arguments[i] = argument + delimiter + value
    break
  else:
    arguments.append(key + value)

  return arguments
