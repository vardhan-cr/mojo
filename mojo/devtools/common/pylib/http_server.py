# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import atexit
import datetime
import email.utils
import hashlib
import logging
import math
import os.path
import threading

import SimpleHTTPServer
import SocketServer


ZERO = datetime.timedelta(0)


class UTC_TZINFO(datetime.tzinfo):
  """UTC time zone representation."""

  def utcoffset(self, _):
    return ZERO

  def tzname(self, _):
    return "UTC"

  def dst(self, _):
     return ZERO

UTC = UTC_TZINFO()


class _SilentTCPServer(SocketServer.TCPServer):
  """
  A TCPServer that won't display any error, unless debugging is enabled. This is
  useful because the client might stop while it is fetching an URL, which causes
  spurious error messages.
  """
  def handle_error(self, request, client_address):
    """
    Override the base class method to have conditional logging.
    """
    if logging.getLogger().isEnabledFor(logging.DEBUG):
      SocketServer.TCPServer.handle_error(self, request, client_address)


def _GetHandlerClassForPath(base_path):
  class RequestHandler(SimpleHTTPServer.SimpleHTTPRequestHandler):
    """
    Handler for SocketServer.TCPServer that will serve the files from
    |base_path| directory over http.
    """

    def __init__(self, *args, **kwargs):
      self.etag = None
      SimpleHTTPServer.SimpleHTTPRequestHandler.__init__(self, *args, **kwargs)

    def get_etag(self):
      if self.etag:
        return self.etag

      path = self.translate_path(self.path)
      if not os.path.isfile(path):
        return None

      sha256 = hashlib.sha256()
      BLOCKSIZE = 65536
      with open(path, 'rb') as hashed:
        buf = hashed.read(BLOCKSIZE)
        while len(buf) > 0:
          sha256.update(buf)
          buf = hashed.read(BLOCKSIZE)
      self.etag = '"%s"' % sha256.hexdigest()
      return self.etag

    def send_head(self):
      # Always close the connection after each request, as the keep alive
      # support from SimpleHTTPServer doesn't like when the client requests to
      # close the connection before downloading the full response content.
      # pylint: disable=W0201
      self.close_connection = 1

      path = self.translate_path(self.path)
      if os.path.isfile(path):
        # Handle If-None-Match
        etag = self.get_etag()
        if ('If-None-Match' in self.headers and
            etag == self.headers['If-None-Match']):
          self.send_response(304)
          return None

        # Handle If-Modified-Since
        if ('If-None-Match' not in self.headers and
            'If-Modified-Since' in self.headers):
          last_modified = datetime.datetime.fromtimestamp(
              math.floor(os.stat(path).st_mtime), tz=UTC)
          ims = datetime.datetime(
              *email.utils.parsedate(self.headers['If-Modified-Since'])[:6],
              tzinfo=UTC)
          if last_modified <= ims:
            self.send_response(304)
            return None

      return SimpleHTTPServer.SimpleHTTPRequestHandler.send_head(self)

    def end_headers(self):
      path = self.translate_path(self.path)

      if os.path.isfile(path):
        etag = self.get_etag()
        if etag:
          self.send_header('ETag', etag)
          self.send_header('Cache-Control', 'must-revalidate')

      return SimpleHTTPServer.SimpleHTTPRequestHandler.end_headers(self)

    def translate_path(self, path):
      path_from_current = (
          SimpleHTTPServer.SimpleHTTPRequestHandler.translate_path(self, path))
      return os.path.join(base_path, os.path.relpath(path_from_current))

    def log_message(self, *_):
      """
      Override the base class method to disable logging.
      """
      pass

  RequestHandler.protocol_version = 'HTTP/1.1'
  return RequestHandler


def StartHttpServer(path):
  """Starts an http server serving files from |path| on random
  (system-allocated) port. Returns the server address."""
  assert path
  httpd = _SilentTCPServer(('127.0.0.1', 0), _GetHandlerClassForPath(path))
  atexit.register(httpd.shutdown)

  http_thread = threading.Thread(target=httpd.serve_forever)
  http_thread.daemon = True
  http_thread.start()
  return httpd.server_address
