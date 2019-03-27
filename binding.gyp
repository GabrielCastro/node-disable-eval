{
  "targets": [
    {
      "target_name": "evalmanager",
      "sources": [ "lib/index.cc" ],
      "include_dirs" : [
        "<!(node -e \"require('nan')\")"
      ]
    }
  ]
}
