# Smoke testing of base utilities (FreeBSD)

This repo is currently a subset of a broader task which is to be done during [Google Summer of Code '17 with FreeBSD](https://summerofcode.withgoogle.com/projects/#6426676740227072).  
**Refer the [FreeBSD wiki](https://wiki.freebsd.org/SummerOfCode2017/SmokeTestingOfBaseUtilities) for an overview and updates.**

## Directory Structure
```
.
├── baseutils
│   ├── ........................:: Base utilities
│   └── ls
│       └── tests ..............:: Smoke tests pertaining to the initial test plan
├── scripts
│   └── ........................:: Handy scripts
└── tool........................:: Automation tool pertaining to the new test plan
    ├── generated_tests
    │   └── ....................:: Generated atf-sh test scripts
    ├── generate_test.cpp.......:: Test generation file
    ├── groff
    │   └── ....................:: groff scripts for utilities
    └── utils.cpp...............:: Index generation file
```
- - -

## Automation tool
An (in-progress) implementation of the automation tool briefly described [here](https://lists.freebsd.org/pipermail/soc-status/2017-July/001079.html).

### Instructions
Execute the following commands inside [tool](tool) -
```
make clean
make && make run
```
The expected result (at the time of writing) should be generation of 3 atf-sh test files under [generated_tests](generated_tests).

## Generated tests
The following table summarizes the type of test-cases produced for 3 (randomly chosen) utilities.

|           **Test**            | **Positive test-cases** | **Negative test-cases** |
--------------------------------|:-----------------------:|:-----------------------:|
[date_test.sh](generated_tests/date_test.sh)    | 5 | 2
[ln_test.sh](generated_tests/ln_test.sh)        | 0 | 11
[stdbuf_test.sh](generated_tests/stdbuf_test.sh)| 1 | 0

**NOTE :** [date_test.sh](generated_tests/date_test.sh) is time/timezone dependent, hence all the test-cases apart from `invalid_usage` will fail.  
The aim for using this utility is to demonstrate that the tool can generate positive test-cases along with negative ones. It also demonstrates the need to sanity check the tests.
