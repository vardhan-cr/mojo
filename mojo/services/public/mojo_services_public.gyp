# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'includes': [
    '../../mojo_variables.gypi',
  ],
  'targets': [
    {
      'target_name': 'mojo_services_public',
      'type': 'none',
      'dependencies': [
        'mojo_clipboard_bindings',
        'mojo_content_handler_bindings',
        'mojo_geometry_bindings',
        'mojo_gpu_bindings',
        'mojo_input_events_bindings',
        'mojo_native_viewport_bindings',
        'mojo_navigation_bindings',
        'mojo_network_bindings',
        'mojo_surface_id_bindings',
        'mojo_surfaces_bindings',
        'mojo_view_manager_bindings',
        'mojo_view_manager_common',
        'mojo_window_manager_bindings',
      ],
    },
    {
      # GN version: //mojo/services/public/interfaces/clipboard
      'target_name': 'mojo_clipboard_bindings',
      'type': 'static_library',
      'sources': [
        'interfaces/clipboard/clipboard.mojom',
      ],
      'includes': [ '../../public/tools/bindings/mojom_bindings_generator.gypi' ],
      'dependencies': [
        '../../public/mojo_public.gyp:mojo_cpp_bindings',
      ],
      'export_dependent_settings': [
        '../../public/mojo_public.gyp:mojo_cpp_bindings',
      ],
    },
    {
      # GN version: //mojo/services/public/interfaces/input_events
      'target_name': 'mojo_input_events_bindings',
      'type': 'static_library',
      'sources': [
        'interfaces/input_events/input_event_constants.mojom',
        'interfaces/input_events/input_events.mojom',
        'interfaces/input_events/input_key_codes.mojom',
      ],
      'includes': [ '../../public/tools/bindings/mojom_bindings_generator.gypi' ],
      'dependencies': [
        'mojo_geometry_bindings',
        '../../public/mojo_public.gyp:mojo_cpp_bindings',
      ],
      'export_dependent_settings': [
        'mojo_geometry_bindings',
        '../../public/mojo_public.gyp:mojo_cpp_bindings',
      ],
    },
    {
      # GN version: //mojo/services/geometry/public/interfaces
      'target_name': 'mojo_geometry_bindings',
      'type': 'static_library',
      'sources': [
        '../geometry/public/interfaces/geometry.mojom',
      ],
      'includes': [ '../../public/tools/bindings/mojom_bindings_generator.gypi' ],
      'dependencies': [
        '../../public/mojo_public.gyp:mojo_cpp_bindings',
      ],
      'export_dependent_settings': [
        '../../public/mojo_public.gyp:mojo_cpp_bindings',
      ],
    },
    {
      # GN version: //mojo/services/public/interfaces/gpu
      'target_name': 'mojo_gpu_bindings',
      'type': 'static_library',
      'sources': [
        'interfaces/gpu/command_buffer.mojom',
        'interfaces/gpu/gpu.mojom',
        'interfaces/gpu/gpu_capabilities.mojom',
        'interfaces/gpu/viewport_parameter_listener.mojom',
      ],
      'includes': [ '../../public/tools/bindings/mojom_bindings_generator.gypi' ],
      'dependencies': [
        'mojo_geometry_bindings',
        '../../public/mojo_public.gyp:mojo_cpp_bindings',
      ],
      'export_dependent_settings': [
        'mojo_geometry_bindings',
        '../../public/mojo_public.gyp:mojo_cpp_bindings',
      ],
    },
    {
      # GN version: //mojo/services/public/interfaces/native_viewport
      'target_name': 'mojo_native_viewport_bindings',
      'type': 'static_library',
      'sources': [
        'interfaces/native_viewport/native_viewport.mojom',
      ],
      'includes': [ '../../public/tools/bindings/mojom_bindings_generator.gypi' ],
      'dependencies': [
        'mojo_geometry_bindings',
        'mojo_gpu_bindings',
        'mojo_input_events_bindings',
        'mojo_surface_id_bindings',
        '../../public/mojo_public.gyp:mojo_cpp_bindings',
      ],
      'export_dependent_settings': [
        'mojo_geometry_bindings',
        'mojo_gpu_bindings',
        'mojo_input_events_bindings',
        'mojo_surface_id_bindings',
        '../../public/mojo_public.gyp:mojo_cpp_bindings',
      ],
    },
    {
      # GN version: //mojo/services/public/interfaces/navigation
      'target_name': 'mojo_navigation_bindings',
      'type': 'static_library',
      'sources': [
        'interfaces/navigation/navigation.mojom',
      ],
      'includes': [ '../../public/tools/bindings/mojom_bindings_generator.gypi' ],
      'export_dependent_settings': [
        '../../public/mojo_public.gyp:mojo_cpp_bindings',
      ],
      'dependencies': [
        'mojo_network_bindings',
        '../../public/mojo_public.gyp:mojo_cpp_bindings',
      ],
    },
    {
      # GN version: //mojo/services/public/interfaces/content_handler
      'target_name': 'mojo_content_handler_bindings',
      'type': 'static_library',
      'sources': [
        'interfaces/content_handler/content_handler.mojom',
      ],
      'includes': [ '../../public/tools/bindings/mojom_bindings_generator.gypi' ],
      'export_dependent_settings': [
        '../../public/mojo_public.gyp:mojo_cpp_bindings',
      ],
      'dependencies': [
        'mojo_network_bindings',
        '../../public/mojo_public.gyp:mojo_application_bindings',
        '../../public/mojo_public.gyp:mojo_cpp_bindings',
      ],
    },
    {
      # GN version: //mojo/services/network/public/interfaces
      'target_name': 'mojo_network_bindings',
      'type': 'static_library',
      'sources': [
        '../network/public/interfaces/cookie_store.mojom',
        '../network/public/interfaces/net_address.mojom',
        '../network/public/interfaces/network_error.mojom',
        '../network/public/interfaces/network_service.mojom',
        '../network/public/interfaces/tcp_bound_socket.mojom',
        '../network/public/interfaces/tcp_connected_socket.mojom',
        '../network/public/interfaces/tcp_server_socket.mojom',
        '../network/public/interfaces/udp_socket.mojom',
        '../network/public/interfaces/url_loader.mojom',
        '../network/public/interfaces/web_socket.mojom',
      ],
      'includes': [ '../../public/tools/bindings/mojom_bindings_generator.gypi' ],
      'export_dependent_settings': [
        '../../public/mojo_public.gyp:mojo_cpp_bindings',
      ],
      'dependencies': [
        '../../public/mojo_public.gyp:mojo_cpp_bindings',
      ],
    },
    {
      # GN version: //mojo/services/view_manager/public/cpp:common
      'target_name': 'mojo_view_manager_common',
      'type': 'static_library',
      'sources': [
        '../view_manager/public/cpp/types.h',
      ],
    },
    {
      # GN version: //mojo/services/view_manager/public/interfaces/
      'target_name': 'mojo_view_manager_bindings',
      'type': 'static_library',
      'sources': [
        '../view_manager/public/interfaces/view_manager.mojom',
        '../view_manager/public/interfaces/view_manager_constants.mojom',
      ],
      'includes': [ '../../public/tools/bindings/mojom_bindings_generator.gypi' ],
      'dependencies': [
        'mojo_geometry_bindings',
        'mojo_input_events_bindings',
        'mojo_surface_id_bindings',
        '../../public/mojo_public.gyp:mojo_application_bindings',
        '../../public/mojo_public.gyp:mojo_cpp_bindings',
      ],
      'export_dependent_settings': [
        'mojo_geometry_bindings',
        'mojo_input_events_bindings',
        'mojo_surface_id_bindings',
        '../../public/mojo_public.gyp:mojo_application_bindings',
        '../../public/mojo_public.gyp:mojo_cpp_bindings',
      ],
    },
    {
      # GN version: //mojo/services/surfaces/public/interfaces
      'target_name': 'mojo_surfaces_bindings',
      'type': 'static_library',
      'sources': [
        '../surfaces/public/interfaces/surfaces.mojom',
        '../surfaces/public/interfaces/surfaces_service.mojom',
        '../surfaces/public/interfaces/quads.mojom',
      ],
      'includes': [ '../../public/tools/bindings/mojom_bindings_generator.gypi' ],
      'dependencies': [
        'mojo_geometry_bindings',
        'mojo_gpu_bindings',
        'mojo_surface_id_bindings',
        '../../public/mojo_public.gyp:mojo_cpp_bindings',
      ],
      'export_dependent_settings': [
        'mojo_geometry_bindings',
        'mojo_surface_id_bindings',
        '../../public/mojo_public.gyp:mojo_cpp_bindings',
      ],
    },
    {
      # GN version: //mojo/services/surfaces/public/interfaces:surface_id
      'target_name': 'mojo_surface_id_bindings',
      'type': 'static_library',
      'sources': [
        '../surfaces/public/interfaces/surface_id.mojom',
      ],
      'includes': [ '../../public/tools/bindings/mojom_bindings_generator.gypi' ],
      'dependencies': [
        '../../public/mojo_public.gyp:mojo_cpp_bindings',
      ],
      'export_dependent_settings': [
        '../../public/mojo_public.gyp:mojo_cpp_bindings',
      ],
    },
    {
      # GN version: //mojo/services/window_manager/public/interfaces
      'target_name': 'mojo_window_manager_bindings',
      'type': 'static_library',
      'sources': [
        '../window_manager/public/interfaces/window_manager.mojom',
      ],
      'includes': [ '../../public/tools/bindings/mojom_bindings_generator.gypi' ],
      'dependencies': [
        'mojo_input_events_bindings',
        '../../public/mojo_public.gyp:mojo_application_bindings',
        '../../public/mojo_public.gyp:mojo_cpp_bindings',
      ],
      'export_dependent_settings': [
        'mojo_input_events_bindings',
        '../../public/mojo_public.gyp:mojo_application_bindings',
        '../../public/mojo_public.gyp:mojo_cpp_bindings',
      ],
    },
  ],
}
