//
// Copyright 2017 Shivansh Rai
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
// 1. Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
// OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
// LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
// OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
// SUCH DAMAGE.
//
// $FreeBSD$

#include <list>
#include <string>
#include <unordered_map>

using namespace std;

namespace tool {

  // Option relation which maps option names
  // to the identifier in their descriptions.
  typedef struct opt_rel {
    char type;            // Option type: (s)short/(l)long.
    string value;         // Name of the option.
    string keyword;       // The keyword which should be looked up in the
                          // message (if) produced when using this option.
  } opt_rel;

  class opt_def {
    public:
      string utility;               // Utility under test
      // TODO: Add support for long_opts
      string opt_list;       // String of all the accepted options.

      unordered_map<string, opt_rel> opt_map;   // Map "option value" to "option definition".
      unordered_map<string, opt_rel>::iterator opt_map_iter;

      // Insert a list of user-defined option definitions
      // into a hashmap. These specific option definitions
      // are the ones which one can be easily tested.
      void insert_opts();

      // For the utility under test, find the valid supported
      // options present in the hashmap generated by insert_opts()
      // and return them in a form of string which can be passed
      // directly to getopt().
      list<opt_rel*> check_opts();
  };
}
