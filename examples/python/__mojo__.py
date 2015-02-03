# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Example python application implementing the Echo service."""

import logging

import application_mojom
import example_service_mojom
import service_provider_mojom
import shell_mojom

import mojo_system

class ApplicationImpl(application_mojom.Application):
  def __init__(self, app_request_handle):
    self._providers = []
    application_mojom.Application.manager.Bind(self, app_request_handle)

  def Initialize(self, shell, args):
    self.shell = shell

  def AcceptConnection(self, requestor_url, services, exposed_services):
    # We keep a reference to ServiceProviderImpl to ensure neither it nor
    # provider gets garbage collected.
    service_provider = ServiceProviderImpl(services)
    service_provider.AddService(ExampleServiceImpl)
    services.Bind(service_provider)
    self._providers.append(service_provider)


class ServiceProviderImpl(service_provider_mojom.ServiceProvider):
  def __init__(self, provider):
    self._provider = provider
    self._name_to_service_connector = {}

  def AddService(self, service_class):
    self._name_to_service_connector[service_class.manager.name] = service_class

  def ConnectToService(self, interface_name, pipe):
    if interface_name in self._name_to_service_connector:
      service = self._name_to_service_connector[interface_name]
      service.manager.Bind(service(), pipe)
    else:
      logging.error("Unable to find service " + interface_name)


class ExampleServiceImpl(example_service_mojom.ExampleService):
  def Ping(self, ping_value):
    return ping_value


def MojoMain(app_request_handle):
  """MojoMain is the entry point for a python Mojo module."""
  loop = mojo_system.RunLoop()

  application = ApplicationImpl(mojo_system.Handle(app_request_handle))
  application.manager.AddOnErrorCallback(loop.Quit)

  loop.Run()
