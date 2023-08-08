{
  "targets": [
    {
      "target_name": "megacache",
      "cflags": [ "-O3", "-fno-exceptions" ],
      "cflags_cc": [ "-O3", "-fno-exceptions" ],
      "sources": [ "main.cc", "cache.cc", "MegaCache.cpp" ],
      "include_dirs": [
        "<!@(node -p \"require('node-addon-api').include\")"
      ],
      'defines': [ 'NAPI_DISABLE_CPP_EXCEPTIONS' ],
    }
  ]
}
