# Copyright (c) 2026 Capgemini Engineering Research and Development.
#
# This file is part of OCCT-Light software library.
#
# This library is free software; you can redistribute it and/or modify it under
# the terms of the GNU Affero General Public License version 3 as published
# by the Free Software Foundation, with an option to use any later version.
# Consult the file LICENSE_AGPL_30.txt included in OCCT-Light distribution
# for complete text of the license and disclaimer of any warranty.
#
# Alternatively, this file may be used under the terms of a commercial
# license or contractual agreement.
#
# SPDX-License-Identifier: AGPL-3.0-or-later

{
  "targets": [
    {
      "target_name": "occtl_node",
      "sources": [
        "src/native/addon.cc",
        "src/native/raw.cc",
        "src/native/error.cc",
        "src/native/handle_wrapper.cc",
        "src/native/iter.cc",
        "src/native/span.cc"
      ],
      "include_dirs": [
        "<!@(node -p \"require('node-addon-api').include\")",
        "../../include"
      ],
      "defines": [
        "NAPI_DISABLE_CPP_EXCEPTIONS",
        "NAPI_VERSION=8"
      ],
      "cflags!":     ["-fno-exceptions"],
      "cflags_cc!":  ["-fno-exceptions"],
      "cflags":     ["-std=c++17"],
         "cflags_cc":  ["-std=c++17"],
      "conditions": [
        ["OS=='mac'", {
          "xcode_settings": {
            "GCC_ENABLE_CPP_EXCEPTIONS": "NO",
            "CLANG_CXX_LANGUAGE_STANDARD": "c++17",
            "MACOSX_DEPLOYMENT_TARGET": "11.0",
            "OTHER_LDFLAGS": [
              "-Wl,-rpath,@loader_path",
              "-Wl,-rpath,@loader_path/../../../../build/full-shared/lib",
              "-Wl,-rpath,@loader_path/../../../../build/full-with-viz-shared/lib",
              "-Wl,-rpath,@loader_path/../../../../build/minimal/lib",
              "-Wl,-rpath,@loader_path/../../../../build/cad/lib",
              "-Wl,-rpath,@loader_path/../../../../build/full-with-viz/lib",
              "-Wl,-rpath,@loader_path/../../../../build/full/lib"
            ]
          },
          "libraries": [
            "-L../../../build/full-shared/lib",
            "-L../../../build/full-with-viz-shared/lib",
            "-L../../../build/minimal/lib",
            "-L../../../build/cad/lib",
            "-L../../../build/full-with-viz/lib",
            "-L../../../build/full/lib",
            "<!(node -p \"(() => { const raw=(process.env.OCCTL_LIBRARY_NAME || 'occtl-full').trim(); const noExt=raw.replace(/\\.(dylib|so|dll|lib)$/i, ''); const noLib=noExt.replace(/^lib/i, ''); const norm=/^occtl-/i.test(noLib) ? noLib : ('occtl-' + noLib); return '-l' + norm; })()\")"
          ]
        }],
        ["OS=='linux'", {
          "ldflags": [
            "-Wl,-rpath,$$ORIGIN",
            "-Wl,-rpath,$$ORIGIN/../../../../build/full-shared/lib",
            "-Wl,-rpath,$$ORIGIN/../../../../build/full-with-viz-shared/lib",
            "-Wl,-rpath,$$ORIGIN/../../../../build/minimal/lib",
            "-Wl,-rpath,$$ORIGIN/../../../../build/cad/lib",
            "-Wl,-rpath,$$ORIGIN/../../../../build/full-with-viz/lib",
            "-Wl,-rpath,$$ORIGIN/../../../../build/full/lib"
          ],
          "libraries": [
            "-L../../../build/full-shared/lib",
            "-L../../../build/full-with-viz-shared/lib",
            "-L../../../build/minimal/lib",
            "-L../../../build/cad/lib",
            "-L../../../build/full-with-viz/lib",
            "-L../../../build/full/lib",
            "<!(node -p \"(() => { const raw=(process.env.OCCTL_LIBRARY_NAME || 'occtl-full').trim(); const noExt=raw.replace(/\\.(dylib|so|dll|lib)$/i, ''); const noLib=noExt.replace(/^lib/i, ''); const norm=/^occtl-/i.test(noLib) ? noLib : ('occtl-' + noLib); return '-l' + norm; })()\")"
          ]
        }],
        ["OS=='win'", {
          "msvs_settings": {
            "VCCLCompilerTool": {
              "ExceptionHandling": 0,
              "AdditionalOptions": ["/std:c++17"]
            }
          },
          "libraries": [
            "<!(node -p \"(() => { const raw=(process.env.OCCTL_LIBRARY_NAME || 'occtl-full').trim(); const noExt=raw.replace(/\\.(dylib|so|dll|lib)$/i, ''); const noLib=noExt.replace(/^lib/i, ''); const norm=/^occtl-/i.test(noLib) ? noLib : ('occtl-' + noLib); return norm + '.lib'; })()\")"
          ]
        }]
      ]
    }
  ]
}
