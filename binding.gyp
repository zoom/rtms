{
  "targets": [
    {
      "target_name": "rtms",
      "sources": [
        "src/rtms.cpp",
        "src/rtms.h",
        "src/node.cpp"
      ],
      "include_dirs": [
        "<!@(node -p \"require('node-addon-api').include\")",
        "./lib/librtmsdk",
        "./lib/librtmsdk/h",
        "<!@(node -e \"const paths = (process.env.LD_LIBRARY_PATH || '').split(':').filter(Boolean); for (const p of paths) console.log(p);\")"
      ],
      "defines": [
        "NAPI_VERSION=9",
        "NODE_ADDON_API_CPP_EXCEPTIONS",
        "NODE_ADDON_API_CPP_EXCEPTIONS_ALL"
      ],
      "cflags!": [ "-fno-exceptions" ],
      "cflags_cc!": [ "-fno-exceptions" ],
      "conditions": [
        ['OS=="linux"', {
          "libraries": [
            "-lrtmsdk"
          ],
          "library_dirs": [
            "./lib/librtmsdk",
            "<!@(node -e \"const paths = (process.env.LD_LIBRARY_PATH || '').split(':').filter(Boolean); for (const p of paths) console.log(p);\")"
          ],
          "ldflags": [
            "-Wl,-rpath,'$$ORIGIN'",
            "-Wl,-rpath,'$$ORIGIN/../lib/librtmsdk'",
            "<!@(node -e \"const paths = (process.env.LD_LIBRARY_PATH || '').split(':').filter(Boolean); for (const p of paths) console.log('-Wl,-rpath,' + p);\")"
          ],
          "cflags_cc": [
            "-std=c++20"
          ]
        }]
      ],
      "xcode_settings": {
        "GCC_ENABLE_CPP_EXCEPTIONS": "YES",
        "CLANG_CXX_LIBRARY": "libc++",
        "MACOSX_DEPLOYMENT_TARGET": "10.15",
        "OTHER_CFLAGS": [
          "-std=c++20"
        ]
      },
      "msvs_settings": {
        "VCCLCompilerTool": {
          "ExceptionHandling": 1,
          "AdditionalOptions": [
            "/std:c++20"
          ]
        }
      }
    }
  ]
}